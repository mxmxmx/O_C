// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Resources definitions.
//
// Automatically generated with:
// make resources


#ifndef PEAKS_RESOURCES_H_
#define PEAKS_RESOURCES_H_


//#include "stmlib/stmlib.h"
#include <stdint.h>


namespace peaks {

typedef uint8_t ResourceId;

extern const char* string_table[];

extern const uint16_t* lookup_table_table[];

extern const uint32_t* lookup_table_32_table[];

extern const uint16_t lut_delay_times[];
extern const uint16_t lut_gravity[];
extern const uint16_t lut_env_linear[];
extern const uint16_t lut_env_expo[];
extern const uint16_t lut_env_quartic[];
extern const uint16_t lut_env_sine[];
extern const uint16_t lut_env_plateau[];
extern const uint16_t lut_env_cliff[];
extern const uint16_t lut_env_big_dipper[];
extern const uint16_t lut_env_medium_dipper[];
extern const uint16_t lut_env_little_dipper[];
extern const uint16_t lut_env_sinefold[];
extern const uint16_t lut_raised_cosine[];
extern const uint16_t lut_svf_cutoff[];
extern const uint16_t lut_svf_damp[];
extern const uint16_t lut_svf_scale[];
extern const uint32_t lut_lfo_increments[];
extern const uint32_t lut_env_increments[];
extern const uint32_t lut_oscillator_increments[];
#define STR_DUMMY 0  // dummy
#define LUT_DELAY_TIMES 0
#define LUT_DELAY_TIMES_SIZE 257
#define LUT_GRAVITY 1
#define LUT_GRAVITY_SIZE 257
#define LUT_ENV_LINEAR 2
#define LUT_ENV_LINEAR_SIZE 257
#define LUT_ENV_EXPO 3
#define LUT_ENV_EXPO_SIZE 257
#define LUT_ENV_QUARTIC 4
#define LUT_ENV_QUARTIC_SIZE 257
#define LUT_ENV_SINE 5
#define LUT_ENV_SINE_SIZE 257
#define LUT_ENV_PLATEAU 6
#define LUT_ENV_PLATEAU_SIZE 257
#define LUT_ENV_CLIFF 7
#define LUT_ENV_CLIFF_SIZE 257
#define LUT_ENV_BIG_DIPPER 8
#define LUT_ENV_BIG_DIPPER_SIZE 257
#define LUT_ENV_MEDIUM_DIPPER 9
#define LUT_ENV_MEDIUM_DIPPER_SIZE 257
#define LUT_ENV_LITTLE_DIPPER 10
#define LUT_ENV_LITTLE_DIPPER_SIZE 257
#define LUT_ENV_SINEFOLD 11
#define LUT_ENV_SINEFOLD_SIZE 257
#define LUT_RAISED_COSINE 12
#define LUT_RAISED_COSINE_SIZE 257
#define LUT_SVF_CUTOFF 13
#define LUT_SVF_CUTOFF_SIZE 257
#define LUT_SVF_DAMP 14
#define LUT_SVF_DAMP_SIZE 257
#define LUT_SVF_SCALE 15
#define LUT_SVF_SCALE_SIZE 257
#define LUT_LFO_INCREMENTS 0
#define LUT_LFO_INCREMENTS_SIZE 257
#define LUT_ENV_INCREMENTS 1
#define LUT_ENV_INCREMENTS_SIZE 257
#define LUT_OSCILLATOR_INCREMENTS 2
#define LUT_OSCILLATOR_INCREMENTS_SIZE 97

}  // namespace peaks

#endif  // PEAKS_RESOURCES_H_
