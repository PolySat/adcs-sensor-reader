/** 
 * Test code for process command handling
 * @author 
 *
 */

#include <polysat/polysat.h>
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
#include "adcs-telemetry.h"

#define WAIT_MS (2 * 1000)

#ifdef BR_TELEM_PATH
#define DFL_TELEM_PATH BR_TELEM_PATH
#else
#define DFL_TELEM_PATH "/usr/bin/adcs-sensor-reader-util"
#endif

struct MulticallInfo;

static int adcs_status(int, char**, struct MulticallInfo *);
static int adcs_telemetry(int, char**, struct MulticallInfo *);
static int adcs_datalogger(int, char**, struct MulticallInfo *);
static int adcs_sensor_metadata(int, char **, struct MulticallInfo *);
static int adcs_json_telem(int, char **, struct MulticallInfo *);


// struct holding all possible function calls
// running the executable with the - flags will call that function
// running without flags will print out this struct
struct MulticallInfo {
   int (*func)(int argc, char **argv, struct MulticallInfo *);
   const char *name;
   const char *opt;
   const char *help;
} multicall[] = {
   { &adcs_status, "adcs-status", "-S", 
       "Display the current status of the adcs process -S" }, 
   { &adcs_telemetry, "adcs-telemetry", "-T", 
       "Display the current KVP telemetry of the adcs process -T" }, 
   { &adcs_datalogger, "adcs-datalogger", "-dl", 
       "Print a list of sensors supported by the -telemetry app in a format suitable for datalogger"},
   { &adcs_sensor_metadata, "adcs-sensor-metadata", "-meta",
                       "Print a list of sensors supported by the -telemetry app in a format suitable for use with the ground-based telemetry database"},
   { &adcs_json_telem, "adcs-json-telem", "-json", 
                       "Print an JSON telemetry dictionary"},


   { NULL, NULL, NULL, NULL }
};

