#ifndef PAGE_DISPLAY_DRIVER_H_
#define PAGE_DISPLAY_DRIVER_H_

#include "util_macros.h"

template <typename display_driver>
class PagedDisplayDriver {
public:

  PagedDisplayDriver() { }

  void Init() {

    display_driver::Init();

    current_page_index_ = 0;
    current_page_data_ = NULL;
  }

  void Begin(const uint8_t *frame) {
    current_page_data_ = frame;
    current_page_index_ = 0;
  }

  bool Update() {
    uint_fast8_t page = current_page_index_;
    const uint8_t *data = current_page_data_;

    display_driver::SendPage(page, data);

    if (page < display_driver::kNumPages - 1) {
      current_page_index_ = page + 1;
      current_page_data_ = data + display_driver::kPageSize;
      return false;
    } else {
      current_page_index_ = 0;
      current_page_data_ = NULL;
      return true;
    }
  }

  bool frame_valid() const {
    return NULL != current_page_data_;
  }

private:
  uint_fast8_t current_page_index_;
  const uint8_t *current_page_data_;

  DISALLOW_COPY_AND_ASSIGN(PagedDisplayDriver);
};

#endif // PAGE_DISPLAY_DRIVER_H_
