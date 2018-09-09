// Copyright (c) 2015, 2016 Patrick Dowling, Tim Churches
//
// Initial app implementation: Patrick Dowling (pld@gurkenkiste.com)
// Modifications by: Tim Churches (tim.churches@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Trigger-driven Neo-Riemannian Tonnetz transformations to generate chords

#include "OC_bitmaps.h"
#include "OC_strings.h"
#include "OC_trigger_delays.h"
#include "tonnetz/tonnetz_state.h"
#include "util/util_settings.h"
#include "util/util_ringbuffer.h"

// NOTE: H1200 state is updated in the ISR, and we're accessing shared state
// (e.g. outputs) without any sync mechanism. So there is a chance of the
// display being slightly inconsistent but the risk seems acceptable.
// Similarly for changing settings, but this should also be safe(ish) since
// - Each setting should be atomic (int)
// - Changing more than one settings happens seldomly
// - Settings aren't modified in the ISR

enum OutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE,
  OUTPUT_MODE_LAST
};

enum TransformPriority {
  TRANSFORM_PRIO_XPLR,
  TRANSFORM_PRIO_XLRP,
  TRANSFORM_PRIO_XRPL,
  TRANSFORM_PRIO_XPRL,
  TRANSFORM_PRIO_XRLP,
  TRANSFORM_PRIO_XLPR,
  TRANSFORM_PRIO_PLR_LAST
};

enum NshTransformPriority {
  TRANSFORM_PRIO_XNSH,
  TRANSFORM_PRIO_XSHN,
  TRANSFORM_PRIO_XHNS,
  TRANSFORM_PRIO_XNHS,
  TRANSFORM_PRIO_XHSN,
  TRANSFORM_PRIO_XSNH,
  TRANSFORM_PRIO_NSH_LAST
};

enum H1200Setting {
  H1200_SETTING_ROOT_OFFSET,
  H1200_SETTING_ROOT_OFFSET_CV,
  H1200_SETTING_OCTAVE,
  H1200_SETTING_OCTAVE_CV,
  H1200_SETTING_MODE,
  H1200_SETTING_INVERSION,
  H1200_SETTING_INVERSION_CV,
  H1200_SETTING_PLR_TRANSFORM_PRIO,
  H1200_SETTING_PLR_TRANSFORM_PRIO_CV,
  H1200_SETTING_NSH_TRANSFORM_PRIO,
  H1200_SETTING_NSH_TRANSFORM_PRIO_CV,
  H1200_SETTING_CV_SAMPLING,
  H1200_SETTING_OUTPUT_MODE,
  H1200_SETTING_TRIGGER_DELAY,
  H1200_SETTING_TRIGGER_TYPE,
  H1200_SETTING_EUCLIDEAN_CV1_MAPPING,
  H1200_SETTING_EUCLIDEAN_CV2_MAPPING,
  H1200_SETTING_EUCLIDEAN_CV3_MAPPING,
  H1200_SETTING_EUCLIDEAN_CV4_MAPPING,
  H1200_SETTING_P_EUCLIDEAN_LENGTH,
  H1200_SETTING_P_EUCLIDEAN_FILL,
  H1200_SETTING_P_EUCLIDEAN_OFFSET,
  H1200_SETTING_L_EUCLIDEAN_LENGTH,
  H1200_SETTING_L_EUCLIDEAN_FILL,
  H1200_SETTING_L_EUCLIDEAN_OFFSET,
  H1200_SETTING_R_EUCLIDEAN_LENGTH,
  H1200_SETTING_R_EUCLIDEAN_FILL,
  H1200_SETTING_R_EUCLIDEAN_OFFSET,
  H1200_SETTING_N_EUCLIDEAN_LENGTH,
  H1200_SETTING_N_EUCLIDEAN_FILL,
  H1200_SETTING_N_EUCLIDEAN_OFFSET,
  H1200_SETTING_S_EUCLIDEAN_LENGTH,
  H1200_SETTING_S_EUCLIDEAN_FILL,
  H1200_SETTING_S_EUCLIDEAN_OFFSET,
  H1200_SETTING_H_EUCLIDEAN_LENGTH,
  H1200_SETTING_H_EUCLIDEAN_FILL,
  H1200_SETTING_H_EUCLIDEAN_OFFSET,
  H1200_SETTING_LAST
};

enum H1200CvSampling {
  H1200_CV_SAMPLING_CONT,
  H1200_CV_SAMPLING_TRIG,
  H1200_CV_SAMPLING_LAST
};

enum H1200CvSource {
  H1200_CV_SOURCE_NONE,
  H1200_CV_SOURCE_CV1,
  H1200_CV_SOURCE_CV2,
  H1200_CV_SOURCE_CV3,
  H1200_CV_SOURCE_CV4,
  H1200_CV_SOURCE_LAST
};

enum H1200TriggerTypes {
  H1200_TRIGGER_TYPE_PLR,
  H1200_TRIGGER_TYPE_NSH,
  H1200_TRIGGER_TYPE_EUCLIDEAN,
  H1200_TRIGGER_TYPE_LAST
};

enum H1200EuclCvMappings {
  H1200_EUCL_CV_MAPPING_NONE,
  H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_LENGTH,
  H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_FILL,
  H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_OFFSET,
  H1200_EUCL_CV_MAPPING_LAST
} ;

class H1200Settings : public settings::SettingsBase<H1200Settings, H1200_SETTING_LAST> {
public:

  H1200CvSampling get_cv_sampling() const {
    return static_cast<H1200CvSampling>(values_[H1200_SETTING_CV_SAMPLING]);
  }