static int adcs_status(int argc, char **argv, struct MulticallInfo * self) 
{
   struct {
      uint8_t cmd;
      struct ADCSReaderStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "adcs", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }

   // print out returned status values   
   printf("Accel X=%f [G]\n", ((int32_t)ntohl(resp.status.accel.x))
                                              / (1024.0*1024.0*16.0));
   printf("Accel Y=%f [G]\n", ((int32_t)ntohl(resp.status.accel.y))
                                              / (1024.0*1024.0*16.0));
   printf("Accel Z=%f [G]\n", ((int32_t)ntohl(resp.status.accel.z))
                                              / (1024.0*1024.0*16.0));

   printf("Gyro X=%f [d/s]\n", ((int32_t)ntohl(resp.status.gyro.x))
                                              / (1024.0*1024.0));
   printf("Gyro Y=%f [d/s]\n", ((int32_t)ntohl(resp.status.gyro.y))
                                              / (1024.0*1024.0));
   printf("Gyro Z=%f [d/s]\n", ((int32_t)ntohl(resp.status.gyro.z))
                                              / (1024.0*1024.0));

   printf("MB Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_mb.x));
   printf("MB Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_mb.y));
   printf("MB Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_mb.z));

   printf("-X Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nx.x));
   printf("-X Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nx.y));
   printf("-X Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nx.z));
   
   printf("+X Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_px.x));
   printf("+X Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_px.y));
   printf("+X Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_px.z));
   
   printf("-Y Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_ny.x));
   printf("-Y Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_ny.y));
   printf("-Y Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_ny.z));
   
   printf("+Y Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_py.x));
   printf("+Y Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_py.y));
   printf("+Y Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_py.z));
   
   printf("-Z Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nz.x));
   printf("-Z Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nz.y));
   printf("-Z Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_nz.z));
   
   printf("+Z Mag X=%d [nT]\n", (int32_t)ntohl(resp.status.mag_pz.x));
   printf("+Z Mag Y=%d [nT]\n", (int32_t)ntohl(resp.status.mag_pz.y));
   printf("+Z Mag Z=%d [nT]\n", (int32_t)ntohl(resp.status.mag_pz.z));
   
   return 0;
}

static struct TELMEventInfo events[] = {
   { 0, 0, NULL, NULL }
};


struct TELMTelemetryInfo telemetryPoints[] = {
   { "accel_x", "motherboard", "software", "G", 1, 0,
     "Accelerometer X",
     "X Accelerration in Gs" },
   { "accel_y", "motherboard", "software", "G", 1, 0,
     "Accelerometer Y",
     "Y Accelerration in Gs" },
   { "accel_z", "motherboard", "software", "G", 1, 0,
     "Accelerometer Z",
     "Z Accelerration in Gs" },

   { "gyro_x", "motherboard", "software", "d/s", 1, 0,
     "Gyroscope X",
     "X Gyro in d/s" },
   { "gyro_y", "motherboard", "software", "d/s", 1, 0,
     "Gyroscope X",
     "X Gyro in d/s" },
   { "gyro_z", "motherboard", "software", "d/s", 1, 0,
     "Gyroscope Z",
     "Z Gyro in d/s" },
   
   { "mb_mag_x", "motherboard", "software", "nT", 1, 0,
     "Motherboard Magnetometer X",
     "Motherboard X magnetism in nTs" },
   { "mb_mag_y", "motherboard", "software", "nT", 1, 0,
     "Motherboard Magnetometer Y",
     "Motherboard Y magnetism in nTs" },
   { "mb_mag_z", "motherboard", "software", "nT", 1, 0,
     "Motherboard Magnetometer Z",
     "Motherboard Z magnetism in nTs" },

   { "nz_mag_x", "-z", "software", "nT", 1, 0,
     "Negative Z magnetometer X",
     "Negative Z magnetism in nTs" },
   { "nz_mag_y", "-z", "software", "nT", 1, 0,
     "Negative Z magnetometer Y",
     "Negative Z Y magnetism in nTs" },
   { "nz_mag_z", "-z", "software", "nT", 1, 0,
     "Negative Z magnetometer Z",
     "Negative Z Z magnetism in nTs" },

   { "pz_mag_x", "+z", "software", "nT", 1, 0,
     "Positive Z magnetometer X",
     "Positive Z magnetism in nTs" },
   { "pz_mag_y", "+z", "software", "nT", 1, 0,
     "Positive Z magnetometer Y",
     "Positive Z Y magnetism in nTs" },
   { "pz_mag_z", "+z", "software", "nT", 1, 0,
     "Positive Z magnetometer Z",
     "Positive Z Z magnetism in nTs" },

   { "ny_mag_x", "-y", "software", "nT", 1, 0,
     "Negative Y magnetometer X",
     "Negative Y magnetism in nTs" },
   { "ny_mag_y", "-y", "software", "nT", 1, 0,
     "Negative Y magnetometer Y",
     "Negative Y Y magnetism in nTs" },
   { "ny_mag_z", "-y", "software", "nT", 1, 0,
     "Negative Y magnetometer Z",
     "Negative Y Z magnetism in nTs" },

   { "py_mag_x", "+y", "software", "nT", 1, 0,
     "Positive Y magnetometer X",
     "Positive Y magnetism in nTs" },
   { "py_mag_y", "+y", "software", "nT", 1, 0,
     "Positive Y magnetometer Y",
     "Positive Y Y magnetism in nTs" },
   { "py_mag_z", "+y", "software", "nT", 1, 0,
     "Positive Y magnetometer Z",
     "Positive Y Z magnetism in nTs" },

   { "nx_mag_x", "-x", "software", "nT", 1, 0,
     "Negative X magnetometer X",
     "Negative X magnetism in nTs" },
   { "nx_mag_y", "-x", "software", "nT", 1, 0,
     "Negative X magnetometer Y",
     "Negative X Y magnetism in nTs" },
   { "nx_mag_z", "-x", "software", "nT", 1, 0,
     "Negative X magnetometer Z",
     "Negative X Z magnetism in nTs" },

   { "px_mag_x", "+x", "software", "nT", 1, 0,
     "Positive X magnetometer X",
     "Positive X magnetism in nTs" },
   { "px_mag_y", "+x", "software", "nT", 1, 0,
     "Positive X magnetometer Y",
     "Positive X Y magnetism in nTs" },
   { "px_mag_z", "+x", "software", "nT", 1, 0,
     "Positive X magnetometer Z",
     "Positive X Z magnetism in nTs" },

   { NULL, NULL },
};

static int adcs_datalogger(int argc, char **argv, struct MulticallInfo * self)
{
   return TELM_print_datalogger_info(telemetryPoints, "adcs-sensor-reader", DFL_TELEM_PATH, argc, argv);
}

static int adcs_sensor_metadata(int argc, char **argv, struct MulticallInfo * self)
{
   return TELM_print_sensor_metadata(telemetryPoints, events);
}
static int adcs_json_telem(int argc, char **argv, struct MulticallInfo * self)
{
   return TELM_print_json_telem_dict(telemetryPoints, events, argc, argv);
}


/* get telemetry from ADCS process
 * @param argc number of command line arguments
 * @param argv char array of command line arguments
 * @return 0 on succes, failure otherwise
 */
static int adcs_telemetry(int argc, char **argv, struct MulticallInfo * self) 
{
   struct {
      uint8_t cmd;
      struct ADCSReaderStatus status;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = 1;
   const char *ip = "127.0.0.1";
   int len, opt;
   
   while ((opt = getopt(argc, argv, "h:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "adcs", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != CMD_STATUS_RESPONSE) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, CMD_STATUS_RESPONSE);
      return 5;
   }

   // print out returned status values   
   printf("accel_x=%d\n", (int32_t)ntohl(resp.status.accel.x));
   printf("accel_y=%d\n", (int32_t)ntohl(resp.status.accel.y));
   printf("accel_z=%d\n", (int32_t)ntohl(resp.status.accel.z));
   printf("gyro_x=%d\n", (int32_t)ntohl(resp.status.gyro.x));
   printf("gyro_y=%d\n", (int32_t)ntohl(resp.status.gyro.y));
   printf("gyro_z=%d\n", (int32_t)ntohl(resp.status.gyro.z));
   printf("mb_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_mb.x));
   printf("mb_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_mb.y));
   printf("mb_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_mb.z));
   printf("nz_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_nz.x));
   printf("nz_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_nz.y));
   printf("nz_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_nz.z));
   printf("pz_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_pz.x));
   printf("pz_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_pz.y));
   printf("pz_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_pz.z));
   printf("ny_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_ny.x));
   printf("ny_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_ny.y));
   printf("ny_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_ny.z));
   printf("py_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_py.x));
   printf("py_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_py.y));
   printf("py_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_py.z));
   printf("nx_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_nx.x));
   printf("nx_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_nx.y));
   printf("nx_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_nx.z));
   printf("px_mag_x=%d\n", (int32_t)ntohl(resp.status.mag_px.x));
   printf("px_mag_y=%d\n", (int32_t)ntohl(resp.status.mag_px.y));
   printf("px_mag_z=%d\n", (int32_t)ntohl(resp.status.mag_px.z));
   
   return 0;
}

// prints out available commands for this util
static int print_usage(const char *name)
{
   struct MulticallInfo *curr;

   printf("adcs-util multicall binary, use the following names instead:\n");

   for (curr = multicall; curr->func; curr++) {
      printf("   %-16s %s\n", curr->name, curr->help);
   }

   return 0;
}

int main(int argc, char **argv) 
{   
   struct MulticallInfo *curr;
   char *exec_name;

   exec_name = rindex(argv[0], '/');
   if (!exec_name) {
      exec_name = argv[0];
   }
   else {
      exec_name++;
   }

   for (curr = multicall; curr->func; curr++) {
      if (!strcmp(curr->name, exec_name)) {
         return curr->func(argc, argv, curr);
      }
   }

   if (argc > 1) {
      for (curr = multicall; curr->func; curr++) {
         if (!strcmp(curr->opt, argv[1])) {
            return curr->func(argc - 1, argv + 1, curr);
         }
      }
   }
   else {
      return print_usage(argv[0]);
   }

   return 0;
}
