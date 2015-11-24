#ifndef TONNETZ_STATE_H_
#define TONNETZ_STATE_H_
#include "tonnetz_abstract_triad.h"
#include "tonnetz.h"

class TonnetzState {
public:
  void init() {
    reset(MODE_MAJOR);
    last_transform_ = tonnetz::TRANSFORM_NONE;
  }

  int outputs(size_t index) const {
    return outputs_[index];
  }

  int root() const {
    return outputs_[0];
  }

  void reset(EMode mode) {
    current_chord_.init(mode);
    current_chord_.generate_notes(outputs_[0], outputs_ + 1);
    last_transform_ = tonnetz::TRANSFORM_NONE;
  }

  void render(int root, tonnetz::ETransformType transform, int inversion) {
    if (tonnetz::TRANSFORM_NONE != transform) {
      current_chord_ = tonnetz::apply_transformation(transform, current_chord_);
      last_transform_ = transform;
    }

    current_chord_.apply_inversion(inversion);

    outputs_[0] = root;
    current_chord_.generate_notes(root, outputs_ + 1);
  }

  const abstract_triad &current_chord() const {
  	return current_chord_;
  }

  tonnetz::ETransformType last_transform() const {
    return last_transform_;
  }

private:

  abstract_triad current_chord_;
  tonnetz::ETransformType last_transform_;
  int outputs_[1 + abstract_triad::NOTES];
};

#endif // TONNETZ_STATE_H_