  int root_offset() const {
    return values_[H1200_SETTING_ROOT_OFFSET];
  }

  H1200CvSource get_root_offset_cv_src() const {
    return static_cast<H1200CvSource>(values_[H1200_SETTING_ROOT_OFFSET_CV]);
  }

  int octave() const {
    return values_[H1200_SETTING_OCTAVE];
  }

  H1200CvSource get_octave_cv_src() const {
    return static_cast<H1200CvSource>(values_[H1200_SETTING_OCTAVE_CV]);
  }

  EMode mode() const {
    return static_cast<EMode>(values_[H1200_SETTING_MODE]);
  }

  int inversion() const {
    return values_[H1200_SETTING_INVERSION];
  }

  H1200CvSource get_inversion_cv_src() const {
    return static_cast<H1200CvSource>(values_[H1200_SETTING_INVERSION_CV]);
  }

  uint8_t get_transform_priority() const {
    return values_[H1200_SETTING_PLR_TRANSFORM_PRIO];
  }

  H1200CvSource get_transform_priority_cv_src() const {
    return static_cast<H1200CvSource>(values_[H1200_SETTING_PLR_TRANSFORM_PRIO_CV]);
  }

  uint8_t get_nsh_transform_priority() const {
    return values_[H1200_SETTING_NSH_TRANSFORM_PRIO];
  }

  H1200CvSource get_nsh_transform_priority_cv_src() const {
    return static_cast<H1200CvSource>(values_[H1200_SETTING_NSH_TRANSFORM_PRIO_CV]);
  }

  OutputMode output_mode() const {
    return static_cast<OutputMode>(values_[H1200_SETTING_OUTPUT_MODE]);
  }

  H1200TriggerTypes get_trigger_type() const {
    return static_cast<H1200TriggerTypes>(values_[H1200_SETTING_TRIGGER_TYPE]);
  }

  uint16_t get_trigger_delay() const {
    return values_[H1200_SETTING_TRIGGER_DELAY];
  }

  uint8_t get_euclidean_cv1_mapping() const {
    return values_[H1200_SETTING_EUCLIDEAN_CV1_MAPPING];
  }

  uint8_t get_euclidean_cv2_mapping() const {
    return values_[H1200_SETTING_EUCLIDEAN_CV2_MAPPING];
  }

  uint8_t get_euclidean_cv3_mapping() const {
    return values_[H1200_SETTING_EUCLIDEAN_CV3_MAPPING];
  }

  uint8_t get_euclidean_cv4_mapping() const {
    return values_[H1200_SETTING_EUCLIDEAN_CV4_MAPPING];
  }

  uint8_t get_p_euclidean_length() const {
    return values_[H1200_SETTING_P_EUCLIDEAN_LENGTH];
  }

  uint8_t get_p_euclidean_fill() const {
    return values_[H1200_SETTING_P_EUCLIDEAN_FILL];
  }

  uint8_t get_p_euclidean_offset() const {
    return values_[H1200_SETTING_P_EUCLIDEAN_OFFSET];
  }

  uint8_t get_l_euclidean_length() const {
    return values_[H1200_SETTING_L_EUCLIDEAN_LENGTH];
  }

  uint8_t get_l_euclidean_fill() const {
    return values_[H1200_SETTING_L_EUCLIDEAN_FILL];
  }

  uint8_t get_l_euclidean_offset() const {
    return values_[H1200_SETTING_L_EUCLIDEAN_OFFSET];
  }

  uint8_t get_r_euclidean_length() const {
    return values_[H1200_SETTING_R_EUCLIDEAN_LENGTH];
  }

  uint8_t get_r_euclidean_fill() const {
    return values_[H1200_SETTING_R_EUCLIDEAN_FILL];
  }

  uint8_t get_r_euclidean_offset() const {
    return values_[H1200_SETTING_R_EUCLIDEAN_OFFSET];
  }

  uint8_t get_n_euclidean_length() const {
    return values_[H1200_SETTING_N_EUCLIDEAN_LENGTH];
  }

  uint8_t get_n_euclidean_fill() const {
    return values_[H1200_SETTING_N_EUCLIDEAN_FILL];
  }

  uint8_t get_n_euclidean_offset() const {
    return values_[H1200_SETTING_N_EUCLIDEAN_OFFSET];
  }

  uint8_t get_s_euclidean_length() const {
    return values_[H1200_SETTING_S_EUCLIDEAN_LENGTH];
  }

  uint8_t get_s_euclidean_fill() const {
    return values_[H1200_SETTING_S_EUCLIDEAN_FILL];
  }

  uint8_t get_s_euclidean_offset() const {
    return values_[H1200_SETTING_S_EUCLIDEAN_OFFSET];
  }

  uint8_t get_h_euclidean_length() const {
    return values_[H1200_SETTING_H_EUCLIDEAN_LENGTH];
  }

  uint8_t get_h_euclidean_fill() const {
    return values_[H1200_SETTING_H_EUCLIDEAN_FILL];
  }

  uint8_t get_h_euclidean_offset() const {
    return values_[H1200_SETTING_H_EUCLIDEAN_OFFSET];
  }

    
  void Init() {
    InitDefaults();
    update_enabled_settings();
    manual_mode_change_ = false;
  }

  H1200Setting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  void mode_change(bool yn) {
    manual_mode_change_ = yn;
  }

  bool mode_manual_change() const {
    return manual_mode_change_; 
  }

