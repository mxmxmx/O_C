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
