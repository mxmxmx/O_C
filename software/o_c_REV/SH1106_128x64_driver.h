#ifndef SH1106_128X64_DRIVER_H_
#define SH1106_128X64_DRIVER_H_

#include <stdint.h>
#include <string.h>

struct SH1106_128x64_Driver {
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages = 8;
  static constexpr size_t kPageSize = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 2;

  static void Init();
  static void Flush();
  static void SendPage(uint_fast8_t index, const uint8_t *data);

  static uint8_t data_start_seq[];
  static uint8_t init_seq[];

  // SH1106 ram is 132x64, so it needs an offset to center data in display.
  // However at least one display (mine) uses offset 0 so it's minimally
  // configurable
  static void AdjustOffset(uint8_t offset);
};

#endif // SH1106_128X64_DRIVER_H_
