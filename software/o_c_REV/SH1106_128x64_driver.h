#ifndef SH1106_128X64_DRIVER_H_
#define SH1106_128X64_DRIVER_H_

#include <stdint.h>
#include <string.h>

struct SH1106_128x64_Driver {
  static const size_t kFrameSize = 128 * 64 / 8;
  static const size_t kNumPages = 8;
  static const size_t kPageSize = kFrameSize / kNumPages;

  static void Init();
  static void Flush();
  static void SendPage(uint_fast8_t index, const uint8_t *data);

  static uint8_t data_start_seq[];
  static uint8_t init_seq[];
};

#endif // SH1106_128X64_DRIVER_H_
