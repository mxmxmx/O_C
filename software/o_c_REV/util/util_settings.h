#ifndef SETTINGS_H_
#define SETTINGS_H_

namespace settings {

enum StorageType {
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

  size_t Save(void *dest) const {
    size_t written = 0;
    for (size_t s = 0; s < num_settings; ++s) {
      char *storage = static_cast<char *>(dest) + written;
      switch(value_attr_[s].storage_type) {
        case STORAGE_TYPE_I8: written += write_setting<int8_t>(storage, s); break;
        case STORAGE_TYPE_U8: written += write_setting<uint8_t>(storage, s); break;
        case STORAGE_TYPE_I16: written += write_setting<int16_t>(storage, s); break;
        case STORAGE_TYPE_U16: written += write_setting<uint16_t>(storage, s); break;
        case STORAGE_TYPE_I32: written += write_setting<int32_t>(storage, s); break;
        case STORAGE_TYPE_U32: written += write_setting<uint32_t>(storage, s); break;
      }
    }
    return written;
  }

  size_t Restore(const void *src) {
    size_t read = 0;
    for (size_t s = 0; s < num_settings; ++s) {
      const char *storage = static_cast<const char *>(src) + read;
      switch(value_attr_[s].storage_type) {
        case STORAGE_TYPE_I8: read += read_setting<int8_t>(storage, s); break;
        case STORAGE_TYPE_U8: read += read_setting<uint8_t>(storage, s); break;
        case STORAGE_TYPE_I16: read += read_setting<int16_t>(storage, s); break;
        case STORAGE_TYPE_U16: read += read_setting<uint16_t>(storage, s); break;
        case STORAGE_TYPE_I32: read += read_setting<int32_t>(storage, s); break;
        case STORAGE_TYPE_U32: read += read_setting<uint32_t>(storage, s); break;
      }
    }
    return read;
  }

  static size_t storageSize() {
    return storage_size_;
  }

protected:

  int values_[num_settings];
  static const settings::value_attr value_attr_[];
  static const size_t storage_size_;

  template <typename storage_type>
  size_t write_setting(void *dest, size_t index) const {
    storage_type *storage = reinterpret_cast<storage_type *>(dest);
    *storage = values_[index];
    return sizeof(storage_type);
  }

  template <typename storage_type>
  size_t read_setting(const void *src, size_t index) {
    const storage_type *storage = reinterpret_cast<const storage_type*>(src);
    apply_value(index, *storage);
    return sizeof(storage_type);
  }

  static size_t calc_storage_size() {
    size_t s = 0;
    for (auto attr : value_attr_)
      switch(attr.storage_type) {
        case STORAGE_TYPE_I8: s += sizeof(int8_t); break;
        case STORAGE_TYPE_U8: s += sizeof(uint8_t); break;
        case STORAGE_TYPE_I16: s += sizeof(int16_t); break;
        case STORAGE_TYPE_U16: s += sizeof(uint16_t); break;
        case STORAGE_TYPE_I32: s += sizeof(int32_t); break;
        case STORAGE_TYPE_U32: s += sizeof(uint32_t); break;
      }
    if (s & 1) ++s;
    return s;
  }
};

#define SETTINGS_DECLARE(clazz, last) \
template <> const size_t settings::SettingsBase<clazz, last>::storage_size_ = settings::SettingsBase<clazz, last>::calc_storage_size(); \
template <> const settings::value_attr settings::SettingsBase<clazz, last>::value_attr_[] =

}; // namespace settings

#endif // SETTINGS_H_

