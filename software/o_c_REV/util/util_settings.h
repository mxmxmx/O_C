// Copyright (c) 2016 Patrick Dowling
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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

namespace settings {

enum StorageType {
  STORAGE_TYPE_U4, // nibbles are packed where possible, else aligned to next byte
  STORAGE_TYPE_I8, STORAGE_TYPE_U8,
  STORAGE_TYPE_I16, STORAGE_TYPE_U16,
  STORAGE_TYPE_I32, STORAGE_TYPE_U32,
};

struct value_attr {
  const int default_;
  const int min_;
  const int max_;
  const char *name;
  const char * const *value_names;
  StorageType storage_type;

  int default_value() const {
    return default_;
  }

  int clamp(int value) const {
    if (value < min_) return min_;
    else if (value > max_) return max_;
    else return value;
  }
};

// Provide a very simple "settings" base.
// Settings values are an array of ints that are accessed by index, usually the
// owning class will use an enum for clarity, and provide specific getter
// functions for each value.
//
// The values are decsribed using the per-class value_attr array and allow min,
// max and default values. To set the defaults, call ::InitDefaults at least
// once before using the class. The min/max values are enforced when setting
// or modifying values. Classes shouldn't normally have to access the values_
// directly.
//
// To try and save some storage space, each setting can be stored as a smaller
// type as specified in the attributes. For even more compact representations,
// the owning class can pack things differently if required.
//
// TODO: Save/Restore is still kind of sucky
// TODO: If absolutely necessary, add STORAGE_TYPE_BIT and pack nibbles & bits
//
template <typename clazz, size_t num_settings>
class SettingsBase {
public:

  int get_value(size_t index) const {
    return values_[index];
  }

  static int clamp_value(size_t index, int value) {
    return value_attr_[index].clamp(value);
  }

  bool apply_value(size_t index, int value) {
    if (index < num_settings) {
      const int clamped = value_attr_[index].clamp(value);
      if (values_[index] != clamped) {
        values_[index] = clamped;
        return true;
      }
    }
    return false;
  }

  bool change_value(size_t index, int delta) {
    return apply_value(index, values_[index] + delta);
  }

  static const settings::value_attr &value_attr(size_t i) {
    return value_attr_[i];
  }

  void InitDefaults() {
    for (size_t s = 0; s < num_settings; ++s)
      values_[s] = value_attr_[s].default_value();
  }

  size_t Save(void *storage) const {
    nibbles_ = 0;
    uint8_t *write_ptr = static_cast<uint8_t *>(storage);
    for (size_t s = 0; s < num_settings; ++s) {
      switch(value_attr_[s].storage_type) {
        case STORAGE_TYPE_U4: write_ptr = write_nibble(write_ptr, s); break;
        case STORAGE_TYPE_I8: write_ptr = write_setting<int8_t>(write_ptr, s); break;
        case STORAGE_TYPE_U8: write_ptr = write_setting<uint8_t>(write_ptr, s); break;
        case STORAGE_TYPE_I16: write_ptr = write_setting<int16_t>(write_ptr, s); break;
        case STORAGE_TYPE_U16: write_ptr = write_setting<uint16_t>(write_ptr, s); break;
        case STORAGE_TYPE_I32: write_ptr = write_setting<int32_t>(write_ptr, s); break;
        case STORAGE_TYPE_U32: write_ptr = write_setting<uint32_t>(write_ptr, s); break;
      }
    }
    return (size_t)(write_ptr - static_cast<uint8_t *>(storage));
  }

