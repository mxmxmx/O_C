// Copyright 2015 Olivier Gillet.
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
// Quantizer scales

#ifndef BRAIDS_QUANTIZER_SCALES_H_
#define BRAIDS_QUANTIZER_SCALES_H_

#include "braids_quantizer.h"

namespace braids {

const Scale scales[] = {
  // Off
  { 0, 0, { } },
  // Semitones
  { 12 << 7, 12, { 0, 128, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408} },
  // Ionian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 256, 512, 640, 896, 1152, 1408} },
  // Dorian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 256, 384, 640, 896, 1152, 1280} },
  // Phrygian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 128, 384, 640, 896, 1024, 1280} },
  // Lydian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 256, 512, 768, 896, 1152, 1408} },
  // Mixolydian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 256, 512, 640, 896, 1152, 1280} },
  // Aeolian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 256, 384, 640, 896, 1024, 1280} },
  // Locrian (From midipal/BitT source code)
  { 12 << 7, 7, { 0, 128, 384, 640, 768, 1024, 1280} },
  // Blues major (From midipal/BitT source code)
  { 12 << 7, 6, { 0, 384, 512, 896, 1152, 1280} },
  // Blues minor (From midipal/BitT source code)
  { 12 << 7, 6, { 0, 384, 640, 768, 896, 1280} },
  // Pentatonic major (From midipal/BitT source code)
  { 12 << 7, 5, { 0, 256, 512, 896, 1152} },
  // Pentatonic minor (From midipal/BitT source code)
  { 12 << 7, 5, { 0, 384, 640, 896, 1280} },
  // Folk (From midipal/BitT source code)
  { 12 << 7, 8, { 0, 128, 384, 512, 640, 896, 1024, 1280} },
  // Japanese (From midipal/BitT source code)
  { 12 << 7, 5, { 0, 128, 640, 896, 1024} },
  // Gamelan (From midipal/BitT source code)
  { 12 << 7, 5, { 0, 128, 384, 896, 1024} },
  // Gypsy
  { 12 << 7, 7, { 0, 256, 384, 768, 896, 1024, 1408} }, 
  // Arabian
  { 12 << 7, 7, { 0, 128, 512, 640, 896, 1024, 1408} }, 
  // Flamenco
  { 12 << 7, 7, { 0, 128, 512, 640, 896, 1024, 1280} },
  // Whole tone (From midipal/BitT source code)
  { 12 << 7, 6, { 0, 256, 512, 768, 1024, 1280} },
  // pythagorean (From yarns source code)
  { 12 << 7, 12, { 0, 115, 261, 376, 522, 637, 783, 899, 1014, 1160, 1275, 1421} },
  // 1_4_eb (From yarns source code)
  { 12 << 7, 12, { 0, 128, 256, 384, 448, 640, 768, 896, 1024, 1152, 1280, 1344} },
  // 1_4_e (From yarns source code)
  { 12 << 7, 12, { 0, 128, 256, 384, 448, 640, 768, 896, 1024, 1152, 1280, 1408} },
  // 1_4_ea (From yarns source code)
  { 12 << 7, 12, { 0, 128, 256, 384, 448, 640, 768, 896, 1024, 1088, 1280, 1408} },
  // bhairav (From yarns source code)
  { 12 << 7, 7, { 0, 115, 494, 637, 899, 1014, 1393} },
  // gunakri (From yarns source code)
  { 12 << 7, 5, { 0, 143, 637, 899, 1042} },
  // marwa (From yarns source code)
  { 12 << 7, 6, { 0, 143, 494, 755, 1132, 1393} },
  // shree (From yarns source code)
  { 12 << 7, 7, { 0, 115, 494, 755, 899, 1014, 1393} },
  // purvi (From yarns source code)
  { 12 << 7, 7, { 0, 143, 494, 755, 899, 1042, 1393} },
  // bilawal (From yarns source code)
  { 12 << 7, 7, { 0, 261, 494, 637, 899, 1160, 1393} },
  // yaman (From yarns source code)
  { 12 << 7, 7, { 0, 261, 522, 783, 899, 1160, 1421} },
  // kafi (From yarns source code)
  { 12 << 7, 7, { 0, 233, 376, 637, 899, 1132, 1275} },
  // bhimpalasree (From yarns source code)
  { 12 << 7, 7, { 0, 261, 404, 637, 899, 1160, 1303} },
  // darbari (From yarns source code)
  { 12 << 7, 7, { 0, 261, 376, 637, 899, 1014, 1275} },
  // rageshree (From yarns source code)
  { 12 << 7, 7, { 0, 261, 494, 637, 899, 1132, 1275} },
  // khamaj (From yarns source code)
  { 12 << 7, 8, { 0, 261, 494, 637, 899, 1160, 1275, 1421} },
  // mimal (From yarns source code)
  { 12 << 7, 8, { 0, 261, 376, 637, 899, 1132, 1275, 1393} },
  // parameshwari (From yarns source code)
  { 12 << 7, 6, { 0, 115, 376, 637, 1132, 1275} },
  // rangeshwari (From yarns source code)
  { 12 << 7, 6, { 0, 261, 376, 637, 899, 1393} },
  // gangeshwari (From yarns source code)
  { 12 << 7, 6, { 0, 494, 637, 899, 1014, 1275} },
  // kameshwari (From yarns source code)
  { 12 << 7, 6, { 0, 261, 755, 899, 1132, 1275} },
  // pa__kafi (From yarns source code)
  { 12 << 7, 7, { 0, 261, 376, 637, 899, 1160, 1275} },
  // natbhairav (From yarns source code)
  { 12 << 7, 7, { 0, 261, 494, 637, 899, 1014, 1393} },
  // m_kauns (From yarns source code)
  { 12 << 7, 6, { 0, 261, 522, 637, 1014, 1275} },
  // bairagi (From yarns source code)
  { 12 << 7, 5, { 0, 115, 637, 899, 1275} },
  // b_todi (From yarns source code)
  { 12 << 7, 5, { 0, 115, 376, 899, 1275} },
  // chandradeep (From yarns source code)
  { 12 << 7, 5, { 0, 376, 637, 899, 1275} },
  // kaushik_todi (From yarns source code)
  { 12 << 7, 5, { 0, 376, 637, 755, 1014} },
  // jogeshwari (From yarns source code)
  { 12 << 7, 6, { 0, 376, 494, 637, 1132, 1275} },
// #if BUCHLA_SUPPORT==true
  // Bohlen-Pierce (equal) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 118, 236, 354, 473, 591, 709, 827, 945, 1063,  1182,  1300,  1418} },
  // Bohlen-Pierce (just) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 108, 244, 351, 470, 595, 714, 822, 941, 1066,  1185,  1292,  1428} },
  // Bohlen-Pierce (lambda) - see  https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 9, { 0, 244, 351, 470, 714, 822, 1066, 1185, 1428} },
  // 16-ED3 (16 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 16, { 0, 96, 192, 288, 384, 480, 576, 672, 768, 864, 960, 1056, 1152, 1248, 1344, 1440} },
  // 15-ED3 (15 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 15, { 0,  102, 205, 307, 410, 512, 614, 717, 819, 922, 1024,  1126,  1229,  1331,  1434} },
  // 14-ED3 (14 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 14, { 0, 110, 219, 329, 439, 549, 658, 768, 878, 987, 1097,  1207,  1317,  1426} },
  // 12-ED3 (12 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 12, { 0, 128, 256, 384, 512, 640, 768, 896, 1024,  1152,  1280,  1408} },
  // 11-ED3 (11 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 11, { 0, 140, 279, 419, 559, 698, 838, 977, 1117,  1257,  1396} },
  // 10-ED3 (10 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 10, { 0, 154, 307, 461, 614, 768, 922, 1075,  1229,  1382} },
  // 9-ED3 (9 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 9, { 0,  171, 341, 512, 683, 853, 1024,  1195,  1365} },
  // 8-ED3 (8 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 8, { 0,  192, 384, 576, 768, 960, 1152,  1344} },
  // 7-ED3 (7 step equally tempered scale on the tritave) - see No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 7, { 0,  219, 439, 658, 878, 1097,  1317} }, 
// #endif  
  } ;
}// namespace braids

#endif  // BRAIDS_QUANTIZER_SCALES_H_
