#ifndef OC_CHORDS_PRESETS_H_
#define OC_CHORDS_PRESETS_H_

#include "OC_scales.h"
#include <Arduino.h>

namespace OC {

  struct Chord {
   
    int8_t quality;
    int8_t inversion;
    int8_t voicing;
    int8_t parallel_scale;
  };


  const Chord chords[] = {
    // default
    { 0, 0, 0, OC::Scales::SCALE_SEMI}
  };

  const int8_t qualities[][4] = 
  {
  	{ 0, 4, 7, 11}, // major 7
  	{ 0, 3, 7, 10}, // minor 7
  	{ 0, 4, 7, 10}, // dominant 7
  	{ 0, 3, 6, 10}, // half diminished
  	{ 0, 4, 7, 0 }, // major triad
  	{ 0, 3, 7, 0 }, // minor triad
  	{ 0, 4, 7, 0 }, // dominant 
  	{ 0, 3, 6, 0 }, // diminished triad
  	{ 0, 0, 0, 0 }, // unisono
  };

  const int8_t voicing[][4] = 
  {
  	// this can't work like this, because it should operate on the inversions, too.
  	{ 0, 0, 0, 0 },  // close
  	{ 0, 0, 0, -1},  // drop 1 ??
  	{ 0, 0, -1, 0},  // drop 2
  	{ 0, -1, 0, 0},  // drop 3
  	{ 0, 0, 0, 1 },  // antidrop 1 ??
  	{ 0, 0, 1, 0 },  // antidrop 2 ??
  	{ 0, 1, 0, 0 },  // antidrop 3
  	{ -1, 1, 1, 1}   // spread
  };

  const char* const quality_names[] {
  	"major 7", "minor 7", "dominant 7", "half dimin.", "major triad", "minor triad", "dominant", "dimin. triad", "unisono"
  };

  const char* const quality_short_names[] {
  	"maj7", "min7", "dom7", "hdim", "maj", "min", "dom", "dim", "uni"
  };

  const char* const voicing_names[] {
  	"close", "drop 1", "drop 2", "drop 3", "antidrop 1", "antidrop 2", "antidrop 3", "spread"
  };
}
#endif // OC_CHORDS_PRESETS_H_