  void update_enabled_settings() {
    
    H1200Setting *settings = enabled_settings_;

    *settings++ =   H1200_SETTING_ROOT_OFFSET;
    *settings++ =   H1200_SETTING_ROOT_OFFSET_CV;
    *settings++ =   H1200_SETTING_OCTAVE;
    *settings++ =   H1200_SETTING_OCTAVE_CV;
    *settings++ =   H1200_SETTING_MODE;
    *settings++ =   H1200_SETTING_INVERSION;
    *settings++ =   H1200_SETTING_INVERSION_CV;
    *settings++ =   H1200_SETTING_PLR_TRANSFORM_PRIO;
    *settings++ =   H1200_SETTING_PLR_TRANSFORM_PRIO_CV;
    *settings++ =   H1200_SETTING_NSH_TRANSFORM_PRIO;
    *settings++ =   H1200_SETTING_NSH_TRANSFORM_PRIO_CV;
    *settings++ =   H1200_SETTING_CV_SAMPLING;
    *settings++ =   H1200_SETTING_OUTPUT_MODE;
    *settings++ =   H1200_SETTING_TRIGGER_DELAY;
    *settings++ =   H1200_SETTING_TRIGGER_TYPE;
 
    switch (get_trigger_type()) {
      case H1200_TRIGGER_TYPE_EUCLIDEAN:
        *settings++ =   H1200_SETTING_EUCLIDEAN_CV1_MAPPING;
        *settings++ =   H1200_SETTING_EUCLIDEAN_CV2_MAPPING;
        *settings++ =   H1200_SETTING_EUCLIDEAN_CV3_MAPPING;
        *settings++ =   H1200_SETTING_EUCLIDEAN_CV4_MAPPING;
        *settings++ =   H1200_SETTING_P_EUCLIDEAN_LENGTH;
          *settings++ =   H1200_SETTING_P_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_P_EUCLIDEAN_OFFSET;
        *settings++ =   H1200_SETTING_L_EUCLIDEAN_LENGTH;
          *settings++ =   H1200_SETTING_L_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_L_EUCLIDEAN_OFFSET;
        *settings++ =   H1200_SETTING_R_EUCLIDEAN_LENGTH;
          *settings++ =   H1200_SETTING_R_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_R_EUCLIDEAN_OFFSET;
        *settings++ =   H1200_SETTING_N_EUCLIDEAN_LENGTH;
           *settings++ =   H1200_SETTING_N_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_N_EUCLIDEAN_OFFSET;
        *settings++ =   H1200_SETTING_S_EUCLIDEAN_LENGTH;
          *settings++ =   H1200_SETTING_S_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_S_EUCLIDEAN_OFFSET;
        *settings++ =   H1200_SETTING_H_EUCLIDEAN_LENGTH;
          *settings++ =   H1200_SETTING_H_EUCLIDEAN_FILL;
          *settings++ =   H1200_SETTING_H_EUCLIDEAN_OFFSET;
        break;
      default:
        break;
     }
    
    num_enabled_settings_ = settings - enabled_settings_;
  }

private:
  int num_enabled_settings_;
  bool manual_mode_change_;
  H1200Setting enabled_settings_[H1200_SETTING_LAST];
};

const char * const output_mode_names[] = {
  "Chord",
  "Tune"
};

const char * const plr_trigger_mode_names[] = {
  "P>L>R",
  "L>R>P",
  "R>P>L",
  "P>R>L",
  "R>L>P",
  "L>P>R",
};

const char * const nsh_trigger_mode_names[] = {
  "N>S>H",
  "S>H>N",
  "H>N>S",
  "N>H>S",
  "H>S>N",
  "S>N>H",
};

const char * const trigger_type_names[] = {
  "PLR",
  "NSH",
  "Eucl",
};

const char * const mode_names[] = {
  "Maj", "Min"
};

const char* const h1200_cv_sampling[2] = {
  "Cont", "Trig"
};

const char* const h1200_eucl_cv_mappings[] = {
  "None", 
  "Plen", "Pfil", "Poff",
  "Llen", "Lfil", "Loff",
  "Rlen", "Rfil", "Roff",
  "Nlen", "Nfil", "Noff",
  "Slen", "Sfil", "Soff",
  "Hlen", "Hfil", "Hoff",  
};

