#ifndef SETTINGS_H_
#define SETTINGS_H_

namespace settings {

struct value_attr {
  const int default_;
  const int min_;
  const int max_;
  const char *name;
  const char * const *value_names;

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
// In a vague attempt to "save space" in storage, save/restore functions can be
// typed to a smaller data type, or individual values saved as different types
// by hand.
//
// TODO Cleanup save/restore
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

  template <typename storage_type>
  size_t save(void *dest) const {
    storage_type *storage = static_cast<storage_type *>(dest);
    for (size_t s = 0; s < num_settings; ++s)
      *storage++ = static_cast<storage_type>(values_[s]);

    return num_settings * sizeof(storage_type);
  }

  template <typename storage_type>
  size_t restore(const void *src) {
    const storage_type *storage = static_cast<const storage_type *>(src);
    for (size_t s = 0; s < num_settings; ++s)
      apply_value(s, *storage++);

    return num_settings * sizeof(storage_type);
  }

  template <typename storage_type>
  char *write_setting(char *dest, size_t index) {
    storage_type *storage = reinterpret_cast<storage_type *>(dest);
    *storage++ = values_[index];
    return (char *)storage;
  }

  template <typename storage_type>
  char *read_setting(const char *src, size_t index) {
    const storage_type *storage = reinterpret_cast<const storage_type*>(src);
    apply_value(index, *storage++);
    return (char *)storage;
  }

protected:

  int values_[num_settings];
  static const settings::value_attr value_attr_[];
};

}; // namespace settings

#endif // SETTINGS_H_