  size_t Restore(const void *storage) {
    nibbles_ = 0;
    const uint8_t *read_ptr = static_cast<const uint8_t *>(storage);
    for (size_t s = 0; s < num_settings; ++s) {
      switch(value_attr_[s].storage_type) {
        case STORAGE_TYPE_U4: read_ptr = read_nibble(read_ptr, s); break;
        case STORAGE_TYPE_I8: read_ptr = read_setting<int8_t>(read_ptr, s); break;
        case STORAGE_TYPE_U8: read_ptr = read_setting<uint8_t>(read_ptr, s); break;
        case STORAGE_TYPE_I16: read_ptr = read_setting<int16_t>(read_ptr, s); break;
        case STORAGE_TYPE_U16: read_ptr = read_setting<uint16_t>(read_ptr, s); break;
        case STORAGE_TYPE_I32: read_ptr = read_setting<int32_t>(read_ptr, s); break;
        case STORAGE_TYPE_U32: read_ptr = read_setting<uint32_t>(read_ptr, s); break;
      }
    }
    return (size_t)(read_ptr - static_cast<const uint8_t *>(storage));
  }

  static size_t storageSize() {
    return storage_size_;
  }

protected:

  int values_[num_settings];
  static const settings::value_attr value_attr_[];
  static const size_t storage_size_;

  mutable uint32_t nibbles_;

  uint8_t *flush_nibbles(uint8_t *dest) const {
    *dest++ = (nibbles_ & 0xff);
    nibbles_ = 0;
    return dest;
  }

  uint8_t *write_nibble(uint8_t *dest, size_t index) const {
    nibbles_ = (nibbles_ << 4) | (values_[index] & 0x0f);
    if (nibbles_ & 0xf0000000) {
      dest = flush_nibbles(dest);
    } else {
      nibbles_ |= 0x0f0000000;
    }
    return dest;
  }

  template <typename storage_type>
  uint8_t *write_setting(uint8_t *dest, size_t index) const {
    if (nibbles_)
      dest = flush_nibbles(dest);
    storage_type *storage = reinterpret_cast<storage_type *>(dest);
    *storage++ = values_[index];
    return reinterpret_cast<uint8_t *>(storage);
  }

  const uint8_t *read_nibble(const uint8_t *src, size_t index) {
    uint8_t value;
    if (nibbles_) {
      value = nibbles_ & 0x0f;
      nibbles_ = 0;
    } else {
      nibbles_ = *src++;
      value = (nibbles_ & 0xf0) >> 4;
      nibbles_ = nibbles_ | 0xf0000000;
    }
    apply_value(index, value);

    return src;
  }

  template <typename storage_type>
  const uint8_t *read_setting(const uint8_t *src, size_t index) {
    nibbles_ = 0;
    const storage_type *storage = reinterpret_cast<const storage_type*>(src);
    apply_value(index, *storage++);
    return reinterpret_cast<const uint8_t *>(storage);
  }

  static size_t calc_storage_size() {
    size_t s = 0;
    unsigned nibbles = 0;
    for (auto attr : value_attr_) {
      if (STORAGE_TYPE_U4 == attr.storage_type) {
        ++nibbles;
      } else {
        if (nibbles & 1) ++nibbles;
        switch(attr.storage_type) {
          case STORAGE_TYPE_I8: s += sizeof(int8_t); break;
          case STORAGE_TYPE_U8: s += sizeof(uint8_t); break;
          case STORAGE_TYPE_I16: s += sizeof(int16_t); break;
          case STORAGE_TYPE_U16: s += sizeof(uint16_t); break;
          case STORAGE_TYPE_I32: s += sizeof(int32_t); break;
          case STORAGE_TYPE_U32: s += sizeof(uint32_t); break;
          default: break;
        }
      }
    }
    if (nibbles & 1) ++nibbles;
    s += nibbles >> 1;
    return s;
  }
};

#define SETTINGS_DECLARE(clazz, last) \
template <> const size_t settings::SettingsBase<clazz, last>::storage_size_ = settings::SettingsBase<clazz, last>::calc_storage_size(); \
template <> const settings::value_attr settings::SettingsBase<clazz, last>::value_attr_[] =

}; // namespace settings

#endif // SETTINGS_H_