SETTINGS_DECLARE(H1200Settings, H1200_SETTING_LAST) {
  {0, -11, 11, "Transpose", NULL, settings::STORAGE_TYPE_I8},
  {H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_LAST-1, "Transpose CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8},
  #ifdef BUCHLA_4U
  {0, 0, 7, "Octave", NULL, settings::STORAGE_TYPE_I8},
  #else
  {0, -3, 3, "Octave", NULL, settings::STORAGE_TYPE_I8},
  #endif
  {H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_LAST-1, "Octave CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8},
  {MODE_MAJOR, 0, MODE_LAST-1, "Root mode", mode_names, settings::STORAGE_TYPE_U8},
  {0, -3, 3, "Inversion", NULL, settings::STORAGE_TYPE_I8},
  {H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_LAST-1, "Inversion CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8},
  {TRANSFORM_PRIO_XPLR, 0, TRANSFORM_PRIO_PLR_LAST-1, "PLR Priority", plr_trigger_mode_names, settings::STORAGE_TYPE_U8},
  {H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_LAST-1, "PLR Prior CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8},
  {TRANSFORM_PRIO_XNSH, 0, TRANSFORM_PRIO_NSH_LAST-1, "NSH Priority", nsh_trigger_mode_names, settings::STORAGE_TYPE_U8},
  {H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_NONE, H1200_CV_SOURCE_LAST-1, "NSH Prior CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8},
  {H1200_CV_SAMPLING_CONT, H1200_CV_SAMPLING_CONT, H1200_CV_SAMPLING_LAST-1, "CV sampling", h1200_cv_sampling, settings::STORAGE_TYPE_U8},
  {OUTPUT_CHORD_VOICING, 0, OUTPUT_MODE_LAST-1, "Output mode", output_mode_names, settings::STORAGE_TYPE_U8},
  { 0, 0, OC::kNumDelayTimes - 1, "Trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  {H1200_TRIGGER_TYPE_PLR, 0, H1200_TRIGGER_TYPE_LAST-1, "Trigger type", trigger_type_names, settings::STORAGE_TYPE_U8},
  {H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_LAST-1, "Eucl CV1 map", h1200_eucl_cv_mappings, settings::STORAGE_TYPE_U8},
  {H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_LAST-1, "Eucl CV2 map", h1200_eucl_cv_mappings, settings::STORAGE_TYPE_U8},
  {H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_LAST-1, "Eucl CV3 map", h1200_eucl_cv_mappings, settings::STORAGE_TYPE_U8},
  {H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_NONE, H1200_EUCL_CV_MAPPING_LAST-1, "Eucl CV4 map", h1200_eucl_cv_mappings, settings::STORAGE_TYPE_U8},
  { 8, 2, 32,  " P EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " P EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " P EuOffs", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 32,  " L EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " L EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " L EuOffs", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 32,  " R EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " R EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " R EuOffs", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 32,  " N EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " N EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " N EuOffs", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 32,  " S EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " S EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " S EuOffs", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 32,  " H EuLeng", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32,  " H EuFill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31,  " H EuOffs", NULL, settings::STORAGE_TYPE_U8 },
};

static constexpr uint32_t TRIGGER_MASK_TR1 = OC::DIGITAL_INPUT_1_MASK;
static constexpr uint32_t TRIGGER_MASK_P = OC::DIGITAL_INPUT_2_MASK;
static constexpr uint32_t TRIGGER_MASK_L = OC::DIGITAL_INPUT_3_MASK;
static constexpr uint32_t TRIGGER_MASK_R = OC::DIGITAL_INPUT_4_MASK;
static constexpr uint32_t TRIGGER_MASK_N = OC::DIGITAL_INPUT_2_MASK;
static constexpr uint32_t TRIGGER_MASK_S = OC::DIGITAL_INPUT_3_MASK;
static constexpr uint32_t TRIGGER_MASK_H = OC::DIGITAL_INPUT_4_MASK;
static constexpr uint32_t TRIGGER_MASK_DIRTY = 0x10;
static constexpr uint32_t TRIGGER_MASK_RESET = TRIGGER_MASK_TR1 | TRIGGER_MASK_DIRTY;

namespace H1200 {
  enum UserActions {
    ACTION_FORCE_UPDATE,
    ACTION_MANUAL_RESET
  };

  typedef uint32_t UiAction;
};

H1200Settings h1200_settings;

class H1200State {
public:
  static constexpr int kMaxInversion = 3;

  void Init() {
    cursor.Init(H1200_SETTING_ROOT_OFFSET, H1200_SETTING_LAST - 1);
    display_notes = true;

    quantizer.Init();
    tonnetz_state.init();
    trigger_delays_.Init();

    euclidean_counter_ = 0;
    root_sample_ = false;
    root_ = 0;
    p_euclidean_length_ = 8;
    p_euclidean_fill_ = 0;
    p_euclidean_offset_ = 0;
    l_euclidean_length_ = 8;
    l_euclidean_fill_ = 0;
    l_euclidean_offset_ = 0;
    r_euclidean_length_ = 8;
    r_euclidean_fill_ = 0;
    r_euclidean_offset_ = 0;
    n_euclidean_length_ = 8;
    n_euclidean_fill_ = 0;
    n_euclidean_offset_ = 0;
    s_euclidean_length_ = 8;
    s_euclidean_fill_ = 0;
    s_euclidean_offset_ = 0;
    h_euclidean_length_ = 8;
    h_euclidean_fill_ = 0;
    h_euclidean_offset_ = 0;
    
  }

  void map_euclidean_cv(uint8_t cv_mapping, int channel_cv) {
    switch(cv_mapping) {
      case H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_LENGTH:
        p_euclidean_length_ = p_euclidean_length_ + channel_cv;
        CONSTRAIN(p_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_FILL:
        p_euclidean_fill_ = p_euclidean_fill_ + channel_cv;
        CONSTRAIN(p_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_P_EUCLIDEAN_OFFSET:
        p_euclidean_offset_ = p_euclidean_offset_ + channel_cv;
        CONSTRAIN(p_euclidean_offset_, 0, 31);
        break;
      case H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_LENGTH:
        l_euclidean_length_ = l_euclidean_length_ + channel_cv;
        CONSTRAIN(l_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_FILL:
        l_euclidean_fill_ = l_euclidean_fill_ + channel_cv;
        CONSTRAIN(l_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_L_EUCLIDEAN_OFFSET:
        l_euclidean_offset_ = l_euclidean_offset_ + channel_cv;
        CONSTRAIN(l_euclidean_offset_, 0, 31);
        break;
      case H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_LENGTH:
        r_euclidean_length_ = r_euclidean_length_ + channel_cv;
        CONSTRAIN(r_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_FILL:
        r_euclidean_fill_ = r_euclidean_fill_ + channel_cv;
        CONSTRAIN(r_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_R_EUCLIDEAN_OFFSET:
        r_euclidean_offset_ = r_euclidean_offset_ + channel_cv;
        CONSTRAIN(r_euclidean_offset_, 0, 31);
        break;
       case H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_LENGTH:
        n_euclidean_length_ = n_euclidean_length_ + channel_cv;
        CONSTRAIN(n_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_FILL:
        n_euclidean_fill_ = n_euclidean_fill_ + channel_cv;
        CONSTRAIN(n_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_N_EUCLIDEAN_OFFSET:
        n_euclidean_offset_ = n_euclidean_offset_ + channel_cv;
        CONSTRAIN(n_euclidean_offset_, 0, 31);
        break;
      case H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_LENGTH:
        s_euclidean_length_ = s_euclidean_length_ + channel_cv;
        CONSTRAIN(s_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_FILL:
        s_euclidean_fill_ = s_euclidean_fill_ + channel_cv;
        CONSTRAIN(s_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_S_EUCLIDEAN_OFFSET:
        s_euclidean_offset_ = s_euclidean_offset_ + channel_cv;
        CONSTRAIN(s_euclidean_offset_, 0, 31);
        break;
      case H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_LENGTH:
        h_euclidean_length_ = h_euclidean_length_ + channel_cv;
        CONSTRAIN(h_euclidean_length_, 2, 32);
        break;
      case H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_FILL:
        h_euclidean_fill_ = h_euclidean_fill_ + channel_cv;
        CONSTRAIN(h_euclidean_fill_, 0, 32);
        break;
     case H1200_EUCL_CV_MAPPING_H_EUCLIDEAN_OFFSET:
        h_euclidean_offset_ = h_euclidean_offset_ + channel_cv;
        CONSTRAIN(h_euclidean_offset_, 0, 31);
        break;
    default:
        break;
    }
  }

  void force_update() {
    ui_actions.Write(H1200::ACTION_FORCE_UPDATE);
  }

  void manual_reset() {
    ui_actions.Write(H1200::ACTION_MANUAL_RESET);
  }

  void Render(int32_t root, int inversion, int octave, OutputMode output_mode) {
    tonnetz_state.render(root + octave * 12, inversion);

    switch (output_mode) {
    case OUTPUT_CHORD_VOICING: {
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_A>(tonnetz_state.outputs(0), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_A));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_B>(tonnetz_state.outputs(1), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_B));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_C>(tonnetz_state.outputs(2), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_C));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_D>(tonnetz_state.outputs(3), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_D));
    }
    break;
    case OUTPUT_TUNE: {
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_A>(tonnetz_state.outputs(0), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_A));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_B>(tonnetz_state.outputs(0), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_B));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_C>(tonnetz_state.outputs(0), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_C));
      OC::DAC::set_voltage_scaled_semitone<DAC_CHANNEL_D>(tonnetz_state.outputs(0), 0, OC::DAC::get_voltage_scaling(DAC_CHANNEL_D));
    }
    break;
    default: break;
    }
  }

  menu::ScreenCursor<menu::kScreenLines> cursor;
  menu::ScreenCursor<menu::kScreenLines> cursor_state;
  bool display_notes;

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }
  
  OC::SemitoneQuantizer quantizer;
  TonnetzState tonnetz_state;
  util::RingBuffer<H1200::UiAction, 4> ui_actions;
  OC::TriggerDelays<OC::kMaxTriggerDelayTicks> trigger_delays_;  
  uint32_t euclidean_counter_;
  bool root_sample_ ;
  int32_t root_ ;
  uint8_t p_euclidean_length_  ;
  uint8_t p_euclidean_fill_  ;
  uint8_t p_euclidean_offset_  ;
  uint8_t l_euclidean_length_  ;
  uint8_t l_euclidean_fill_  ;
  uint8_t l_euclidean_offset_  ;
  uint8_t r_euclidean_length_  ;
  uint8_t r_euclidean_fill_  ;
  uint8_t r_euclidean_offset_  ;
  uint8_t n_euclidean_length_  ;
  uint8_t n_euclidean_fill_  ;
  uint8_t n_euclidean_offset_  ;
  uint8_t s_euclidean_length_  ;
  uint8_t s_euclidean_fill_  ;
  uint8_t s_euclidean_offset_  ;
  uint8_t h_euclidean_length_  ;
  uint8_t h_euclidean_fill_  ;
  uint8_t h_euclidean_offset_  ;
 
};

H1200State h1200_state;

void FASTRUN H1200_clock(uint32_t triggers) {

  triggers = h1200_state.trigger_delays_.Process(triggers, OC::trigger_delay_ticks[h1200_settings.get_trigger_delay()]);
  
  // Reset has priority
  if (triggers & TRIGGER_MASK_TR1) {
    h1200_state.tonnetz_state.reset(h1200_settings.mode());
    h1200_settings.mode_change(false);
  }

  // Reset on next trigger = manual change min/maj
  if (h1200_settings.mode_manual_change()) {
    
      if ((triggers & OC::DIGITAL_INPUT_2_MASK) || (triggers & OC::DIGITAL_INPUT_3_MASK) || (triggers & OC::DIGITAL_INPUT_4_MASK)) {
        h1200_settings.mode_change(false);
        h1200_state.tonnetz_state.reset(h1200_settings.mode());
      }
  }

  int32_t root_ = h1200_settings.root_offset();
  int8_t octave_ = h1200_settings.octave();
  int inversion_ = h1200_settings.inversion();
  uint8_t plr_transform_priority_ = h1200_settings.get_transform_priority();
  uint8_t nsh_transform_priority_ = h1200_settings.get_nsh_transform_priority();

  if (triggers || (h1200_settings.get_cv_sampling() == H1200_CV_SAMPLING_CONT)) {
        switch (h1200_settings.get_root_offset_cv_src()) {
          case H1200_CV_SOURCE_CV1:
            root_ += h1200_state.quantizer.Process(OC::ADC::raw_pitch_value(ADC_CHANNEL_1));
            break ;
          case H1200_CV_SOURCE_CV2:
            root_ += h1200_state.quantizer.Process(OC::ADC::raw_pitch_value(ADC_CHANNEL_2));
            break ;
          case H1200_CV_SOURCE_CV3:
            root_ += h1200_state.quantizer.Process(OC::ADC::raw_pitch_value(ADC_CHANNEL_3));
            break ;
          case H1200_CV_SOURCE_CV4:
            root_ += h1200_state.quantizer.Process(OC::ADC::raw_pitch_value(ADC_CHANNEL_4));
            break ;
          default:
            break; 
        } 
      
        switch (h1200_settings.get_octave_cv_src()) {
          case H1200_CV_SOURCE_CV1:
            octave_ += ((OC::ADC::value<ADC_CHANNEL_1>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV2:
            octave_ += ((OC::ADC::value<ADC_CHANNEL_2>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV3:
            octave_ += ((OC::ADC::value<ADC_CHANNEL_3>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV4:
            octave_ += ((OC::ADC::value<ADC_CHANNEL_4>() + 255) >> 9);
            break ;
          default:
            break; 
        }
        #ifdef BUCHLA_4U
          CONSTRAIN(octave_, 0, 7);
        #else
          CONSTRAIN(octave_, -3, 3);
        #endif
      
        switch (h1200_settings.get_inversion_cv_src()) {
          case H1200_CV_SOURCE_CV1:
            inversion_ += ((OC::ADC::value<ADC_CHANNEL_1>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV2:
            inversion_ += ((OC::ADC::value<ADC_CHANNEL_2>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV3:
            inversion_ += ((OC::ADC::value<ADC_CHANNEL_3>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV4:
            inversion_ += ((OC::ADC::value<ADC_CHANNEL_4>() + 255) >> 9);
            break ;
          default:
            break; 
        } 
        CONSTRAIN(inversion_,-H1200State::kMaxInversion, H1200State::kMaxInversion);
      
        switch (h1200_settings.get_transform_priority_cv_src()) {
          case H1200_CV_SOURCE_CV1:
            plr_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_1>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV2:
            plr_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_2>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV3:
            plr_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_3>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV4:
            plr_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_4>() + 255) >> 9);
            break ;
          default:
            break; 
        } 
      
        CONSTRAIN(plr_transform_priority_, TRANSFORM_PRIO_XPLR, TRANSFORM_PRIO_PLR_LAST-1);
        
        switch (h1200_settings.get_nsh_transform_priority_cv_src()) {
          case H1200_CV_SOURCE_CV1:
            nsh_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_1>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV2:
            nsh_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_2>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV3:
            nsh_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_3>() + 255) >> 9);
            break ;
          case H1200_CV_SOURCE_CV4:
            nsh_transform_priority_ += ((OC::ADC::value<ADC_CHANNEL_4>() + 255) >> 9);
            break ;
          default:
            break; 
        } 
      
        CONSTRAIN(nsh_transform_priority_, TRANSFORM_PRIO_XNSH, TRANSFORM_PRIO_NSH_LAST-1);
  }

  if (h1200_settings.get_trigger_type() == H1200_TRIGGER_TYPE_PLR) {
    
      // Since there can be simultaneous triggers, there is a definable priority.
      // Reset always has top priority

      switch (plr_transform_priority_) {
        case TRANSFORM_PRIO_XPLR:
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          break;
    
        case TRANSFORM_PRIO_XLRP:
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          break;
    
        case TRANSFORM_PRIO_XRPL:
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          break;
    
        case TRANSFORM_PRIO_XPRL:
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          break;
    
        case TRANSFORM_PRIO_XRLP:
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          break;
    
        case TRANSFORM_PRIO_XLPR:
          if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          break;
    
        default: break;
      }
  } else if (h1200_settings.get_trigger_type() == H1200_TRIGGER_TYPE_NSH) {
    
      // Since there can be simultaneous triggers, there is a definable priority.
      // Reset always has top priority
      
      switch (nsh_transform_priority_) {
        case TRANSFORM_PRIO_XNSH:
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          break;

        case TRANSFORM_PRIO_XSHN:
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          break;
    
        case TRANSFORM_PRIO_XHNS:
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          break;
    
        case TRANSFORM_PRIO_XNHS:
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          break;
    
        case TRANSFORM_PRIO_XHSN:
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          break;
    
        case TRANSFORM_PRIO_XSNH:
          if (triggers & TRIGGER_MASK_S) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (triggers & TRIGGER_MASK_N) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (triggers & TRIGGER_MASK_H) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          break;
    
        default: break;
      }
  } else {

    if (triggers) {

      h1200_state.p_euclidean_length_ = h1200_settings.get_p_euclidean_length() ;
      h1200_state.p_euclidean_fill_ = h1200_settings.get_p_euclidean_fill() ;
      h1200_state.p_euclidean_offset_ = h1200_settings.get_p_euclidean_offset() ;
      h1200_state.l_euclidean_length_ = h1200_settings.get_l_euclidean_length() ;
      h1200_state.l_euclidean_fill_ = h1200_settings.get_l_euclidean_fill() ;
      h1200_state.l_euclidean_offset_ = h1200_settings.get_l_euclidean_offset() ;
      h1200_state.r_euclidean_length_ = h1200_settings.get_r_euclidean_length() ;
      h1200_state.r_euclidean_fill_ = h1200_settings.get_r_euclidean_fill() ;
      h1200_state.r_euclidean_offset_ = h1200_settings.get_r_euclidean_offset() ;
      h1200_state.n_euclidean_length_ = h1200_settings.get_n_euclidean_length() ;
      h1200_state.n_euclidean_fill_ = h1200_settings.get_n_euclidean_fill() ;
      h1200_state.n_euclidean_offset_ = h1200_settings.get_n_euclidean_offset() ;
      h1200_state.s_euclidean_length_ = h1200_settings.get_s_euclidean_length() ;
      h1200_state.s_euclidean_fill_ = h1200_settings.get_s_euclidean_fill() ;
      h1200_state.s_euclidean_offset_ = h1200_settings.get_s_euclidean_offset() ;
      h1200_state.h_euclidean_length_ = h1200_settings.get_h_euclidean_length() ;
      h1200_state.h_euclidean_fill_ = h1200_settings.get_h_euclidean_fill() ;
      h1200_state.h_euclidean_offset_ = h1200_settings.get_h_euclidean_offset() ;

      int channel_1_cv_ = ((OC::ADC::value<ADC_CHANNEL_1>() + 127) >> 8);
      int channel_2_cv_ = ((OC::ADC::value<ADC_CHANNEL_2>() + 127) >> 8);
      int channel_3_cv_ = ((OC::ADC::value<ADC_CHANNEL_3>() + 127) >> 8);
      int channel_4_cv_ = ((OC::ADC::value<ADC_CHANNEL_4>() + 127) >> 8);

 
      h1200_state.map_euclidean_cv(h1200_settings.get_euclidean_cv1_mapping(), channel_1_cv_) ;
      h1200_state.map_euclidean_cv(h1200_settings.get_euclidean_cv2_mapping(), channel_2_cv_) ;
      h1200_state.map_euclidean_cv(h1200_settings.get_euclidean_cv3_mapping(), channel_3_cv_) ;
      h1200_state.map_euclidean_cv(h1200_settings.get_euclidean_cv4_mapping(), channel_4_cv_) ;
          
      ++h1200_state.euclidean_counter_;
      
      switch (plr_transform_priority_) {
        case TRANSFORM_PRIO_XPLR:
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          break;   
        case TRANSFORM_PRIO_XLRP:
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          break;   
        case TRANSFORM_PRIO_XRPL:
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          break;   
        case TRANSFORM_PRIO_XPRL:
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          break;   
        case TRANSFORM_PRIO_XRLP:
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          break;   
        case TRANSFORM_PRIO_XLPR:
          if (EuclideanFilter(h1200_state.l_euclidean_length_, h1200_state.l_euclidean_fill_, h1200_state.l_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
          if (EuclideanFilter(h1200_state.p_euclidean_length_, h1200_state.p_euclidean_fill_, h1200_state.p_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
          if (EuclideanFilter(h1200_state.r_euclidean_length_, h1200_state.r_euclidean_fill_, h1200_state.r_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
          break;
    
        default: break;
      }
        
      switch (nsh_transform_priority_) {
        case TRANSFORM_PRIO_XNSH:
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          break;
        case TRANSFORM_PRIO_XSHN:
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          break;
        case TRANSFORM_PRIO_XHNS:
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          break;
        case TRANSFORM_PRIO_XNHS:
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          break;
        case TRANSFORM_PRIO_XHSN:
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          break;
        case TRANSFORM_PRIO_XSNH:
          if (EuclideanFilter(h1200_state.s_euclidean_length_, h1200_state.s_euclidean_fill_, h1200_state.s_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_S);
          if (EuclideanFilter(h1200_state.n_euclidean_length_, h1200_state.n_euclidean_fill_, h1200_state.n_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_N);
          if (EuclideanFilter(h1200_state.h_euclidean_length_, h1200_state.h_euclidean_fill_, h1200_state.h_euclidean_offset_, h1200_state.euclidean_counter_)) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_H);
          break;
          
         default: break;
      }  
       
    }
 }

  // Finally, we're ready to actually render the triad transformation!
  if (triggers || (h1200_settings.get_cv_sampling() == H1200_CV_SAMPLING_CONT)) h1200_state.Render(root_, inversion_, octave_, h1200_settings.output_mode());

  if (triggers)
    MENU_REDRAW = 1;
}

void H1200_init() {
  h1200_settings.Init();
  h1200_state.Init();
  h1200_settings.update_enabled_settings();
  h1200_state.cursor.AdjustEnd(h1200_settings.num_enabled_settings() - 1);
}

size_t H1200_storageSize() {
  return H1200Settings::storageSize();
}

size_t H1200_save(void *storage) {
  return h1200_settings.Save(storage);
}

size_t H1200_restore(const void *storage) {
  h1200_settings.update_enabled_settings();
  h1200_state.cursor.AdjustEnd(h1200_settings.num_enabled_settings() - 1);
  return h1200_settings.Restore(storage);
}

void H1200_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      h1200_state.cursor.set_editing(false);
      h1200_state.tonnetz_state.reset(h1200_settings.mode());
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      h1200_settings.update_enabled_settings();
      h1200_state.cursor.AdjustEnd(h1200_settings.num_enabled_settings() - 1);
      break;
  }
}

void H1200_isr() {
  uint32_t triggers = OC::DigitalInputs::clocked();

  while (h1200_state.ui_actions.readable()) {
    switch (h1200_state.ui_actions.Read()) {
      case H1200::ACTION_FORCE_UPDATE:
        triggers |= TRIGGER_MASK_DIRTY;
        break;
      case H1200::ACTION_MANUAL_RESET:
        triggers |= TRIGGER_MASK_RESET;
        break;
      default:
        break;
    }
  }

  H1200_clock(triggers);
}

void H1200_loop() {
}

void H1200_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        if (h1200_settings.change_value(H1200_SETTING_OCTAVE, 1))
          h1200_state.force_update();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        if (h1200_settings.change_value(H1200_SETTING_OCTAVE, -1))
          h1200_state.force_update();
        break;
      case OC::CONTROL_BUTTON_L:
        h1200_state.display_notes = !h1200_state.display_notes;
        break;
      case OC::CONTROL_BUTTON_R:
        h1200_state.cursor.toggle_editing();
        break;
    }
  } else {
    if (OC::CONTROL_BUTTON_L == event.control) {
      h1200_settings.InitDefaults();
      h1200_state.manual_reset();
    }
  }
}

void H1200_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    if (h1200_settings.change_value(H1200_SETTING_INVERSION, event.value))
      h1200_state.force_update();
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (h1200_state.cursor.editing()) {
      H1200Setting setting = h1200_settings.enabled_setting_at(h1200_state.cursor_pos());
      
      if (h1200_settings.change_value(h1200_state.cursor.cursor_pos(), event.value)) {
          if (setting == H1200_SETTING_TRIGGER_TYPE) {
              h1200_settings.update_enabled_settings();
              h1200_state.cursor.AdjustEnd(h1200_settings.num_enabled_settings() - 1);            
          }
          h1200_state.force_update();

          switch(setting) {
   
              case H1200_SETTING_TRIGGER_TYPE:
                h1200_settings.update_enabled_settings();
                h1200_state.cursor.AdjustEnd(h1200_settings.num_enabled_settings() - 1);
                // hack/hide extra options when default trigger type is selected
                if (h1200_settings.get_trigger_type() != H1200_TRIGGER_TYPE_EUCLIDEAN) 
                  h1200_state.cursor.Scroll(h1200_state.cursor_pos());
              break;
              case H1200_SETTING_MODE:
                h1200_settings.mode_change(true);
              break;
              default:
              break;
           }
          
      }        
    } else {
      h1200_state.cursor.Scroll(event.value);
    }
  }
}

void H1200_menu() {

  /* show mode change instantly, because it's somewhat confusing (inconsistent?) otherwise */
  const EMode current_mode = h1200_settings.mode(); // const EMode current_mode = h1200_state.tonnetz_state.current_chord().mode();
  int outputs[4];
  h1200_state.tonnetz_state.get_outputs(outputs);

  menu::DefaultTitleBar::Draw();
  graphics.print(note_name(outputs[0]));
  graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
  graphics.print(mode_names[current_mode]);
  graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);

  if (h1200_state.display_notes) {
    for (size_t i=1; i < 4; ++i) {
      graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
      graphics.print(note_name(outputs[i]));
    }
  } else {
    for (size_t i=1; i < 4; ++i) {
      graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
      graphics.pretty_print(outputs[i]);
    }
  }

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(h1200_state.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {

    const int setting = h1200_settings.enabled_setting_at(settings_list.Next(list_item));
    const int value = h1200_settings.get_value(setting);
    const settings::value_attr &attr = H1200Settings::value_attr(setting); 

    list_item.DrawDefault(value, attr);
  }
}

void H1200_screensaver() {
  uint8_t y = 0;
  static const uint8_t x_col_0 = 66;
  static const uint8_t x_col_1 = 66 + 24;
  static const uint8_t line_h = 16;
  static const weegfx::coord_t note_circle_x = 32;
  static const weegfx::coord_t note_circle_y = 32;

  uint32_t history = h1200_state.tonnetz_state.history();
  int outputs[4];
  h1200_state.tonnetz_state.get_outputs(outputs);

  uint8_t normalized[3];
  y = 8;
  for (size_t i=0; i < 3; ++i, y += line_h) {
    int note = outputs[i + 1];
    int octave = note / 12;
    note = (note + 120) % 12;
    normalized[i] = note;

    graphics.setPrintPos(x_col_1, y);
    graphics.print(OC::Strings::note_names_unpadded[note]);
    graphics.print(octave + 1);
  }
  y = 0;

  size_t len = 4;
  while (len--) {
    graphics.setPrintPos(x_col_0, y);
    graphics.print(history & 0x80 ? '+' : '-');
    graphics.print(tonnetz::transform_names[static_cast<tonnetz::ETransformType>(history & 0x7f)]);
    y += line_h;
    history >>= 8;
  }

  OC::visualize_pitch_classes(normalized, note_circle_x, note_circle_y);
}

#ifdef H1200_DEBUG  
void H1200_debug() {
  int cv = OC::ADC::value<ADC_CHANNEL_4>();
  int scaled = ((OC::ADC::value<ADC_CHANNEL_4>() + 127) >> 8);

  graphics.setPrintPos(2, 12);
  graphics.printf("I: %4d %4d", cv, scaled);
}
#endif // H1200_DEBUG
