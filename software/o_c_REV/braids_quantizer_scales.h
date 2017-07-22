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
  // { 12 << 7, 5, { 0, 143, 637, 899, 1042} },
  // marwa (From yarns source code)
  { 12 << 7, 6, { 0, 143, 494, 755, 1132, 1393} },
  // shree (From yarns source code)
  // { 12 << 7, 7, { 0, 115, 494, 755, 899, 1014, 1393} },
  // purvi (From yarns source code)
  // { 12 << 7, 7, { 0, 143, 494, 755, 899, 1042, 1393} },
  // bilawal (From yarns source code)
  // { 12 << 7, 7, { 0, 261, 494, 637, 899, 1160, 1393} },
  // yaman (From yarns source code)
  { 12 << 7, 7, { 0, 261, 522, 783, 899, 1160, 1421} },
  // kafi (From yarns source code)
  // { 12 << 7, 7, { 0, 233, 376, 637, 899, 1132, 1275} },
  // bhimpalasree (From yarns source code)
  { 12 << 7, 7, { 0, 261, 404, 637, 899, 1160, 1303} },
  // darbari (From yarns source code)
  // { 12 << 7, 7, { 0, 261, 376, 637, 899, 1014, 1275} },
  // rageshree (From yarns source code)
  // { 12 << 7, 7, { 0, 261, 494, 637, 899, 1132, 1275} },
  // khamaj (From yarns source code)
  // { 12 << 7, 8, { 0, 261, 494, 637, 899, 1160, 1275, 1421} },
  // mimal (From yarns source code)
  // { 12 << 7, 8, { 0, 261, 376, 637, 899, 1132, 1275, 1393} },
  // parameshwari (From yarns source code)
  // { 12 << 7, 6, { 0, 115, 376, 637, 1132, 1275} },
  // rangeshwari (From yarns source code)
  // { 12 << 7, 6, { 0, 261, 376, 637, 899, 1393} },
  // gangeshwari (From yarns source code)
  // { 12 << 7, 6, { 0, 494, 637, 899, 1014, 1275} },
  // kameshwari (From yarns source code)
  // { 12 << 7, 6, { 0, 261, 755, 899, 1132, 1275} },
  // pa__kafi (From yarns source code)
  // { 12 << 7, 7, { 0, 261, 376, 637, 899, 1160, 1275} },
  // natbhairav (From yarns source code)
  // { 12 << 7, 7, { 0, 261, 494, 637, 899, 1014, 1393} },
  // m_kauns (From yarns source code)
  // { 12 << 7, 6, { 0, 261, 522, 637, 1014, 1275} },
  // bairagi (From yarns source code)
  { 12 << 7, 5, { 0, 115, 637, 899, 1275} },
  // b_todi (From yarns source code)
  // { 12 << 7, 5, { 0, 115, 376, 899, 1275} },
  // chandradeep (From yarns source code)
  // { 12 << 7, 5, { 0, 376, 637, 899, 1275} },
  // kaushik_todi (From yarns source code)
  // { 12 << 7, 5, { 0, 376, 637, 755, 1014} },
  // jogeshwari (From yarns source code)
  // { 12 << 7, 6, { 0, 376, 494, 637, 1132, 1275} },

  // Tartini-Vallotti [12] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 12, { 0, 120, 251, 381, 502, 643, 758, 893, 1019,  1144,  1280,  1395} },
  // 13 out of 22-tET, generator = 5 [13] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 13, { 0, 140, 279, 419, 489, 628, 768, 838, 977, 1117,  1187,  1327,  1466} },
  // 13 out of 19-tET, Mandelbaum [13] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 13, { 0, 162, 243, 404, 485, 647, 728, 889, 970, 1132,  1213,  1374,  1455} },
  // Magic[16] in 145-tET [16] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 16, { 0, 191, 265, 339, 413, 487, 561, 752, 826, 900, 975, 1049,  1239,  1314,  1388,  1462} },
  // g=9 steps of 139-tET. Gene Ward Smith "Quartaminorthirds" 7-limit temperament [16] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 16, { 0, 99,  199, 298, 398, 497, 597, 696, 796, 895, 995, 1094,  1193,  1293,  1392,  1492} },
  // Armodue semi-equalizzato [16] - from the Huygens-Fokker Scala scale archive - see http://www.huygens-fokker.org/scala/downloads.html#scales
  { 12 << 7, 16, { 0, 99,  198, 297, 396, 495, 595, 694, 793, 892, 991, 1090,  1189,  1239,  1338,  1437} },

  // Hirajoshi[5] - from Sevish World Scales Pack at http://sevish.com/music-resources
  { 12 << 7, 5, { 0,  237, 431, 874, 1011} },
  // Scottish bagpipes[7] - from Sevish World Scales Pack at http://sevish.com/music-resources
  { 12 << 7, 7, { 0,  252, 436, 634, 900, 1092,  1292} },
  // Thai ranat[7] - from Sevish World Scales Pack at http://sevish.com/music-resources
  { 12 << 7, 7, { 0,  206, 443, 673, 878, 1103,  1317} },

  // Sevish quasi-12-equal mode from 31-EDO - see http://sevish.com/2017/mapping-microtonal-scales-keyboard-scala/
  { 12 << 7, 12, { 0, 149, 297, 396, 545, 644, 793, 941, 1041,  1189,  1288,  1437} },
  // 11 TET Machine[6] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 6, { 0,  279, 559, 698, 977, 1257} },
  // 13 TET Father[8] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 8, { 0,  236, 473, 591, 827, 1063,  1182,  1418} },
  // 15 TET Blackwood[10] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 10, { 0,  205, 307, 512, 614, 819, 922, 1126,  1229,  1434} },
  // 16 TET Mavila[7] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 7, { 0,  192, 384, 672, 864, 1056,  1248} },
  // 16 TET Mavila[9] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 9, { 0,  96,  288, 480, 672, 768, 960, 1152,  1344} },
  // 17 TET Superpyth[12] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 12, { 0, 90,  181, 361, 452, 632, 723, 813, 994, 1084,  1265,  1355} },

  // 22 TET Orwell[9] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 9, { 0, 140, 349, 489, 698, 838, 1047,  1187,  1396} },
  // 22 TET Pajara[10] Static Symmetrical Maj - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 10, { 0, 140, 279, 489, 628, 768, 908, 1047,  1257,  1396} },
  // 22 TET Pajara[10] Std Pentachordal Maj - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 10, { 0, 140, 279, 489, 628, 768, 908, 1117,  1257,  1396} },
  // 22 TET Porcupine[7] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 7, { 0,  209, 419, 628, 908, 1117,  1327} },
  // 26 TET Flattone[12] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 12, { 0, 59,  236, 295, 473, 650, 709, 886, 945, 1122,  1182,  1359} },
  // 26 TET Lemba[10] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 10, { 0, 177, 295, 473, 591, 768, 945, 1063,  1241,  1359} },
  // 46 TET Sensi[11] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 11, { 0,  167, 334, 501, 568, 735, 902, 1069,  1135,  1302,  1469} },
  // 53 TET Orwell[9] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 9, { 0,  145, 348, 493, 696, 840, 1043,  1188,  1391} },
  // 12 out of 72-TET scale by Prent Rodgers - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 12, { 0, 256, 341, 491, 555, 640, 704, 896, 1131,  1237,  1344,  1387} },
  // Trivalent scale in zeus temperament[7]; thirds are all {7/6, 6/5, 5/4}; 99et tuning; - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 7, { 0, 202, 496, 698, 900, 1195,  1396} },
  // 202 TET tempering of octone[8] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 8, { 0,  152, 449, 494, 791, 897, 1194,  1239} },
  // 313 TET elfmadagasgar[9] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 9, { 0,  260, 319, 579, 638, 898, 957, 1217,  1276} },
  // Marvel woo version of glumma[12] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 12, { 0, 63,  298, 406, 491, 703, 789, 897, 1131,  1194,  1239,  1492} },
  // TOP Parapyth[12] - from Sevish Regular Temperaments Pack at http://sevish.com/music-resources
  { 12 << 7, 12, { 0, 75,  265, 340, 530, 605, 710, 901, 975, 1166,  1240,  1431} },
  
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

  // 32-16-SD2 (16 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 16, { 0, 70,  143, 218, 296, 376, 460, 547, 637, 732, 830, 933, 1042,  1155,  1275,  1402} },
  // 30-15-SD2 (15 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 15, { 0, 75,  153, 233, 317, 404, 494, 589, 687, 790, 899, 1012,  1132,  1259,  1393} },
  // 28-14-SD2 (14 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 14, { 0, 81,  164, 251, 342, 436, 534, 637, 746, 859, 979, 1106,  1240,  1383,  1536} },
  // 26-13-SD2 (13 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 13, { 0, 87,  177, 272, 370, 473, 581, 695, 815, 942, 1076,  1219,  1372} },
  // 24-12-SD2 (12 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 12, { 0, 94,  193, 296, 404, 518, 637, 764, 899, 1042,  1194,  1359} },
  // 22-11-SD2 (11 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 11, { 0, 103, 211, 325, 445, 571, 706, 849, 1002,  1166,  1343} },
  // 20-10-SD2 (10 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 10, { 0, 114, 233, 360, 494, 637, 790, 955, 1132,  1325} },
  // 18-9-SD2 (9 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 9, { 0,  127, 261, 404, 557, 721, 899, 1091,  1303} },
  // 16-8-SD2 (8 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 8, { 0,  143, 296, 460, 637, 830, 1042,  1275} },
  // 14-7-SD2 (7 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 7, { 0,  164, 342, 534, 746, 979, 1240} },
  // 12-6-SD2 (6 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 6, { 0,  193, 404, 637, 899, 1194} },
  // 10-5-SD2 (5 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 5, { 0,  233, 494, 790, 1132} },
  // 8-4-SD2 (4 step subharmonic series scale on the octave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
  { 12 << 7, 4, { 0,  296, 637, 1042} },
  
  // Bohlen-Pierce (equal) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 118, 236, 354, 473, 591, 709, 827, 945, 1063,  1182,  1300,  1418} },
  // Bohlen-Pierce (just) - see http://ziaspace.com/NYU/BP-Scale_research.pdf and https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 13, { 0, 108, 244, 351, 470, 595, 714, 822, 941, 1066,  1185,  1292,  1428} },
  // Bohlen-Pierce (lambda) - see  https://en.wikipedia.org/wiki/Bohlen–Pierce_scale
  { 12 << 7, 9, { 0, 244, 351, 470, 714, 822, 1066, 1185, 1428} },

