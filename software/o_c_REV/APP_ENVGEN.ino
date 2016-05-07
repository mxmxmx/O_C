// Copyright (c) 2016 Patrick Dowling, Tim Churches
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
// Quad enevelope generator app, based on the multistage envelope implementation
// from Peaks by Olivier Gillet (see peaks_multistage_envelope.h/cpp)

#include "OC_apps.h"
#include "OC_bitmaps.h"
#include "OC_digital_inputs.h"
#include "OC_menus.h"
#include "OC_strings.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "peaks_multistage_envelope.h"
#include "bjorklund.h"

// peaks::MultistageEnvelope allow setting of more parameters per stage, but
// that will involve more editing code, so keeping things simple for now
// with one value per stage.
//
// MultistageEnvelope maps times to lut_env_increments directly, so only 256 discrete values (no interpolation)
// Levels are 0-32767 to be positive on Peaks' bipolar output

enum EnvelopeSettings {
  ENV_SETTING_TYPE,
  ENV_SETTING_SEG1_VALUE,
  ENV_SETTING_SEG2_VALUE,
  ENV_SETTING_SEG3_VALUE,
  ENV_SETTING_SEG4_VALUE,
  ENV_SETTING_TRIGGER_INPUT,
  ENV_SETTING_TRIGGER_DELAY_MODE,
  ENV_SETTING_TRIGGER_DELAY_COUNT,
  ENV_SETTING_TRIGGER_DELAY_MILLISECONDS,
  ENV_SETTING_TRIGGER_DELAY_SECONDS,
  ENV_SETTING_EUCLIDEAN_LENGTH,
  ENV_SETTING_EUCLIDEAN_FILL,
  ENV_SETTING_EUCLIDEAN_OFFSET,
  ENV_SETTING_CV1,
  ENV_SETTING_CV2,
  ENV_SETTING_CV3,
  ENV_SETTING_CV4,
  ENV_SETTING_HARD_RESET,
  ENV_SETTING_GATE_HIGH,
  ENV_SETTING_ATTACK_SHAPE,
  ENV_SETTING_DECAY_SHAPE,
  ENV_SETTING_RELEASE_SHAPE,
  ENV_SETTING_LAST
};

enum CVMapping {
  CV_MAPPING_NONE,
  CV_MAPPING_SEG1,
  CV_MAPPING_SEG2,
  CV_MAPPING_SEG3,
  CV_MAPPING_SEG4,
  CV_MAPPING_EUCLIDEAN_LENGTH,
  CV_MAPPING_EUCLIDEAN_FILL,
  CV_MAPPING_EUCLIDEAN_OFFSET,
  CV_MAPPING_DELAY_MSEC,
  CV_MAPPING_LAST
};

enum EnvelopeType {
  ENV_TYPE_AD,
  ENV_TYPE_ADSR,
  ENV_TYPE_ADR,
  ENV_TYPE_AR,
  ENV_TYPE_ADSAR,
  ENV_TYPE_ADAR,
  ENV_TYPE_AD_LOOP,
  ENV_TYPE_ADR_LOOP,
  ENV_TYPE_ADAR_LOOP,
  ENV_TYPE_LAST, ENV_TYPE_FIRST = ENV_TYPE_AD
};

enum TriggerDelayMode {
  TRIGGER_DELAY_OFF,
  TRIGGER_DELAY_QUEUE, // Queue up to kMaxDelayedTriggers delays, additional triggers ignored
  TRIGGER_DELAY_RING,  // Queue up to kMaxDelayedTriggers delays, additional triggers overwrite oldest
  TRIGGER_DELAY_LAST
};

class EnvelopeGenerator : public settings::SettingsBase<EnvelopeGenerator, ENV_SETTING_LAST> {
public:

  static constexpr int kMaxSegments = 4;
  static constexpr int kEuclideanParams = 3;
  static constexpr int kDelayParams = 1;
  static constexpr size_t kMaxDelayedTriggers = 32;

  struct DelayedTrigger {
    uint32_t delay;
    uint32_t time_left;

    inline void Activate(uint32_t t) {
      delay = time_left = t;
    }

    inline void Reset() {
      delay = time_left = 0;
    }
  };

