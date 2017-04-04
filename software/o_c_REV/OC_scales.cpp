#include "OC_scales.h"
#include "braids_quantizer_scales.h"

namespace OC {

Scale user_scales[Scales::SCALE_USER_LAST];
Scale dummy_scale;

/*static*/
const int Scales::NUM_SCALES = OC::Scales::SCALE_USER_LAST + sizeof(braids::scales) / sizeof(braids::scales[0]);

/*static*/
void Scales::Init() {
  for (size_t i = 0; i < SCALE_USER_LAST; ++i)
    memcpy(&user_scales[i], &braids::scales[1], sizeof(Scale));
}

/*static*/
const Scale &Scales::GetScale(int index) {
  if (index < SCALE_USER_LAST)
    return user_scales[index];
  else
    return braids::scales[index - SCALE_USER_LAST];
}

const char* const scale_names_short[] = {
    "USER1",
    "USER2",
    "USER3",
    "USER4",
    "OFF ",
    "SEMI",
    "IONI",
    "DORI",
    "PHRY",
    "LYDI",
    "MIXO",
    "AEOL",
    "LOCR",
    "BLU+",
    "BLU-",
    "PEN+",
    "PEN-",
    "FOLK",
    "JAPA",
    "GAME",
    "GYPS",
    "ARAB",
    "FLAM",
    "WHOL",
    "PYTH",
    "EB/4",
    "E /4",
    "EA/4",
    "BHAI",
    "GUNA",
    "MARW",
    "SHRI",
    "PURV",
    "BILA",
    "YAMA",
    "KAFI",
    "BHIM",
    "DARB",
    "RAGE",
    "KHAM",
    "MIMA",
    "PARA",
    "RANG",
    "GANG",
    "KAME",
    "PAKA",
    "NATB",
    "KAUN",
    "BAIR",
    "BTOD",
    "CHAN",
    "KTOD",
    "JOGE",
    "SEVI",
    "16ED",
    "15ED",
    "14ED",
    "13ED",
    "11ED",
    "10ED",
    "9ED",
    "8ED",
    "7ED",
    "6ED",
    "5ED",
    "16H2",
    "15H2",
    "14H2",
    "13H2",
    "12H2",
    "11H2",
    "10H2",
    "9H2",
    "8H2",
    "7H2",
    "6H2",
    "5H2",
    "4H2",
    
// #if BUCHLA_SUPPORT==true
    "B-Pe",
    "B-Pj",
    "B-Pl",
    "16H3",
    "14H3",
    "12H3", 
    "10H3", 
    "8H3", 
    
// #endif    
    };

const char* const scale_names[] = {
    "User-defined 1",
    "User-defined 2",
    "User-defined 3",
    "User-defined 4",
    "Off ",
    "Semitone",
    "Ionian",
    "Dorian",
    "Phrygian",
    "Lydian",
    "Mixolydian",
    "Aeolian",
    "Locrian",
    "Blues major",
    "Blues minor",
    "Pentatonic maj",
    "Pentatonic min",
    "Folk",
    "Japanese",
    "Gamelan",
    "Gypsy",
    "Arabian",
    "Flamenco",
    "Whole tone",
    "Pythagorean",
    "EB/4",
    "E /4",
    "EA/4",
    "Bhairav",
    "Gunakri",
    "Marwa",
    "Shree [Camel]",
    "Purvi",
    "Bilawal",
    "Yaman",
    "Kafi",
    "Bhimpalasree",
    "Darbari",
    "Rageshree",
    "Khamaj",
    "Mimal",
    "Parameshwari",
    "Rangeshwari",
    "Gangeshwari",
    "Kameshwari",
    "Pa Khafi",
    "Natbhairav",
    "Malkauns",
    "Bairagi",
    "B Todi",
    "Chandradeep",
    "Kaushik Todi",
    "Jogeshwari",
    "Sevish 12on31-EDO",
    "16-ED (2 or 3)",
    "15-ED (2 or 3)",
    "14-ED (2 or 3)",
    "13-ED (2 or 3)",
    "11-ED (2 or 3)",
    "10-ED (2 or 3)",
    "9-ED (2 or 3)",
    "8-ED (2 or 3)",
    "7-ED (2 or 3)",
    "6-ED2",
    "5-ED2",
    "16-HD2",
    "15-HD2",
    "14-HD2",
    "13-HD2",
    "12-HD2",
    "11-HD2",
    "10-HD2",
    "9-HD2",
    "8-HD2",
    "7-HD2",
    "6-HD2",
    "5-HD2",
    "4-HD2",

// #if BUCHLA_SUPPORT==true
    "Bohlen-Pierce equal",
    "Bohlen-Pierce just",
    "Bohlen-Pierce lambda",
    "8-24-HD3 (16 step)",
    "7-21-HD3 (14 step)",
    "6-18-HD3 (12 step)", 
    "5-15-HD3 (10 step)", 
    "4-12-HD3 (8 step)", 
     
// #endif
    };

const char* const voltage_scalings[] = {
    "1", // 1V/octave
    "1.2", // 1.2V/octave Buchla
    "2", // 2V/oct Buchla
    "a", // Wendy Carlos alpha scale
    "b", // Wendy Carlos beta scale
    "g", // Wendy Carlos gamma scale
    "tri" // Tritave (as used by Bohlen-Pierce macrotonal scale)
    } ;

}; // namespace OC