//  // 8-24-HD3 (16 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 16, { 0, 165, 312, 445, 567, 679, 782, 879, 969, 1054,  1134,  1209,  1281,  1349,  1414,  1476} },
//  // 7-21-HD3 (14 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 14, { 0, 187, 351, 499, 632, 754, 865, 969, 1066,  1156,  1241,  1320,  1396,  1468} },
//  // 6-18-HD3 (12 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 12, { 0, 216, 402, 567, 714, 847, 969, 1081,  1185,  1281,  1371,  1456} },
//  // 5-15-HD3 (10 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 10, { 0, 255, 470, 657, 822, 969, 1102,  1224,  1336,  1440} },
//  // 4-12-HD3 (8 step harmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 8, { 0,  312, 567, 782, 969, 1134,  1281,  1414} },
//
//  // 24-8-HD3 (16 step subharmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 16, { 0, 60,  122, 187, 255, 327, 402, 482, 567, 657, 754, 857, 969, 1091,  1224,  1371} },
//  // 21-7-HD3 (14 step subharmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 14, { 0, 68,  140, 216, 295, 380, 470, 567, 671, 782, 904, 1037,  1185,  1349} },
//  // 18-6-HD3 (12 step subharmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 12, { 0, 80,  165, 255, 351, 455, 567, 689, 822, 969, 1134,  1320} },
//  // 15-5-HD3 (10 step subharmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 10, { 0, 96,  200, 312, 434, 567, 714, 879, 1066,  1281} },
//  // 12-4-HD3 (8 step subharmonic series scale on the tritave) - see Xen-Arts VSTi microtuning library at http://www.xen-arts.net/Xen-Arts%20VSTi%20Microtuning%20Library.zip
//  { 12 << 7, 8, { 0,  122, 255, 402, 567, 754, 969, 1224} }
  } ;
}// namespace braids

#endif  // BRAIDS_QUANTIZER_SCALES_H_