  void Init(OC::DigitalInput default_trigger);

  EnvelopeType get_type() const {
    return static_cast<EnvelopeType>(values_[ENV_SETTING_TYPE]);
  }

  OC::DigitalInput get_trigger_input() const {
    return static_cast<OC::DigitalInput>(values_[ENV_SETTING_TRIGGER_INPUT]);
  }

  int32_t get_trigger_delay_ms() const {
    return 1000U * values_[ENV_SETTING_TRIGGER_DELAY_SECONDS] + values_[ENV_SETTING_TRIGGER_DELAY_MILLISECONDS] ;
  }

  TriggerDelayMode get_trigger_delay_mode() const {
    return static_cast<TriggerDelayMode>(values_[ENV_SETTING_TRIGGER_DELAY_MODE]);
  }

  uint8_t get_euclidean_length() const {
    return values_[ENV_SETTING_EUCLIDEAN_LENGTH];
  }

  uint8_t get_euclidean_fill() const {
    return values_[ENV_SETTING_EUCLIDEAN_FILL];
  }

  uint8_t get_euclidean_offset() const {
    return values_[ENV_SETTING_EUCLIDEAN_OFFSET];
  }

  // Debug only
  //  uint8_t get_s_euclidean_length() const {
  //    return static_cast<uint8_t>(s_euclidean_length_);
  //  }
  //
  //  uint8_t get_s_euclidean_fill() const {
  //    return static_cast<uint8_t>(s_euclidean_fill_);
  //  }
  //
  //  uint8_t get_s_euclidean_offset() const {
  //    return static_cast<uint8_t>(s_euclidean_offset_);
  //  }

  int get_trigger_delay_count() const {
    return values_[ENV_SETTING_TRIGGER_DELAY_COUNT];
  }

