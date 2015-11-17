#ifndef ABSTRACT_TRIAD_H_
#define ABSTRACT_TRIAD_H_

#include <stddef.h>
#include <stdint.h>

typedef int note_t;

enum EMode {
  MODE_MAJOR
, MODE_MINOR
, MODE_LAST
};

/**
 * Compact representation of a triad that enseentially stores the intervals.
 * The root node is needed to be able to perform transformations.
 *
 * Since we're running on teensy with an M4, we should be able to pack all
 * the intervals as 4xint8_t and use the SIMD instructions. Then all the
 * inversion functions become shifts and either root_index_ or base_index_
 * can be eliminated.
 * 256/128 is still more than enough resolution in semitones
 */
class abstract_triad {
public:
  static const size_t NOTES = 3; // power of two would be better...

  EMode mode() const {
    return mode_;
  }

  abstract_triad() {
    init(MODE_MAJOR);
  }

  void init(EMode mode) {
    static const note_t root_chord_intervals_[MODE_LAST][NOTES] = {
      {0, 4, 7},
      {0, 3, 7}
    };

    mode_ = mode;
    for (size_t n = 0; n < NOTES; ++n)
      notes_[n] = root_chord_intervals_[mode_][n];
    root_index_ = base_index_ = 0;
  }

  void apply_offsets(const int *offsets) {
    for (size_t n = 0; n < NOTES; ++n)
      notes_[(root_index_ + n) % NOTES] += offsets[n];
  }

  void generate_notes(note_t root, int *dest) {
    for (size_t n = 0; n < NOTES; ++n)
      dest[n] = root + notes_[(base_index_ + n) % NOTES];
  }

  void change_mode() {
    mode_ = static_cast<EMode>(MODE_MINOR - mode_);
  }

  void shift_root(int offset) {
    root_index_ = (root_index_ + offset) % NOTES;
  }

  void apply_inversion(int inversion) {
/*
    if (inversion > 0) {
      notes_[base_index_] += inversion * 12;
      if (inversion > 1) notes_[(base_index_ + 1) % NOTES] += (inversion - 1) * 12;
      if (inversion > 2) notes_[(base_index_ + 2) % NOTES] += (inversion - 2) * 12;
      base_index_ = (base_index_ + inversion) & NOTES;
    } else if (inversion < 0) {
      inversion = -inversion;
      notes_[(base_index_ + 2) % NOTES] -= inversion * 12;
      if (inversion > 1) notes_[(base_index_ + 1) % NOTES] -= (inversion - 1) * 12;
      if (inversion > 2) notes_[base_index_] -= (inversion - 2) * 12;
      base_index_ = (base_index_ + 2 * inversion) % NOTES;
    }
*/
  }

private:

  size_t root_index_;
  size_t base_index_;
  EMode mode_;
  note_t notes_[NOTES];
};

#endif // ABSTRACT_TRIAD_H_
