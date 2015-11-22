#ifndef SETTINGS_H_
#define SETTINGS_H_

namespace settings {

struct value_attr {
  const int default_;
  const int min_;
  const int max_;
  const char *name;
  const char **value_names;

  int default_value() const {
    return default_;
  }

  int clamp(int value) const {
    if (value < min_) return min_;
    else if (value > max_) return max_;
    else return value;
  }
};

}; // namespace settings

#endif // SETTINGS_H_