  CVMapping get_cv1_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV1]);
  }

  CVMapping get_cv2_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV2]);
  }

  CVMapping get_cv3_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV3]);
  }

  CVMapping get_cv4_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV4]);
  }

  bool get_hard_reset() const {
    return values_[ENV_SETTING_HARD_RESET];
  }

  bool get_gate_high() const {
    return values_[ENV_SETTING_GATE_HIGH];
  }

  peaks::EnvelopeShape get_attack_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_ATTACK_SHAPE]);
  }

  peaks::EnvelopeShape get_decay_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_DECAY_SHAPE]);
  }

  peaks::EnvelopeShape get_release_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_RELEASE_SHAPE]);
  }

  // Utils
  uint16_t get_segment_value(int segment) const {
    return values_[ENV_SETTING_SEG1_VALUE + segment];
  }

  int num_editable_segments() const {
    switch (get_type()) {
      case ENV_TYPE_AD:
      case ENV_TYPE_AR:
      case ENV_TYPE_AD_LOOP:
        return 2;
      case ENV_TYPE_ADR:
      case ENV_TYPE_ADSR:
      case ENV_TYPE_ADSAR:
      case ENV_TYPE_ADAR:
      case ENV_TYPE_ADR_LOOP:
      case ENV_TYPE_ADAR_LOOP:
        return 4;
      default: break;
    }
    return 0;
  }

  inline void apply_cv_mapping(EnvelopeSettings cv_setting, const int32_t cvs[ADC_CHANNEL_LAST], int32_t segments[kMaxSegments + kEuclideanParams + kDelayParams]) {
    int mapping = values_[cv_setting];
    switch (mapping) {
      case CV_MAPPING_SEG1:
      case CV_MAPPING_SEG2:
      case CV_MAPPING_SEG3:
      case CV_MAPPING_SEG4:
        segments[mapping - CV_MAPPING_SEG1] += (cvs[cv_setting - ENV_SETTING_CV1] * 65536) >> 12;
        break;
      case CV_MAPPING_EUCLIDEAN_LENGTH:
      case CV_MAPPING_EUCLIDEAN_FILL:
      case CV_MAPPING_EUCLIDEAN_OFFSET:
        segments[mapping - CV_MAPPING_SEG1] += cvs[cv_setting - ENV_SETTING_CV1]  >> 6;
        break;
      case CV_MAPPING_DELAY_MSEC:
        segments[mapping - CV_MAPPING_SEG1] += cvs[cv_setting - ENV_SETTING_CV1]  >> 2;
        break;
       default:
        break;
    }
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  EnvelopeSettings enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    EnvelopeSettings *settings = enabled_settings_;

    *settings++ = ENV_SETTING_TYPE;
    *settings++ = ENV_SETTING_TRIGGER_INPUT;
    *settings++ = ENV_SETTING_TRIGGER_DELAY_MODE;
    if (get_trigger_delay_mode()) {
      *settings++ = ENV_SETTING_TRIGGER_DELAY_COUNT;
      *settings++ = ENV_SETTING_TRIGGER_DELAY_MILLISECONDS;
      *settings++ = ENV_SETTING_TRIGGER_DELAY_SECONDS;
    }
    
    *settings++ = ENV_SETTING_EUCLIDEAN_LENGTH;
    if (get_euclidean_length()) {
      *settings++ = ENV_SETTING_EUCLIDEAN_FILL;
      *settings++ = ENV_SETTING_EUCLIDEAN_OFFSET;
    }

    *settings++ = ENV_SETTING_ATTACK_SHAPE;
    *settings++ = ENV_SETTING_DECAY_SHAPE;
    *settings++ = ENV_SETTING_RELEASE_SHAPE;

    *settings++ = ENV_SETTING_CV1;
    *settings++ = ENV_SETTING_CV2;
    *settings++ = ENV_SETTING_CV3;
    *settings++ = ENV_SETTING_CV4;
    *settings++ = ENV_SETTING_HARD_RESET;
    *settings++ = ENV_SETTING_GATE_HIGH;

    num_enabled_settings_ = settings - enabled_settings_;
  }

  static bool indentSetting(EnvelopeSettings setting) {
    switch (setting) {
      case ENV_SETTING_TRIGGER_DELAY_COUNT:
      case ENV_SETTING_TRIGGER_DELAY_SECONDS:
      case ENV_SETTING_TRIGGER_DELAY_MILLISECONDS:
      case ENV_SETTING_EUCLIDEAN_FILL:
      case ENV_SETTING_EUCLIDEAN_OFFSET:
        return true;
      default:
      break;
    }
    return false;
  }

  template <DAC_CHANNEL dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[ADC_CHANNEL_LAST]) {

    int32_t s[kMaxSegments + kEuclideanParams + kDelayParams];
    s[0] = SCALE8_16(static_cast<int32_t>(get_segment_value(0)));
    s[1] = SCALE8_16(static_cast<int32_t>(get_segment_value(1)));
    s[2] = SCALE8_16(static_cast<int32_t>(get_segment_value(2)));
    s[3] = SCALE8_16(static_cast<int32_t>(get_segment_value(3)));
    s[4] = static_cast<int32_t>(get_euclidean_length());
    s[5] = static_cast<int32_t>(get_euclidean_fill());
    s[6] = static_cast<int32_t>(get_euclidean_offset());
    s[7] = get_trigger_delay_ms();

    apply_cv_mapping(ENV_SETTING_CV1, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV2, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV3, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV4, cvs, s);

    s[0] = USAT16(s[0]);
    s[1] = USAT16(s[1]);
    s[2] = USAT16(s[2]);
    s[3] = USAT16(s[3]);
    CONSTRAIN(s[4], 0, 31);
    CONSTRAIN(s[5], 0, 32);
    CONSTRAIN(s[6], 0, 32);
    CONSTRAIN(s[7], 0, 65535);

    // debug only
    // s_euclidean_length_ = s[4];
    // s_euclidean_fill_ = s[5];
    // s_euclidean_offset_ = s[6];

    EnvelopeType type = get_type();
    switch (type) {
      case ENV_TYPE_AD: env_.set_ad(s[0], s[1]); break;
      case ENV_TYPE_ADSR: env_.set_adsr(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADR: env_.set_adr(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_AR: env_.set_ar(s[0], s[1]); break;
      case ENV_TYPE_ADSAR: env_.set_adsar(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADAR: env_.set_adar(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_AD_LOOP: env_.set_ad_loop(s[0], s[1]); break;
      case ENV_TYPE_ADR_LOOP: env_.set_adr_loop(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADAR_LOOP: env_.set_adar_loop(s[0], s[1], s[2]>>1, s[3]); break;
      default:
      break;
    }

    if (type != last_type_) {
      last_type_ = type;
      env_.reset();
    }

    // hard reset forces the envelope to start at level_[0] on rising gate.
    env_.set_hard_reset(get_hard_reset());

    // set the envelope segment shapes
    env_.set_attack_shape(get_attack_shape());
    env_.set_decay_shape(get_decay_shape());
    env_.set_release_shape(get_release_shape());

    OC::DigitalInput trigger_input = get_trigger_input();
    bool triggered = triggers & DIGITAL_INPUT_MASK(trigger_input);

    trigger_display_.Update(1, triggered || gate_raised_);

    if (triggered) ++euclidean_counter_;
    uint8_t euclidean_length = static_cast<uint8_t>(s[4]);
    uint8_t euclidean_fill = static_cast<uint8_t>(s[5]);
    uint8_t euclidean_offset = static_cast<uint8_t>(s[6]);
    if (get_euclidean_length() && !EuclideanFilter(euclidean_length, euclidean_fill, euclidean_offset, euclidean_counter_)) {
      triggered = false;
    }
      
    if (triggered) {
      TriggerDelayMode delay_mode = get_trigger_delay_mode();
      // uint32_t delay = get_trigger_delay_ms() * 1000U;
      uint32_t delay = static_cast<uint32_t>(s[7] * 1000U);
      if (delay_mode && delay) {
        triggered = false;
        if (TRIGGER_DELAY_QUEUE == delay_mode) {
          if (delayed_triggers_free_ < get_trigger_delay_count())
            delayed_triggers_[delayed_triggers_free_].Activate(delay);
        } else { // TRIGGER_DELAY_RING
          // Assume these are mostly in order, so the "next" is also the oldest
          if (delayed_triggers_free_ < get_trigger_delay_count())
            delayed_triggers_[delayed_triggers_free_].Activate(delay);
          else
            delayed_triggers_[delayed_triggers_next_].Activate(delay);
        }
      }
    }

    if (DelayedTriggers())
      triggered = true;

    uint8_t gate_state = 0;
    if (triggered)
      gate_state |= peaks::CONTROL_GATE_RISING;
 
    bool gate_raised = OC::DigitalInputs::read_immediate(trigger_input);
    if (gate_raised || get_gate_high())
      gate_state |= peaks::CONTROL_GATE;
    else if (gate_raised_)
      gate_state |= peaks::CONTROL_GATE_FALLING;
    gate_raised_ = gate_raised;

    // TODO Scale range or offset?
    uint32_t value = OC::DAC::get_zero_offset(dac_channel) + env_.ProcessSingleSample(gate_state);
    OC::DAC::set<dac_channel>(value);
  }

  uint16_t RenderPreview(int16_t *values, uint16_t *segment_start_points, uint16_t *loop_points, uint16_t &current_phase) const {
    return env_.RenderPreview(values, segment_start_points, loop_points, current_phase);
  }

  uint16_t RenderFastPreview(int16_t *values) const {
    return env_.RenderFastPreview(values);
  }

  uint8_t getTriggerState() const {
    return trigger_display_.getState();
  }

  inline void get_next_trigger(DelayedTrigger &trigger) const {
    trigger = delayed_triggers_[delayed_triggers_next_];
  }

private:

  peaks::MultistageEnvelope env_;
  EnvelopeType last_type_;
  bool gate_raised_;
  uint32_t euclidean_counter_;
  // debug only
  //  int32_t s_euclidean_length_;  
  //  int32_t s_euclidean_fill_;  
  //  int32_t s_euclidean_offset_;  
  
  DelayedTrigger delayed_triggers_[kMaxDelayedTriggers];
  size_t delayed_triggers_free_;
  size_t delayed_triggers_next_;

  int num_enabled_settings_;
  EnvelopeSettings enabled_settings_[ENV_SETTING_LAST];

  OC::DigitalInputDisplay trigger_display_;

  bool DelayedTriggers() {
    bool triggered = false;

    delayed_triggers_free_ = kMaxDelayedTriggers;
    delayed_triggers_next_ = 0;
    uint32_t min_time_left = -1;

    size_t i = kMaxDelayedTriggers;
    while (i--) {
      DelayedTrigger &trigger = delayed_triggers_[i];
      uint32_t time_left = trigger.time_left;
      if (time_left) {
        if (time_left > OC_CORE_TIMER_RATE) {
          time_left -= OC_CORE_TIMER_RATE;
          if (time_left < min_time_left) {
            min_time_left = time_left;
            delayed_triggers_next_ = i;
          }
          trigger.time_left = time_left;
        } else {
          trigger.Reset();
          delayed_triggers_free_ = i;
          triggered = true;
        }
      } else {
        delayed_triggers_free_ = i;
      }
    }

    return triggered;
  }
};

void EnvelopeGenerator::Init(OC::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(ENV_SETTING_TRIGGER_INPUT, default_trigger);
  env_.Init();
  last_type_ = ENV_TYPE_LAST;
  gate_raised_ = false;
  euclidean_counter_ = 0;

  memset(delayed_triggers_, 0, sizeof(delayed_triggers_));
  delayed_triggers_free_ = delayed_triggers_next_ = 0;

  trigger_display_.Init();

  update_enabled_settings();
}

const char* const envelope_types[ENV_TYPE_LAST] = {
  "AD", "ADSR", "ADR", "ASR", "ADSAR", "ADAR", "AD loop", "ADR loop", "ADAR loop"
};

const char* const segment_names[] = {
  "Attack", "Decay", "Sustain/Level", "Release"
};

const char* const envelope_shapes[peaks::ENV_SHAPE_LAST] = {
  "Lin", "Exp", "Quart", "Sine", "Ledge", "Cliff", "Gate", "BgDip", "MeDip", "LtDip", "Wiggl"
};

const char* const cv_mapping_names[CV_MAPPING_LAST] = {
  "None", "Att", "Dec", "Sus", "Rel", "Eleng", "Efill", "Eoffs", "Delay"
};

const char* const trigger_delay_modes[TRIGGER_DELAY_LAST] = {
  "Off", "Queue", "Ring"
};

const char* const euclidean_lengths[] = {
  "Off", "  2", "  3", "  4", "  5", "  6", "  7", "  8", "  9", " 10",
  " 11", " 12", " 13", " 14", " 15", " 16", " 17", " 18", " 19", " 20",
  " 21", " 12", " 23", " 24", " 25", " 26", " 27", " 28", " 29", " 30",
  " 31", " 32",
};

SETTINGS_DECLARE(EnvelopeGenerator, ENV_SETTING_LAST) {
  { ENV_TYPE_AD, ENV_TYPE_FIRST, ENV_TYPE_LAST-1, "TYPE", envelope_types, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "S1", NULL, settings::STORAGE_TYPE_U16 }, // u16 in case resolution proves insufficent
  { 128, 0, 255, "S2", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S3", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S4", NULL, settings::STORAGE_TYPE_U16 },
  { OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_4, "Trigger input", OC::Strings::trigger_input_names, settings::STORAGE_TYPE_U4 },
  { TRIGGER_DELAY_OFF, TRIGGER_DELAY_OFF, TRIGGER_DELAY_LAST - 1, "Tr delay mode", trigger_delay_modes, settings::STORAGE_TYPE_U4 },
  { 1, 1, EnvelopeGenerator::kMaxDelayedTriggers, "Tr delay count", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 999, "Tr delay msecs", NULL, settings::STORAGE_TYPE_U16 },
  { 0, 0, 64, "Tr delay secs", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 31, "Eucl length", euclidean_lengths, settings::STORAGE_TYPE_U8 },
  { 1, 0, 32, "Eucl fill", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 32, "Eucl offset", NULL, settings::STORAGE_TYPE_U8 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_LAST - 1, "CV1 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_LAST - 1, "CV2 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_LAST - 1, "CV3 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_LAST - 1, "CV4 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Hard reset", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Gate high", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_QUARTIC, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Attack shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Decay shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Release shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
};

class QuadEnvelopeGenerator {
public:
  static constexpr int32_t kCvSmoothing = 16;

  void Init() {
    int input = OC::DIGITAL_INPUT_1;
    for (auto &env : envelopes_) {
      env.Init(static_cast<OC::DigitalInput>(input));
      ++input;
    }

    ui.edit_mode = MODE_EDIT_SEGMENTS;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.segment_editing = false;
    ui.cursor.Init(0, envelopes_[0].num_enabled_settings() - 1);
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    const int32_t cvs[ADC_CHANNEL_LAST] = { cv1.value(), cv2.value(), cv3.value(), cv4.value() };
    uint32_t triggers = OC::DigitalInputs::clocked();

    envelopes_[0].Update<DAC_CHANNEL_A>(triggers, cvs);
    envelopes_[1].Update<DAC_CHANNEL_B>(triggers, cvs);
    envelopes_[2].Update<DAC_CHANNEL_C>(triggers, cvs);
    envelopes_[3].Update<DAC_CHANNEL_D>(triggers, cvs);
  }

  enum EnvEditMode {
    MODE_EDIT_SEGMENTS,
    MODE_EDIT_SETTINGS
  };

  struct {
    EnvEditMode edit_mode;

    int selected_channel;
    int selected_segment;
    bool segment_editing;

    menu::ScreenCursor<menu::kScreenLines> cursor;
  } ui;

  EnvelopeGenerator &selected() {
    return envelopes_[ui.selected_channel];
  }

  EnvelopeGenerator envelopes_[4];

  SmoothedValue<int32_t, kCvSmoothing> cv1;
  SmoothedValue<int32_t, kCvSmoothing> cv2;
  SmoothedValue<int32_t, kCvSmoothing> cv3;
  SmoothedValue<int32_t, kCvSmoothing> cv4;
};

QuadEnvelopeGenerator envgen;

void ENVGEN_init() {
  envgen.Init();
}

size_t ENVGEN_storageSize() {
  return 4 * EnvelopeGenerator::storageSize();
}

size_t ENVGEN_save(void *storage) {
  size_t s = 0;
  for (auto &env : envgen.envelopes_)
    s += env.Save(static_cast<byte *>(storage) + s);
  return s;
}

size_t ENVGEN_restore(const void *storage) {
  size_t s = 0;
  for (auto &env : envgen.envelopes_) {
    s += env.Restore(static_cast<const byte *>(storage) + s);
    env.update_enabled_settings();
  }

  envgen.ui.cursor.AdjustEnd(envgen.envelopes_[0].num_enabled_settings() - 1);
  return s;
}

void ENVGEN_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void ENVGEN_loop() {
}

static constexpr weegfx::coord_t kPreviewH = 32;
static constexpr weegfx::coord_t kPreviewTopY = 32;
static constexpr weegfx::coord_t kPreviewBottomY = 32 + kPreviewH - 1;

static constexpr weegfx::coord_t kLoopMarkerY = 28;
static constexpr weegfx::coord_t kCurrentSegmentCursorY = 26;

int16_t preview_values[128 + 64];
uint16_t preview_segment_starts[peaks::kMaxNumSegments];
uint16_t preview_loop_points[peaks::kMaxNumSegments];
static constexpr uint16_t kPreviewTerminator = 0xffff;

settings::value_attr segment_editing_attr = { 128, 0, 255, "DOH!", NULL, settings::STORAGE_TYPE_U16 };

void ENVGEN_menu_preview() {
  auto const &env = envgen.selected();

  menu::SettingsListItem list_item;
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX>::AbsoluteLine(0, list_item);
  list_item.selected = false;
  list_item.editing = envgen.ui.segment_editing;
  const int selected_segment = envgen.ui.selected_segment;

  segment_editing_attr.name = segment_names[selected_segment];
  list_item.DrawDefault(env.get_segment_value(selected_segment), segment_editing_attr);

  // Current envelope shape
  uint16_t current_phase = 0;
  weegfx::coord_t x = 0;
  weegfx::coord_t w = env.RenderPreview(preview_values, preview_segment_starts, preview_loop_points, current_phase);
  const int16_t *data = preview_values;
  while (x <= static_cast<weegfx::coord_t>(current_phase)) {
    const int16_t value = *data++ >> 10;
    graphics.drawVLine(x++, kPreviewBottomY - value, value + 1);
  }

  while (x < w) {
    const int16_t value = *data++ >> 10;
    graphics.setPixel(x++, kPreviewBottomY - value);
  }

  if (x < menu::kDisplayWidth)
    graphics.drawHLine(x, kPreviewBottomY, menu::kDisplayWidth - x);

  // Minimal cursor thang (x is end of preview)
  weegfx::coord_t start = preview_segment_starts[selected_segment];
  weegfx::coord_t end = preview_segment_starts[selected_segment + 1];
  w = kPreviewTerminator == end ? x - start + 1 : end - start + 1;
  if (w < 4) w = 4;
  graphics.drawRect(start, kCurrentSegmentCursorY, w, 2);

  // Current types only loop over full envelope, so just pixel dust
  uint16_t *loop_points = preview_loop_points;
  uint_fast8_t i = 0;
  while (*loop_points != kPreviewTerminator) {
    // odd: end marker, even: start marker
    if (i++ & 1)
      graphics.drawBitmap8(*loop_points++ - 1, kLoopMarkerY, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW);
    else
      graphics.drawBitmap8(*loop_points++, kLoopMarkerY, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8);
  }

  // Brute-force way of handling "pathological" cases where A/D has no visible
  // pixels instead of line-drawing between points
  uint16_t *segment_start = preview_segment_starts;
  while (*segment_start != kPreviewTerminator) {
    weegfx::coord_t x = *segment_start++;
    weegfx::coord_t value = preview_values[x] >> 10;
    graphics.drawVLine(x, kPreviewBottomY - value, value);
  }
}

void ENVGEN_menu_settings() {
  auto const &env = envgen.selected();

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(envgen.ui.cursor);
  menu::SettingsListItem list_item;

  while (settings_list.available()) {
    const int setting = 
      env.enabled_setting_at(settings_list.Next(list_item));
    const int value = env.get_value(setting);
    const settings::value_attr &attr = EnvelopeGenerator::value_attr(setting);

    switch (setting) {
      case ENV_SETTING_TYPE:
        list_item.SetPrintPos();
        if (list_item.editing) {
          menu::DrawEditIcon(6, list_item.y, value, attr);
          graphics.movePrintPos(6, 0);
        }
        graphics.print(attr.value_names[value]);
        list_item.DrawCustom();
      break;
      default:
        if (EnvelopeGenerator::indentSetting(static_cast<EnvelopeSettings>(setting)))
          list_item.x += menu::kIndentDx;
        list_item.DrawDefault(value, attr);
      break;
    }
  }
}

void ENVGEN_menu() {

  menu::QuadTitleBar::Draw();
  for (uint_fast8_t i = 0; i < 4; ++i) {
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
    menu::QuadTitleBar::DrawGateIndicator(i, envgen.envelopes_[i].getTriggerState());


    EnvelopeGenerator::DelayedTrigger trigger;
    envgen.envelopes_[i].get_next_trigger(trigger);
    if (trigger.delay) {
      weegfx::coord_t x = menu::QuadTitleBar::ColumnStartX(i) + 28;
      weegfx::coord_t h = (trigger.time_left * 8) / trigger.delay;
      graphics.drawRect(x, menu::QuadTitleBar::kTextY + 7 - h, 2, 1 + h);
    }
  }
  // If settings mode, draw level in title bar?
  menu::QuadTitleBar::Selected(envgen.ui.selected_channel);

  if (QuadEnvelopeGenerator::MODE_EDIT_SEGMENTS == envgen.ui.edit_mode)
    ENVGEN_menu_preview();
  else
    ENVGEN_menu_settings();
}

void ENVGEN_topButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, 32);
}

void ENVGEN_lowerButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, -32); 
}

void ENVGEN_rightButton() {

  if (QuadEnvelopeGenerator::MODE_EDIT_SEGMENTS == envgen.ui.edit_mode) {
    envgen.ui.segment_editing = !envgen.ui.segment_editing;
  } else {
    envgen.ui.cursor.toggle_editing();
  }
}

void ENVGEN_leftButton() {
  if (QuadEnvelopeGenerator::MODE_EDIT_SETTINGS == envgen.ui.edit_mode) {
    envgen.ui.edit_mode = QuadEnvelopeGenerator::MODE_EDIT_SEGMENTS;
    envgen.ui.cursor.set_editing(false);
  } else {
    envgen.ui.edit_mode = QuadEnvelopeGenerator::MODE_EDIT_SETTINGS;
    envgen.ui.segment_editing = false;
  }
}

void ENVGEN_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        ENVGEN_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        ENVGEN_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        ENVGEN_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        ENVGEN_rightButton();
        break;
    }
  }
}

