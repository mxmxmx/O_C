// Copyright (c) 2015 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef PAGESTORAGE_H_
#define PAGESTORAGE_H_

#include "util_misc.h"

//#define DEBUG_STORAGE
#ifdef DEBUG_STORAGE
#define STORAGE_PRINTF(fmt, ...) serial_printf(fmt, ##__VA_ARGS__)
#else
#define STORAGE_PRINTF(...)
#endif

enum EStorageMode {
  STORAGE_UPDATE,
  STORAGE_WRITE
};

/**
 * Helper to save settings in a definable storage (e.g. EEPROM).
 *
 * Divides the available space into fixed-length pages, so we can use a very
 * basic wear-levelling to write pages in a different location each time.
 * EEPROM only has a limited number of (reliable) write cycles so this helps
 * to increase the usable time. Also, the page is only updated when the
 * contents have changed.
 *
 * The "newest" version is identified as the page with the highest generation
 * number; this is an uint32_t so should be safe for a while. A very simple
 * CRC is used to check validity.
 *
 * Note that storage is uninitialized until ::load is called!
 *
 * Using the MODE parameter, the storage can either use a plain write or define
 * an UPDATE method that furthere minimizes actual writes, but may be slower.
 *
 * The optional FASTSCAN parameter to can be used for force a scan of all pages
 * during ::load. If it is true, the scan stops at the first non-good page,
 * which is faster but might miss pages if a write is corrupted.
 */
template <typename STORAGE, size_t BASE_ADDR, size_t END_ADDR, typename DATA_TYPE, EStorageMode MODE = STORAGE_UPDATE, bool FASTSCAN=true>
class PageStorage {
protected:
 
  struct page_header {
    uint32_t fourcc;
    uint32_t generation;
    uint16_t size;
    uint16_t checksum;
  } __attribute__((aligned(2)));

  /**
   * Binary page structure (aligned to uint16_t for CRC calculation)
   */
  struct page_data {
    page_header header;
    DATA_TYPE data;

  } __attribute__((aligned(2)));

public:

  static const size_t LENGTH = END_ADDR - BASE_ADDR;
  static const size_t PAGESIZE = sizeof(page_data);
  static const size_t PAGES = LENGTH / PAGESIZE;

  // throw compiler error if no pages
  typedef bool CHECK_PAGES[PAGES > 0 ? 1 : -1 ];
  // throw compiler error if OOB
  typedef bool CHECK_BASEADDR[BASE_ADDR + LENGTH > STORAGE::LENGTH ? -1 : 1];

  /**
   * @return index of page in storage; only valid after ::load
   */
  int page_index() const {
    return page_index_;
  }

  /**
   * Use this instead of load to get a valid state for saving
   */
  void Init() {
    page_index_ = -1;
    page_.header.fourcc = DATA_TYPE::FOURCC;
    page_.header.size = sizeof(DATA_TYPE);
  }

  /**
   * Scan all pages to find the newest to load.
   * If no data found, initialize internal page to empty.
   * @param data [out] loaded data if load successful, else unmodified
   * @return true if data loaded
   */
  bool Load(DATA_TYPE &data) {

    page_index_ = -1;
    memset(&page_, 0, sizeof(page_));
    page_.header.generation = -1;
    page_data next_page;
    for (size_t i = 0; i < PAGES; ++i) {
      STORAGE::read(BASE_ADDR + i * PAGESIZE, &next_page, sizeof(next_page));

      STORAGE_PRINTF("[%u]\n", BASE_ADDR + i * PAGESIZE);
      STORAGE_PRINTF("FOURCC:%x (%x)\n", next_page.header.fourcc, DATA_TYPE::FOURCC);
      STORAGE_PRINTF("size  :%u (%u)\n", next_page.header.size, sizeof(DATA_TYPE));
      STORAGE_PRINTF("gen   :%u (%u)\n", next_page.header.generation, page_.header.generation);

      if ((DATA_TYPE::FOURCC != next_page.header.fourcc) ||
          (sizeof(DATA_TYPE) != next_page.header.size) ||
          (next_page.header.checksum != checksum(next_page)) ||
          (next_page.header.generation < page_.header.generation && (int32_t)page_.header.generation != -1)) {
        if (FASTSCAN) {
          STORAGE_PRINTF("Aborting scan at page %d\n", i);
          break;
        } else {
          STORAGE_PRINTF("Ignoring page %d\n", i);
          continue;
        }
      }

      page_index_ = i;
      memcpy(&page_, &next_page, sizeof(page_));
    }
    
    if (-1 == page_index_) {
      page_.header.fourcc = DATA_TYPE::FOURCC;
      page_.header.size = sizeof(DATA_TYPE);
      return false;
    } else {
      memcpy(&data, &page_.data, sizeof(DATA_TYPE));
      return true;
    }
  }

  /**
   * Save data to storage; assumes ::load has been called!
   * @param data data to be stored
   * @return true if data was written to storage
   */
  bool Save(const DATA_TYPE &data) {

    bool dirty = false;
    const uint8_t *src = (const uint8_t*)&data;
    uint8_t *dst = (uint8_t*)&page_.data;
    size_t length = sizeof(DATA_TYPE);
    while (length--) {
      if (*dst != *src) {
        dirty = true;
        *dst = *src;
      }
      ++dst;
      ++src;
    }

    if (dirty) {
      ++page_.header.generation;
      page_.header.checksum = checksum(page_);
      page_index_ = (page_index_ + 1) % PAGES;

      if (STORAGE_UPDATE == MODE)
        STORAGE::update(BASE_ADDR + page_index_ * PAGESIZE, &page_, sizeof(page_));
      else
        STORAGE::write(BASE_ADDR + page_index_ * PAGESIZE, &page_, sizeof(page_));
    }

    return dirty;
  }

protected:

  int page_index_;
  page_data page_;

  static uint16_t checksum(const page_data &page) {
    uint16_t c = 0;
    // header not included in crc
    const uint8_t *p = (const uint8_t *)&page.data;
    size_t length = sizeof(page_data) - sizeof(page_header);
    while (length--) {
      c += *p++;
    }

    return c ^ 0xffff;
  }
};

#endif // PAGESTORAGE_H_

