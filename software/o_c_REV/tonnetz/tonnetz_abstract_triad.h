// Copyright (c) 2015, 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
 * inversion functions become shifts and either root_index_ can be eliminated?
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
    root_index_ = 0;
  }

  void apply_offsets(const int *offsets) {
    for (size_t n = 0; n < NOTES; ++n)
      notes_[(root_index_ + n) % NOTES] += offsets[n];
  }

  void change_mode() {
    mode_ = static_cast<EMode>(MODE_MINOR - mode_);
  }

  void shift_root(int offset) {
    root_index_ = (root_index_ + offset) % NOTES;
  }


  void render(note_t root, int inversion, int *dest) const {

    // This can probably be made sleeker by computing things on the fly
    // and by using the funky util_math.h functions, but for now it's
    // Good Enough if it works

    int offsets[NOTES] = {0,0,0};
    size_t base_index = calc_inversion_offsets(inversion, offsets);

    for (size_t n = 0; n < NOTES; ++n) {
      const size_t index = (base_index + n) % NOTES;
      dest[n] = root + notes_[index] + offsets[index];
    }
  }

private:

  size_t root_index_;
  EMode mode_;
  note_t notes_[NOTES];

  // Calculate the inversion offsets and return the base note index
  // TODO I think this can be further simplified to a single division and modulo
  size_t calc_inversion_offsets(int inversion, note_t *offsets) const {
    size_t base_index = root_index_;
    if (!inversion) {
      offsets[0] = offsets[1] = offsets[2] = 0;
    } else if (inversion > 0) {
      offsets[base_index] += ((inversion + 2) / 3) * 12;
      offsets[(base_index + 1) % 3] += ((inversion + 1) / 3) * 12;
      offsets[(base_index + 2) % 3] += ((inversion + 0) / 3) * 12;
      base_index = (base_index + inversion) % NOTES;
    } else if (inversion < 0) {
      inversion = -inversion;
      offsets[(base_index + 2) % 3] -= ((inversion + 2) / 3) * 12;
      offsets[(base_index + 1) % 3] -= ((inversion + 1) / 3) * 12;
      offsets[base_index] -= ((inversion + 0) / 3) * 12;
      base_index = (base_index + 2 * inversion) % NOTES;
    }
    return base_index;
  }

};

#endif // ABSTRACT_TRIAD_H_
