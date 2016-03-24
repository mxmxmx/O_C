#ifndef DRIVERS_FRAMEBUFFER_H_
#define DRIVERS_FRAMEBUFFER_H_

#include "../util/util_macros.h"

// - This could be specialized for frames == 2 (i.e. double-buffer)
// - Takes some short-cuts so assumes correct order of calls

// Since this is essentially a standard ring buffer implementation, it uses
// only (frames - 1) element to be able to distinguish between empty/full.
// To have frames useable items see:
// https://gist.github.com/patrickdowling/0029f58fb20e63d7db9d

template <size_t frame_size, size_t frames>
class FrameBuffer {
public:

  static const size_t kFrameSize = frame_size;

  FrameBuffer() { }

  void Init() {
    memset(frame_memory_, 0, sizeof(frame_memory_));
    for (size_t f = 0; f < frames; ++f)
      frame_buffers_[f] = frame_memory_ + kFrameSize * f;
    write_ptr_ = read_ptr_ = 0;
  }

  size_t writeable() const {
    return (read_ptr_ - write_ptr_ - 1) % frames;
  }

  size_t readable() const {
    return (write_ptr_ - read_ptr_) % frames;
  }

  // @return readable frame (assumes one exists)
  const uint8_t *readable_frame() const {
    return frame_buffers_[read_ptr_];
  }

  // @return next writeable frame (assumes one exists)
  uint8_t *writeable_frame() {
    return frame_buffers_[write_ptr_];
  }

  void read() {
    read_ptr_ = (read_ptr_ + 1) % frames;
  }

  void written() {
    write_ptr_ = (write_ptr_ + 1) % frames;
  }

private:

  uint8_t frame_memory_[kFrameSize * frames] __attribute__ ((aligned (4)));
  uint8_t *frame_buffers_[frames];

  volatile size_t write_ptr_;
  volatile size_t read_ptr_;

  DISALLOW_COPY_AND_ASSIGN(FrameBuffer);
};

#endif // DRIVERS_FRAMEBUFFER_H_
