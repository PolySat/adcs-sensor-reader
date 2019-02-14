#include <polysat_pkt/status-structs.h>
#include <polysat_pkt/shared-structs.h>
#include <polysat_pkt/payload_cmd.h>
#include <polysat_drivers/drivers/accelerometer.h>
#include <polysat_drivers/drivers/gyroscope.h>
#include <polysat_drivers/drivers/magnetometer.h>
#include <polysat_drivers/driverdb.h>
#include <polysat/polysat.h>
#include <polysat/util.h>
#include <polysat_pkt/filemgr_cmd.h>
#include <polysat_pkt/datalogger_cmd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <zlib.h>

#include "adcs-telemetry.h"

#define HIGHRES_DIR "/data/adcs_dumps"
#define MAX_DATA_FILE_CNT 256

struct ADCSState {
   ProcessData *proc;
   void *create_evt;
   void *highres_evt;
   void *read_evt;
   uint32_t sample_cnt;
   uint32_t sample_period_ms;
   uint32_t cnt;
   char filename[PATH_MAX];
   gzFile zout;
};

static struct ADCSState *gState;

#define SENSOR_OPEN_INTV_MS 20
#define SENSOR_RETRY_INTV_MS (1000 * 60 * 15)

#define ACCEL_TYPE_FLAG (1 << 0)
#define GYRO_TYPE_FLAG (1 << 1)
#define MAG_TYPE_FLAG (1 << 2)

// Structure to hold sensor related information
// This enables generic sensor handling code, minimizing special cases
struct SensorInfo {
   const char *name;
   const char *location;
   const char *type;
   int flags;
   void (*marshal)(struct SensorInfo *sensor, void *dst);
   int offset;
   struct Sensor *sensor;
   struct DeviceInfo *dev_info;
   int disabled;
};

// Declarations of functions that read sensor values and pack them
//  into status response
static void marshal_accel(struct SensorInfo *si, void *dst);
static void marshal_gyro(struct SensorInfo *si, void *dst);
static void marshal_mag(struct SensorInfo *si, void *dst);
int highres_sensors(void * arg);

#define ACCEL_SENSOR(n,l,field) { n, l, DRVR_CLS_ACCELEROMETER, \
    ACCEL_TYPE_FLAG, &marshal_accel, \
    offsetof(struct ADCSReaderStatus, field), \
    NULL, NULL, 0 }

#define GYRO_SENSOR(n,l,field) { n, l, DRVR_CLS_GYROSCOPE, \
    GYRO_TYPE_FLAG, &marshal_gyro, \
    offsetof(struct ADCSReaderStatus, field), \
    NULL, NULL, 0 }

#define MAG_SENSOR(n,l,field) { n, l, DRVR_CLS_MAGNETOMETER, \
    MAG_TYPE_FLAG, &marshal_mag, \
    offsetof(struct ADCSReaderStatus, field), \
    NULL, NULL, 0 }

// Sensors managed by this process
struct SensorInfo sensors[] = {
   ACCEL_SENSOR("mb_accel", DEVICE_LOCATION_MB, accel),
   GYRO_SENSOR("mb_gyro", DEVICE_LOCATION_MB, gyro),
   MAG_SENSOR("mb", DEVICE_LOCATION_MB, mag_mb),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_MINUS_Z, mag_nz),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_PLUS_Z, mag_pz),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_MINUS_Y, mag_ny),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_PLUS_Y, mag_py),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_MINUS_X, mag_nx),
   MAG_SENSOR("Magnetometer", DEVICE_LOCATION_PLUS_X, mag_px),
   { NULL, NULL, NULL, 0, NULL, 0, NULL, NULL, 0 }
};

static void marshal_accel(struct SensorInfo *si, void *dst)
{
   struct AccelerometerSensor *accel = (struct AccelerometerSensor*)si->sensor;
   struct ADCS3DData *ad = (struct ADCS3DData*)dst;
   AccelData accelCache;

   if (accel->read) {
      accel->read(accel, &accelCache);

      ad->x = htonl(accelCache.x_result);
      ad->y = htonl(accelCache.y_result);
      ad->z = htonl(accelCache.z_result);
   }
   // ad->x = htonl(accel->accelCache.x_result);
   // ad->y = htonl(accel->accelCache.y_result);
   // ad->z = htonl(accel->accelCache.z_result);
}

static void marshal_gyro(struct SensorInfo *si, void *dst)
{
   struct GyroscopeSensor *gyro = (struct GyroscopeSensor*)si->sensor;
   struct ADCS3DData *gd = (struct ADCS3DData*)dst;
   GyroData data;

   if (gyro->read) {
      if (gyro->read(gyro, &data) >= 0) {
         gd->x = htonl(data.x);
         gd->y = htonl(data.y);
         gd->z = htonl(data.z);
      }
   }
}

