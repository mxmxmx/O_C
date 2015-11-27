#ifndef TONNETZ_STATE_H_
#define TONNETZ_STATE_H_
#include "tonnetz_abstract_triad.h"
#include "tonnetz.h"

class TonnetzState {
public:

  static const size_t HISTORY_LENGTH = 4;

  struct HistoryEntry {
    char str[4];
  };

  void init() {
    reset(MODE_MAJOR);
    memset(history_, 0, sizeof(history_));
    history_tail_ = 0;
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

  const HistoryEntry &history(size_t index) const {
    return history_[(history_tail_ + HISTORY_LENGTH - index) % HISTORY_LENGTH];
  }

private:

  void push_history(tonnetz::ETransformType transform, EMode mode) {
    size_t tail = (history_tail_ + 1) % HISTORY_LENGTH;
    history_[tail].str[0] = MODE_MAJOR == mode ? '+' : '-';
    history_[tail].str[1] = tonnetz::transform_names[transform];
    history_tail_ = tail;
  }

  abstract_triad current_chord_;
  int outputs_[1 + abstract_triad::NOTES];
  HistoryEntry history_[HISTORY_LENGTH];
  size_t history_tail_;
};

#endif // TONNETZ_STATE_H_
