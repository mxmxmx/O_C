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
  // Sevish quasi-12-equal mode from 31-EDO - see http://sevish.com/2017/mapping-microtonal-scales-keyboard-scala/
  { 12 << 7, 12, { 0, 149, 297, 396, 545, 644, 793, 941, 1041,  1189,  1288,  1437,  1536} },
  
  // 16-ED (ED2 or ED3) (16 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 16, { 0, 96,  192, 288, 384, 480, 576, 672, 768, 864, 960, 1056,  1152,  1248,  1344,  1440} },
  // 15-ED (ED2 or ED3) (15 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 15, { 0, 102, 205, 307, 410, 512, 614, 717, 819, 922, 1024,  1126,  1229,  1331,  1434} },
  // 14-ED (ED2 or ED3) (14 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 14, { 0, 110, 219, 329, 439, 549, 658, 768, 878, 987, 1097,  1207,  1317,  1426} },
  // 13-ED (ED2 or ED3) (13 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 13, { 0, 118, 236, 354, 473, 591, 709, 827, 945, 1063,  1182,  1300,  1418} },
  // 11-ED (ED2 or ED3) (11 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 11, { 0, 140, 279, 419, 559, 698, 838, 977, 1117,  1257,  1396} },
  // 10-ED (ED2 or ED3) (10 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 10, { 0, 154, 307, 461, 614, 768, 922, 1075,  1229,  1382} },
  // 9-ED (ED2 or ED3) (9 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 9, { 0,  171, 341, 512, 683, 853, 1024,  1195,  1365} },
  // 8-ED (ED2 or ED3) (8 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 8, { 0,  192, 384, 576, 768, 960, 1152,  1344} },
  // 7-ED (ED2 or ED3) (7 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 7, { 0,  219, 439, 658, 878, 1097,  1317} }, 
  // 6-ED (ED2 or ED3) (6 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 6, { 0,  256, 512, 768, 1024,  1280} }, 
  // 5-ED (ED2 or ED3) (5 step equally tempered scale on the octave or the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 5, { 0,  307, 614, 922, 1229} }, 

  // 16-HD2 (16 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 16, { 0,  134, 261, 381, 494, 603, 706, 804, 899, 989, 1076,  1160,  1240,  1318,  1393,  1466} },
  // 15-HD2 (15 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 15, { 0, 143, 277, 404, 524, 637, 746, 849, 947, 1042,  1132,  1219,  1303,  1383,  1461} },
  // 14-HD2 (14 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 14, { 0, 153, 296, 430, 557, 677, 790, 899, 1002,  1100,  1194,  1285,  1372,  1455} },
  // 13-HD2 (13 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 13, { 0, 164, 317, 460, 594, 721, 841, 955, 1063,  1166,  1264,  1359,  1449} },
  // 12-HD2 (12 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 12, { 0, 177, 342, 494, 637, 772, 899, 1018,  1132,  1240,  1343,  1442} },
  // 11-HD2 (11 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 11, { 0, 193, 370, 534, 687, 830, 965, 1091,  1211,  1325,  1433} },
  // 10-HD2 (10 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 10, { 0, 211, 404, 581, 746, 899, 1042,  1176,  1303,  1422} },
  // 9-HD2 (9 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 9, { 0,  233, 445, 637, 815, 979, 1132,  1275,  1409} },
  // 8-HD2 (8 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 8, { 0,  261, 494, 706, 899, 1076,  1240,  1393} },
  // 7-HD2 (7 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 7, { 0,  296, 557, 790, 1002,  1194,  1372} },
  // 6-HD2 (6 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 6, { 0,  342, 637, 899, 1132,  1343} },
  // 5-HD2 (5 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 5, { 0,  404, 746, 1042,  1303} },
  // 4-HD2 (4 step harmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 4, { 0,  494, 899, 1240} },

  
// #if BUCHLA_SUPPORT==true
  // Bohlen-Pierce (equal) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 118, 236, 354, 473, 591, 709, 827, 945, 1063,  1182,  1300,  1418} },
  // Bohlen-Pierce (just) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 108, 244, 351, 470, 595, 714, 822, 941, 1066,  1185,  1292,  1428} },
  // Bohlen-Pierce (lambda) - see  https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 9, { 0, 244, 351, 470, 714, 822, 1066, 1185, 1428} },

/* The following are redundant  
  // 16-ED3 (16 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 16, { 0, 96, 192, 288, 384, 480, 576, 672, 768, 864, 960, 1056, 1152, 1248, 1344, 1440} },
  // 15-ED3 (15 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 15, { 0,  102, 205, 307, 410, 512, 614, 717, 819, 922, 1024,  1126,  1229,  1331,  1434} },
  // 14-ED3 (14 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 14, { 0, 110, 219, 329, 439, 549, 658, 768, 878, 987, 1097,  1207,  1317,  1426} },
  // 12-ED3 (12 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 12, { 0, 128, 256, 384, 512, 640, 768, 896, 1024,  1152,  1280,  1408} },
  // 11-ED3 (11 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 11, { 0, 140, 279, 419, 559, 698, 838, 977, 1117,  1257,  1396} },
  // 10-ED3 (10 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 10, { 0, 154, 307, 461, 614, 768, 922, 1075,  1229,  1382} },
  // 9-ED3 (9 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 9, { 0,  171, 341, 512, 683, 853, 1024,  1195,  1365} },
  // 8-ED3 (8 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 8, { 0,  192, 384, 576, 768, 960, 1152,  1344} },
  // 7-ED3 (7 step equally tempered scale on the tritave) - see Xen-Arts VSTi microtuning library No Octaves Pack at http://sevish.com/music-resources/
  { 12 << 7, 7, { 0,  219, 439, 658, 878, 1097,  1317} },
*/

  // 8-24-HD3 (16 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 16, { 0, 165, 312, 445, 567, 679, 782, 879, 969, 1054,  1134,  1209,  1281,  1349,  1414,  1476} },
  // 7-21-HD3 (14 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 14, { 0, 187, 351, 499, 632, 754, 865, 969, 1066,  1156,  1241,  1320,  1396,  1468} },
  // 6-18-HD3 (12 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 12, { 0, 216, 402, 567, 714, 847, 969, 1081,  1185,  1281,  1371,  1456} },
  // 5-15-HD3 (10 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 10, { 0, 255, 470, 657, 822, 969, 1102,  1224,  1336,  1440} },
  // 4-12-HD3 (8 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 8, { 0,  312, 567, 782, 969, 1134,  1281,  1414} },
   
// #endif  
  } ;
}// namespace braids

#endif  // BRAIDS_QUANTIZER_SCALES_H_
