#include "OC_chords.h"
#include "OC_chords_presets.h"

namespace OC {

    Chord user_chords[Chords::CHORDS_USER_LAST];
    Chord dummy_chord;

    /*static*/
    const int Chords::NUM_CHORDS = OC::Chords::CHORDS_USER_LAST; // = 8
    const int Chords::NUM_CHORDS_PROPERTIES = sizeof(Chord);

    /*static*/
    // 
    void Chords::Init() {
      for (size_t i = 0; i < OC::Chords::CHORDS_USER_LAST; ++i)
        memcpy(&user_chords[i], &OC::chords[0], sizeof(Chord));
    }

    const Chord &Chords::GetChord(int index) {
       if (index < CHORDS_USER_LAST) 
        return user_chords[index];
       else
        return user_chords[0x0];
    }
} // namespace OC