static void marshal_mag(struct SensorInfo *si, void *dst)
{
   struct MagnetometerSensor *mag = (struct MagnetometerSensor*)si->sensor;
   struct ADCS3DData *md = (struct ADCS3DData*)dst;

   md->x = htonl(mag->magnetometerCache.x_result);
   md->y = htonl(mag->magnetometerCache.y_result);
   md->z = htonl(mag->magnetometerCache.z_result);
}

// Read all the sensors and place their values into the status packet
static void marshal_sensors(void *dst, int cache)
{
   struct SensorInfo *curr;
   struct timeval now;

   if (cache)
      gettimeofday(&now, NULL);
   else
      now.tv_sec = now.tv_usec = 0;

   for (curr = sensors; curr->name; curr++) {
      if (curr->offset < 0 || !curr->sensor || !curr->marshal)
         continue;

      if (curr->sensor->update_cached_values)
         curr->sensor->update_cached_values(curr->sensor, &now);

      curr->marshal(curr, ((char*)dst) + curr->offset);
   }
}

static void adcs_set_critical_state_cmd(int socket, unsigned char cmd,
   void * data, size_t dataLen, struct sockaddr_in * src)
{
   struct CriticalState cs;
   int8_t res = -1;

   if (dataLen != sizeof(cs))
      return;

   memcpy(&cs, data, sizeof(cs));

   if (PROC_save_critical_state(gState->proc, &cs, sizeof(cs)) == sizeof(cs)) {
      res = 0;
      gState->sample_cnt = ntohl(cs.sample_cnt);
      gState->sample_period_ms = ntohl(cs.sample_period_ms);
      if (gState->highres_evt)
         EVT_sched_remove(PROC_evt(gState->proc), gState->highres_evt);

      gState->highres_evt = NULL;
      if (ntohl(cs.init_highres_delay) && ntohl(cs.highres_period) &&
                    gState->sample_cnt && gState->sample_period_ms)
         gState->highres_evt = EVT_sched_add_with_timestep(
                     PROC_evt(gState->proc),
                     EVT_ms2tv(ntohl(cs.init_highres_delay)*1000),
                     EVT_ms2tv(ntohl(cs.highres_period)*1000),
                     &highres_sensors, NULL);
   }


   PROC_cmd_sockaddr(gState->proc, ADCS_SET_CS_RESP, &res,
      sizeof(res), src);
}

void adcs_get_critical_state_cmd(int socket, unsigned char cmd, void *data,
            size_t dataLen, struct sockaddr_in *src)
{
   struct CriticalState cs;

   if (PROC_read_critical_state(gState->proc, &cs, sizeof(cs)) == sizeof(cs))
      PROC_cmd_sockaddr(gState->proc, ADCS_GET_CS_RESP, &cs,
         sizeof(cs), src);
}

void adcs_status(int socket, unsigned char cmd, void * data,
   size_t dataLen, struct sockaddr_in * src)
{
   struct ADCSReaderStatus status;
   struct ADCSState *adcs = gState;

   memset(&status, 0, sizeof(status));
   marshal_sensors(&status, 1);

   PROC_cmd_sockaddr(adcs->proc, CMD_STATUS_RESPONSE, &status,
               sizeof(status), src);
}

// Initializes the sensors slowly via event callbacks to minimize impact on
//  event loop stalling the process
int initialize_cfged_sensors(void * arg)
{
   struct SensorInfo *curr = NULL;

   if(arg)
      curr = (struct SensorInfo *)arg;

   while (curr && curr->name && curr->sensor)
      curr++;

   if(!curr || !curr->name){
      gState->create_evt = EVT_sched_add(PROC_evt(gState->proc),
                  EVT_ms2tv(SENSOR_RETRY_INTV_MS),
                  &initialize_cfged_sensors, NULL);
      return EVENT_REMOVE;
   }

   // create the devices
   if (!curr->disabled) {
      if (!curr->dev_info) {
         while ((curr->dev_info = enumerate_devices(curr->dev_info,
                 curr->type, curr->location)))
            if (0 == strcasecmp(curr->name, curr->dev_info->name))
               break;
      }
      if (!curr->dev_info)
         curr->disabled = 1;

      if (!curr->sensor && curr->dev_info) {
         curr->sensor = create_device(curr->dev_info);
      }
   }

   curr++;
   gState->create_evt = EVT_sched_add(PROC_evt(gState->proc),
                  EVT_ms2tv(SENSOR_OPEN_INTV_MS),
                  initialize_cfged_sensors, (void*)curr);

   return EVENT_REMOVE;
}

static void cleanup_sensors(void)
{
   struct SensorInfo *curr;

   for (curr = sensors; curr->name; curr++) {
      if (!curr->sensor || !curr->sensor->close)
         continue;

      curr->sensor->close(&curr->sensor);
   }
}

