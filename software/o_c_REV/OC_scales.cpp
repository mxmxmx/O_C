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

const char* const scale_names[] = {
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
    "JOGE" };

}; // namespace OC
