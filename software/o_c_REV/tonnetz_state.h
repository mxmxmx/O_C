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

    if (tonnetz::TRANSFORM_P == transform) transform_indicator_ = "P";
    else if (tonnetz::TRANSFORM_L == transform) transform_indicator_ = "L";
    else if (tonnetz::TRANSFORM_R == transform) transform_indicator_ = "R";
    else transform_indicator_ = " ";

    Serial.print("transform=");
    Serial.print(transform);
  }


  const abstract_triad &current_chord() const {
  	return current_chord_;
  }

private:

  abstract_triad current_chord_;
  int outputs_[1 + abstract_triad::NOTES];
  String transform_indicator_ ;
};

#endif // TONNETZ_STATE_H_
