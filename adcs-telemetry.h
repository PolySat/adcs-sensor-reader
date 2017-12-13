#ifndef ADCS_TELEMETRY_H
#define ADCS_TELEMETRY_H

#include <stdint.h>

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

#endif
