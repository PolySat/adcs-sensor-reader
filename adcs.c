#include <polysat_pkt/status-structs.h>
#include <polysat_pkt/shared-structs.h>
#include <polysat_pkt/payload_cmd.h>
#include <polysat_drivers/drivers/accelerometer.h>
#include <polysat_drivers/drivers/gyroscope.h>
#include <polysat_drivers/drivers/magnetometer.h>
#include <polysat_drivers/driverdb.h>
#include <polysat/polysat.h>
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

#include "polypkt/adcs-sensor-reader-pkt.h"
#include "adcs-telemetry.h"

struct ADCSState {
   ProcessData *proc;
   void *create_evt;
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
   void (*marshal_legacy)(struct SensorInfo *sensor, void *dst);
   void (*marshal)(struct SensorInfo *sensor, void *dst);
   int legacy_offset;
   int offset;
   struct Sensor *sensor;
   struct DeviceInfo *dev_info;
   int disabled;
};

// Declarations of functions that read sensor values and pack them
//  into status response
static void marshal_accel_xdr(struct SensorInfo *si, void *dst);
static void marshal_gyro_xdr(struct SensorInfo *si, void *dst);
static void marshal_mag_xdr(struct SensorInfo *si, void *dst);
static void marshal_accel_legacy(struct SensorInfo *si, void *dst);
static void marshal_gyro_legacy(struct SensorInfo *si, void *dst);
static void marshal_mag_legacy(struct SensorInfo *si, void *dst);

#define ACCEL_SENSOR(n,l,field) { n, l, DRVR_CLS_ACCELEROMETER, \
    ACCEL_TYPE_FLAG, &marshal_accel_legacy, &marshal_accel_xdr, \
    offsetof(struct ADCSReaderStatus, field), \
    offsetof(struct IPC_ADCS_Status, field), \
    NULL, NULL, 0 }

#define GYRO_SENSOR(n,l,field) { n, l, DRVR_CLS_GYROSCOPE, \
    GYRO_TYPE_FLAG, &marshal_gyro_legacy, &marshal_gyro_xdr, \
    offsetof(struct ADCSReaderStatus, field), \
    offsetof(struct IPC_ADCS_Status, field), \
    NULL, NULL, 0 }

#define MAG_SENSOR(n,l,field) { n, l, DRVR_CLS_MAGNETOMETER, \
    MAG_TYPE_FLAG, &marshal_mag_legacy, &marshal_mag_xdr, \
    offsetof(struct ADCSReaderStatus, field), \
    offsetof(struct IPC_ADCS_Status, field), \
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
   { NULL, NULL, NULL, 0, NULL, NULL, 0, 0, NULL, NULL, 0 }
};

static void marshal_accel_legacy(struct SensorInfo *si, void *dst)
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
}

static void marshal_accel_xdr(struct SensorInfo *si, void *dst)
{
   struct AccelerometerSensor *accel = (struct AccelerometerSensor*)si->sensor;
   struct IPC_ADCS_Vector *vec = (struct IPC_ADCS_Vector*)dst;
   AccelData accelCache;

   if (accel->read) {
      accel->read(accel, &accelCache);

      vec->x = accelCache.x_result;
      vec->y = accelCache.y_result;
      vec->z = accelCache.z_result;
   }
}

static void marshal_gyro_legacy(struct SensorInfo *si, void *dst)
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

static void marshal_gyro_xdr(struct SensorInfo *si, void *dst)
{
   struct GyroscopeSensor *gyro = (struct GyroscopeSensor*)si->sensor;
   struct IPC_ADCS_Vector *vec = (struct IPC_ADCS_Vector*)dst;
   GyroData data;

   if (gyro->read) {
      if (gyro->read(gyro, &data) >= 0) {
         vec->x = data.x;
         vec->y = data.y;
         vec->z = data.z;
      }
   }
}

static void marshal_mag_legacy(struct SensorInfo *si, void *dst)
{
   struct MagnetometerSensor *mag = (struct MagnetometerSensor*)si->sensor;
   struct ADCS3DData *md = (struct ADCS3DData*)dst;

   md->x = htonl(mag->magnetometerCache.x_result);
   md->y = htonl(mag->magnetometerCache.y_result);
   md->z = htonl(mag->magnetometerCache.z_result);
}

static void marshal_mag_xdr(struct SensorInfo *si, void *dst)
{
   struct MagnetometerSensor *mag = (struct MagnetometerSensor*)si->sensor;
   struct IPC_ADCS_Vector *vec = (struct IPC_ADCS_Vector*)dst;

   vec->x = mag->magnetometerCache.x_result;
   vec->y = mag->magnetometerCache.y_result;
   vec->z = mag->magnetometerCache.z_result;
}

// Read all the sensors and place their values into the status packet
static void marshal_sensors(void *dst)
{
   struct SensorInfo *curr;
   struct timeval now;

   gettimeofday(&now, NULL);
   for (curr = sensors; curr->name; curr++) {
      if (curr->offset < 0 || !curr->sensor || !curr->marshal)
         continue;

      if (curr->sensor->update_cached_values)
         curr->sensor->update_cached_values(curr->sensor, &now);

      curr->marshal(curr, ((char*)dst) + curr->offset);
   }
}

// Read all the sensors and place their values into the status packet
static void marshal_sensors_legacy(void *dst)
{
   struct SensorInfo *curr;
   struct timeval now;

   gettimeofday(&now, NULL);
   for (curr = sensors; curr->name; curr++) {
      if (curr->legacy_offset < 0 || !curr->sensor || !curr->marshal)
         continue;

      if (curr->sensor->update_cached_values)
         curr->sensor->update_cached_values(curr->sensor, &now);

      curr->marshal(curr, ((char*)dst) + curr->legacy_offset);
   }
}

static void adcs_status_populator(void *arg, XDR_tx_struct cb,
                  void *cb_args)
{
   struct IPC_ADCS_Status status;
   memset(&status, 0, sizeof(status));

   marshal_sensors(&status);

   cb(&status, cb_args, IPC_RESULTCODE_SUCCESS);
}

void adcs_status_legacy(int socket, unsigned char cmd, void * data,
   size_t dataLen, struct sockaddr_in * src)
{
   struct ADCSReaderStatus status;
   struct ADCSState *adcs = gState;

   memset(&status, 0, sizeof(status));
   marshal_sensors_legacy(&status);

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

   IPC_ADCS_forcelink();

   // initialize state structure
   memset(&adcs, 0, sizeof(adcs));
   gState = &adcs;

   // Initialize the process
   adcs.proc = PROC_init("adcs", WD_XDR);
   if (!adcs.proc)
      return -1;

   /* Enable debug interface for development
    * Can be removed in 'published' revisions
    */
   DBG_INIT();

   // Add a signal handler call back for SIGINT signal
   PROC_signal(adcs.proc, SIGINT, &sigint_handler, adcs.proc);

   initialize_cfged_sensors(sensors);

   XDR_register_populator(&adcs_status_populator, adcs.proc,
         IPC_ADCS_TYPES_STATUS);

   // Enter the main event loop
   EVT_start_loop(PROC_evt(adcs.proc));

   // Remove any pending events
   if (adcs.create_evt)
      EVT_sched_remove(PROC_evt(adcs.proc), adcs.create_evt);

   // Close any open sensors
   cleanup_sensors();

   // Clean up, whenever we exit event loop
   PROC_cleanup(adcs.proc);

   return 0;
}
