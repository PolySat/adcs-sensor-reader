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
#include <zlib.h>
#include "adcs-telemetry.h"

#define WAIT_MS (2 * 1000)

struct MulticallInfo;

static int adcs_status(int, char**, struct MulticallInfo *);
static int adcs_telemetry(int, char**, struct MulticallInfo *);
static int adcs_get_cs(int, char**, struct MulticallInfo *);
static int adcs_set_cs(int, char**, struct MulticallInfo *);
static int adcs_read_dump(int, char**, struct MulticallInfo *);

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
   { &adcs_get_cs, "adcs-get-cs", "-G", 
       "Display the current values in the adcs critical state -G" }, 
   { &adcs_set_cs, "adcs-set-cs", "-s", 
       "Change the values in the adcs critical state -s" }, 
   { &adcs_read_dump, "adcs-read-dump", "-R", 
       "Read a dump file and convert into KVP -R" }, 
   { NULL, NULL, NULL, NULL }
};

static int adcs_read_dump(int argc, char **argv, struct MulticallInfo * self) 
{
   struct ADCSReaderLog entry;
   gzFile inp;
   int count = 0;

   if (argc < 2) {
      printf("Usage: %s <input file>\n", argv[0]);
      return 0;
   }

   inp = gzopen(argv[1], "r");
   if (!inp) {
      printf("Failed to open %s\n", argv[1]);
      return -1;
   }

   while (sizeof(entry) == gzread(inp, &entry, sizeof(entry))) {
      count++;
      printf("accel_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.accel.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("accel_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.accel.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("accel_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.accel.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("gyro_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.gyro.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("gyro_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.gyro.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("gyro_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.gyro.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_mb.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_mb.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_mb.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nz_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nz.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nz_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nz.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nz_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nz.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("pz_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_pz.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("pz_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_pz.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("pz_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_pz.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("ny_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_ny.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("ny_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_ny.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("ny_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_ny.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("py_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_py.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("py_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_py.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("py_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_py.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nx_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nx.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nx_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nx.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("nx_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_nx.z),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("px_mag_x=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_px.x),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("px_mag_y=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_px.y),
         ntohl(entry.sec), ntohl(entry.usec));
      printf("px_mag_z=%d,%u.%06u\n", (int32_t)ntohl(entry.status.mag_px.z),
         ntohl(entry.sec), ntohl(entry.usec));
   }

   gzclose(inp);

   return 0;
}

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

#if 0
#define DFL_HIGHRES_DELAY (60*60)
#define DFL_HIGHRES_PERIOD (24*60*60)
#define DFL_SAMPLE_PERIOD_MS 100
#define DLF_SAMPLES (20*10)
#endif
static int adcs_set_cs(int argc, char **argv, struct MulticallInfo * self) 
{
   struct {
      uint8_t cmd;
      uint8_t res;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
      struct CriticalState cs;
   } __attribute__((packed)) send;

   send.cmd = ADCS_SET_CS_CMD;
   const char *ip = "127.0.0.1";
   int len, opt;

   send.cs.init_highres_delay = htonl(DFL_HIGHRES_DELAY);
   send.cs.highres_period = htonl(DFL_HIGHRES_PERIOD);
   send.cs.sample_cnt = htonl(DFL_SAMPLE_PERIOD_MS);
   send.cs.sample_period_ms = htonl(DLF_SAMPLES);
   
   while ((opt = getopt(argc, argv, "h:d:p:c:s:")) != -1) {
      switch(opt) {
         case 'h':
            ip = optarg;
            break;
         case 'd':
            send.cs.init_highres_delay = htonl(atol(optarg));
            break;
         case 'p':
            send.cs.highres_period = htonl(atol(optarg));
            break;
         case 'c':
            send.cs.sample_cnt = htonl(atol(optarg));
            break;
         case 's':
            send.cs.sample_period_ms = htonl(atol(optarg));
            break;
      }
   }
   
   // send packet and wait for response
   if ((len = socket_send_packet_and_read_response(ip, "adcs", &send, 
    sizeof(send), &resp, sizeof(resp), WAIT_MS)) <= 0) {
      return len;
   }
 
   if (resp.cmd != ADCS_SET_CS_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ADCS_SET_CS_RESP);
      return 5;
   }

   // print out returned values   
   if (resp.res)
      printf("Spacecraft reported an error setting critical state\n");

   return 0;
}

static int adcs_get_cs(int argc, char **argv, struct MulticallInfo * self) 
{
   struct {
      uint8_t cmd;
      struct CriticalState cs;
   } __attribute__((packed)) resp;

   struct {
      uint8_t cmd;
   } __attribute__((packed)) send;

   send.cmd = ADCS_GET_CS_CMD;
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
 
   if (resp.cmd != ADCS_GET_CS_RESP) {
      printf("response code incorrect, Got 0x%02X expected 0x%02X\n", 
       resp.cmd, ADCS_GET_CS_RESP);
      return 5;
   }

   // print out returned values   
   printf("Delay before first high res dataset: %u [s]\n",
       ntohl(resp.cs.init_highres_delay));
   printf("Delay between high res datasets: %u [s]\n",
       ntohl(resp.cs.highres_period));
   printf("Number of samples in high res dataset: %u\n",
       ntohl(resp.cs.sample_cnt));
   printf("Time between samples: %u [ms]\n",
       ntohl(resp.cs.sample_period_ms));

   return 0;
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
   printf("mag_x=%d\n", (int32_t)ntohl(resp.status.mag_mb.x));
   printf("mag_y=%d\n", (int32_t)ntohl(resp.status.mag_mb.y));
   printf("mag_z=%d\n", (int32_t)ntohl(resp.status.mag_mb.z));
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
