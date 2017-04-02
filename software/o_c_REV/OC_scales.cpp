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
    "16E2",
    "15E2",
    "14E2",
    "13E2",
    "11E2",
    "10E2",
    "9E2",
    "8E2",
    "7E2",
    "6E2",
    "5E2",
// #if BUCHLA_SUPPORT==true
    "B-Pe",
    "B-Pj",
    "B-Pl",
    "16E3",
    "15E3", 
    "14E3", 
    "12E3", 
    "11E3", 
    "10E3", 
    "9E3", 
    "8E3", 
    "7E3", 
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
    "16-ED2",
    "15-ED2",
    "14-ED2",
    "13-ED2",
    "11-ED2",
    "10-ED2",
    "9-ED2",
    "8-ED2",
    "7-ED2",
    "6-ED2",
    "5-ED2",
// #if BUCHLA_SUPPORT==true
    "Bohlen-Pierce equal",
    "Bohlen-Pierce just",
    "Bohlen-Pierce lambda",
    "16-ED3",
    "15-ED3", 
    "14-ED3", 
    "12-ED3", 
    "11-ED3", 
    "10-ED3", 
    "9-ED3", 
    "8-ED3", 
    "7-ED3", 
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
