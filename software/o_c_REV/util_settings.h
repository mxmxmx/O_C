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

  void init_defaults() {
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

protected:

  int values_[num_settings];
  static const settings::value_attr value_attr_[];
};

}; // namespace settings

#endif // SETTINGS_H_

