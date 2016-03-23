#ifndef OC_STRINGS_H_
#define OC_STRINGS_H_

namespace OC {

  namespace Strings {
    extern const char * const note_names[];
    extern const char * const note_names_unpadded[];
    extern const char * const trigger_input_names[];
    extern const char * const cv_input_names[];
    extern const char * const no_yes[];
  };
};

inline
const char *note_name(int note) {
  return OC::Strings::note_names[(note + 120) % 12];
}

#endif // OC_STRINGS_H_
