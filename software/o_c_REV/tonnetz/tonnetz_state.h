#ifndef TONNETZ_STATE_H_
#define TONNETZ_STATE_H_
#include "tonnetz_abstract_triad.h"
#include "tonnetz.h"

class TonnetzState {
public:

  void init() {
    reset(MODE_MAJOR);
    history_ = 0;
  }

  int outputs(size_t index) const {
    return outputs_[index];
  }

  int root() const {
    return outputs_[0];
  }

  void reset(EMode mode) {
    current_chord_.init(mode);
    push_history(tonnetz::TRANSFORM_NONE, mode);
  }

  void apply_transformation(tonnetz::ETransformType transform) {
    current_chord_ = tonnetz::apply_transformation(transform, current_chord_);
    if (tonnetz::TRANSFORM_NONE != transform)
      push_history(transform, current_chord_.mode());
  }

  void render(int root, int inversion) {
    outputs_[0] = root;
    current_chord_.render(root, inversion, outputs_ + 1);
  }

  const abstract_triad &current_chord() const {
  	return current_chord_;
  }

  // Keep a "history" of transforms/chord mode using 4 x uint8_t; this makes it
  // atomic to get/set for ISR use
  uint32_t history() const {
    return history_;
  }

  void get_outputs(int *dest) const {
    size_t len = 4;
    const int *outputs = outputs_;
    while (len--)
      *dest++ = *outputs++;
  }

private:

  void push_history(tonnetz::ETransformType transform, EMode mode) {
    uint8_t entry = static_cast<uint8_t>(transform);
    if (MODE_MAJOR == mode)
      entry |= 0x80;
    history_ = (history_ << 8) | entry;
  }

  abstract_triad current_chord_;
  int outputs_[1 + abstract_triad::NOTES];

  uint32_t history_;
};

#endif // TONNETZ_STATE_H_