void ENVGEN_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    int left_value = envgen.ui.selected_channel + event.value;
    CONSTRAIN(left_value, 0, 3);
    envgen.ui.selected_channel = left_value;
    auto &selected_env = envgen.selected();
    CONSTRAIN(envgen.ui.selected_segment, 0, selected_env.num_editable_segments() - 1);
    envgen.ui.cursor.AdjustEnd(selected_env.num_enabled_settings() - 1);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (QuadEnvelopeGenerator::MODE_EDIT_SEGMENTS == envgen.ui.edit_mode) {
      auto &selected_env = envgen.selected();
      if (envgen.ui.segment_editing) {
        selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, event.value);
      } else {
        int selected_segment = envgen.ui.selected_segment + event.value;
        CONSTRAIN(selected_segment, 0, selected_env.num_editable_segments() - 1);
        envgen.ui.selected_segment = selected_segment;
      }
    } else {
      if (envgen.ui.cursor.editing()) {
        auto &selected_env = envgen.selected();
        EnvelopeSettings setting = selected_env.enabled_setting_at(envgen.ui.cursor.cursor_pos());
        selected_env.change_value(setting, event.value);
        if (ENV_SETTING_TRIGGER_DELAY_MODE == setting || ENV_SETTING_EUCLIDEAN_LENGTH == setting)
          selected_env.update_enabled_settings();
      } else {
        envgen.ui.cursor.Scroll(event.value);
      }
    }
  }
}

