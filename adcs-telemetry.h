#ifndef ADCS_TELEMETRY_H
#define ADCS_TELEMETRY_H

#include <stdint.h>

struct ADCS3DData {
   int32_t x, y, z;
} __attribute__((packed));

struct ADCSReaderStatus {
   struct ADCS3DData accel;
   struct ADCS3DData gyro;
   struct ADCS3DData mag;
} __attribute__((packed));

#endif
