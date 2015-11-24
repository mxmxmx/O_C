#ifndef TONNETZ_STATE_H_
#define TONNETZ_STATE_H_
#include "tonnetz_abstract_triad.h"
#include "tonnetz.h"

class TonnetzState {
public:
  void init() {
    reset(MODE_MAJOR);
  }

  int outputs(size_t index) const {
    return outputs_[index];
  }

  int root() const {
    return outputs_[0];
  }

  String last_trans() {
    return transform_indicator_;
  }

  void set_transform_indicator(char trans, int pos) {
    transform_indicator_[pos] = trans;
  }

  void reset_transform_indicator() {
    for (int i = 0; i < 3; ++i) {
      transform_indicator_[i] = ' ';
    }
  }


  void reset(EMode mode) {
    current_chord_.init(mode);
    current_chord_.generate_notes(outputs_[0], outputs_ + 1);
  }

  void render(int root, tonnetz::ETransformType transform, int inversion) {
    if (tonnetz::TRANSFORM_NONE != transform)
      current_chord_ = tonnetz::apply_transformation(transform, current_chord_);

    current_chord_.apply_inversion(inversion);

    outputs_[0] = root;
    current_chord_.generate_notes(root, outputs_ + 1);

  }


  const abstract_triad &current_chord() const {
  	return current_chord_;
  }

private:

  abstract_triad current_chord_;
  int outputs_[1 + abstract_triad::NOTES];
  char transform_indicator_[4] ;
};

#endif // TONNETZ_STATE_H_