int read_sensors(void * arg)
{
   struct ADCSReaderLog entry;
   struct timeval now_tv;
   char *sep = "";
   char *ext = "gz";
   int last;
   time_t now_sec = time(0);
   struct tm now;

   if (gState->cnt++ > gState->sample_cnt) {
      gState->read_evt = NULL;
      if (gState->zout) {
         gzclose(gState->zout);
         gState->zout = NULL;
         FILEMGR_add_to_download_queue(gState->proc, gState->filename, 5, 100);
      }

      UTIL_cleanup_dir(HIGHRES_DIR, MAX_DATA_FILE_CNT, 0, NULL, NULL,
                       NULL, NULL);
      return EVENT_REMOVE;
   }

   if (!gState->zout) {
      if (!UTIL_ensure_path(HIGHRES_DIR))
         return EVENT_REMOVE;

      last = strlen(HIGHRES_DIR) - 1;
      if (last < 0)
         return EVENT_REMOVE;
      if (HIGHRES_DIR[last] != '/')
         sep = "/";

      now_sec = time(0);
      gmtime_r(&now_sec, &now);

      snprintf(gState->filename, sizeof(gState->filename),
         "%s%smag-stats.%04d-%02d-%02d.%02d:%02d:%02d.%s", HIGHRES_DIR, sep,
         now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
         now.tm_min, now.tm_sec, ext);
         gState->filename[sizeof(gState->filename) - 1] = 0;

      gState->zout = gzopen(gState->filename, "w");
   }

   memset(&entry.status, 0, sizeof(entry.status));
   marshal_sensors(&entry.status, 0);
   gettimeofday(&now_tv, NULL);
   entry.sec = htonl(now_tv.tv_sec);
   entry.usec = htonl(now_tv.tv_usec);

   if (gState->zout)
      gzwrite(gState->zout, &entry, sizeof(entry));

   return EVENT_KEEP;
}

int highres_sensors(void * arg)
{
   if (gState->read_evt)
      EVT_sched_remove(PROC_evt(gState->proc), gState->read_evt);

   if (gState->sample_cnt && gState->sample_period_ms) {
      gState->cnt = 0;
      gState->read_evt = EVT_sched_add(PROC_evt(gState->proc),
                  EVT_ms2tv(gState->sample_period_ms),
                  read_sensors, NULL);
   }

   return EVENT_KEEP;
}

// Simple SIGINT handler example
int sigint_handler(int signum, void *arg)
{
   DBG("SIGINT handler!\n");
   EVT_exit_loop(PROC_evt(arg));
   return EVENT_KEEP;
}

// Entry point
int main(int argc, char *argv[])
{
   struct ADCSState adcs;
   struct CriticalState cs;

   // initialize state structure
   memset(&adcs, 0, sizeof(adcs));
   gState = &adcs;

   // Initialize the process
   adcs.proc = PROC_init("adcs");
   if (!adcs.proc)
      return -1;

   /* Enable debug interface for development
    * Can be removed in 'published' revisions
    */
   DBG_INIT();

   // Add a signal handler call back for SIGINT signal
   PROC_signal(adcs.proc, SIGINT, &sigint_handler, adcs.proc);
   PROC_set_cmd_handler(adcs.proc, ADCS_GET_CS_CMD,
                 adcs_get_critical_state_cmd, 0, 0, 0);
   PROC_set_cmd_handler(adcs.proc, ADCS_SET_CS_CMD,
                 adcs_set_critical_state_cmd, 0, 0, 0);

   initialize_cfged_sensors(sensors);

   // Set up recurring mag measurements
   if (PROC_read_critical_state(adcs.proc, &cs, sizeof(cs)) != sizeof(cs)) {
      cs.init_highres_delay = htonl(DFL_HIGHRES_DELAY);
      cs.highres_period = htonl(DFL_HIGHRES_PERIOD);
      cs.sample_cnt = htonl(DLF_SAMPLES);
      cs.sample_period_ms = htonl(DFL_SAMPLE_PERIOD_MS);

      PROC_save_critical_state(adcs.proc, &cs, sizeof(cs));
   }

   adcs.sample_cnt = ntohl(cs.sample_cnt);
   adcs.sample_period_ms = ntohl(cs.sample_period_ms);
   if (ntohl(cs.init_highres_delay) && ntohl(cs.highres_period) &&
                    adcs.sample_cnt && adcs.sample_period_ms)
      gState->highres_evt = EVT_sched_add_with_timestep(PROC_evt(gState->proc),
                  EVT_ms2tv(ntohl(cs.init_highres_delay)*1000),
                  EVT_ms2tv(ntohl(cs.highres_period)*1000),
                  &highres_sensors, NULL);

   // Enter the main event loop
   EVT_start_loop(PROC_evt(adcs.proc));

   // Remove any pending events
   if (adcs.create_evt)
      EVT_sched_remove(PROC_evt(adcs.proc), adcs.create_evt);
   if (adcs.read_evt)
      EVT_sched_remove(PROC_evt(adcs.proc), adcs.read_evt);
   if (adcs.highres_evt)
      EVT_sched_remove(PROC_evt(adcs.proc), adcs.highres_evt);

   // Close any open sensors
   cleanup_sensors();

   // Clean up, whenever we exit event loop
   PROC_cleanup(adcs.proc);

   return 0;
}
