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

#ifndef TONNETZ_H_
#define TONNETZ_H_

#include "tonnetz_abstract_triad.h"

namespace tonnetz {
  enum ETransformType {
    TRANSFORM_NONE,
    TRANSFORM_P,
    TRANSFORM_L,
    TRANSFORM_R,
    TRANSFORM_N, // RLP
    TRANSFORM_S, // LPR
    TRANSFORM_H, // LPL
    TRANSFORM_LAST
  };

  static const char transform_names[TRANSFORM_LAST + 1] = {
    '*', 'P', 'L', 'R', 'N', 'S', 'H', '@'
  };

  static const char *transform_names_str[TRANSFORM_LAST + 1] = {
    "*", "P", "L", "R", "N", "S", "H", "@"
  };

  static struct transformation {
    size_t root_shift; // +1 = root -> third, +2 root -> fifth
    int offsets[abstract_triad::NOTES]; // root, third, fifth
  } transformations[TRANSFORM_LAST][2] = {
    { { 0, {  0,  0,  0 } }, { 0, {  0,  0,  0 } } }, // NONE
    { { 0, {  0, -1,  0 } }, { 0, {  0,  1,  0 } } }, // TRANSFORM_P
    { { 1, { -1,  0,  0 } }, { 2, {  0,  0,  1 } } }, // TRANSFORM_L
    { { 2, {  0,  0,  2 } }, { 1, { -2,  0,  0 } } }, // TRANSFORM_R
    { { 1, {  0,  1,  1 } }, { 2, { -1, -1,  0 } } }, // TRANSFORM_N
    { { 0, {  1,  0,  1 } }, { 0, { -1,  0, -1 } } }, // TRANSFORM_S
    { { 2, { -1, -1,  1 } }, { 1, { -1,  1,  1 } } }, // TRANSFORM_H
  };

  abstract_triad apply_transformation(ETransformType type, const abstract_triad &source) {

    const tonnetz::transformation &t = tonnetz::transformations[type][source.mode()];

    abstract_triad result = source;
    result.change_mode();
    result.apply_offsets(t.offsets);
    result.shift_root(t.root_shift);
    return result;
  }
};

#endif // TONNETZ_H_