int16_t fast_preview_values[peaks::kFastPreviewWidth + 32];

template <int index, weegfx::coord_t startx, weegfx::coord_t y>
void RenderFastPreview() {
  uint16_t w = envgen.envelopes_[index].RenderFastPreview(fast_preview_values);
  CONSTRAIN(w, 0, peaks::kFastPreviewWidth); // Just-in-case
  weegfx::coord_t x = startx;
  const int16_t *values = fast_preview_values;
  while (w--) {
    const int16_t value = 1 + ((*values++ >> 10) & 0x1f);
    graphics.drawVLine(x++, y + 32 - value, value);
  }
}

void ENVGEN_screensaver() {
#ifdef ENVGEN_DEBUG_SCREENSAVER
  debug::CycleMeasurement render_cycles;
#endif

  RenderFastPreview<0, 0, 0>();
  RenderFastPreview<1, 64, 0>();
  RenderFastPreview<2, 0, 32>();
  RenderFastPreview<3, 64, 32>();

  OC::scope_render();

#ifdef ENVGEN_DEBUG_SCREENSAVER
  uint32_t us = debug::cycles_to_us(render_cycles.read());
  graphics.setPrintPos(2, 56);
  graphics.printf("%u",  us);
#endif
}

//void ENVGEN_debug() {
//  for (int i = 0; i < 4; ++i) { 
//    uint8_t ypos = 10*(i + 1) + 2 ; 
//    graphics.setPrintPos(2, ypos);
//    graphics.print(envgen.envelopes_[i].get_s_euclidean_length()) ;
//    graphics.setPrintPos(50, ypos);
//    graphics.print(envgen.envelopes_[i].get_s_euclidean_fill()) ;
//    graphics.setPrintPos(100, ypos);
//    graphics.print(envgen.envelopes_[i].get_s_euclidean_offset()) ;
//  }
//}

void FASTRUN ENVGEN_isr() {
  envgen.ISR();
}
