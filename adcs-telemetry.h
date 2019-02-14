#ifndef ADCS_TELEMETRY_H
#define ADCS_TELEMETRY_H

#include <stdint.h>

#define DFL_HIGHRES_DELAY (60*60)
#define DFL_HIGHRES_PERIOD (24*60*60)
#define DFL_SAMPLE_PERIOD_MS 100
#define DLF_SAMPLES (20*10)

struct ADCS3DData {
   int32_t x, y, z;
} __attribute__((packed));

struct ADCSReaderStatus {
   struct ADCS3DData accel;
   struct ADCS3DData gyro;
   struct ADCS3DData mag_mb;
   struct ADCS3DData mag_nx;
   struct ADCS3DData mag_px;
   struct ADCS3DData mag_ny;
   struct ADCS3DData mag_py;
   struct ADCS3DData mag_nz;
   struct ADCS3DData mag_pz;
} __attribute__((packed));

struct CriticalState {
   uint32_t init_highres_delay;
   uint32_t highres_period;
   uint32_t sample_cnt;
   uint32_t sample_period_ms;
} __attribute__((packed));

struct ADCSReaderLog {
   uint32_t sec;
   uint32_t usec;
   struct ADCSReaderStatus status;
} __attribute__((packed));

#define ADCS_GET_CS_CMD 41
#define ADCS_GET_CS_RESP (ADCS_GET_CS_CMD | 0x80)
#define ADCS_SET_CS_CMD 42
#define ADCS_SET_CS_RESP (ADCS_SET_CS_CMD | 0x80)

#endif
