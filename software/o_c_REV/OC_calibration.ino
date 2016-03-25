/*
*
* calibration menu:
*
* enter by pressing left encoder button during start up; use encoder switches to navigate.
*
*/

#include "OC_calibration.h"

static constexpr uint16_t DAC_OFFSET = 4890; // DAC offset, initial approx., ish --> -3.5V to 6V
static constexpr uint16_t _ADC_OFFSET = (uint16_t)((float)pow(2,OC::ADC::kAdcResolution)*0.6666667f); // ADC offset

namespace OC {
CalibrationStorage calibration_storage;
CalibrationData calibration_data;
};

uint16_t semitones[SEMITONES];          // DAC output LUT

static constexpr unsigned kCalibrationAdcSmoothing = 4;
const OC::CalibrationData kCalibrationDefaults = {
  // DAC
  { {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535},
    {0, 0, 0, 0 } },
  // ADC
  { { _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET }, 0, 0 },
  // display_offset
  SH1106_128x64_Driver::kDefaultOffset,
  0, // flags
  0, 0 // reserved
};
//const uint16_t THEORY[OCTAVES+1] = {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535}; // in theory  

void calibration_reset() {
  memcpy(&OC::calibration_data, &kCalibrationDefaults, sizeof(OC::calibration_data));
  for (int i = 0; i < OCTAVES; ++i)
    OC::calibration_data.dac.octaves[i] += DAC_OFFSET;
}

void calibration_load() {
  SERIAL_PRINTLN("CalibrationStorage: PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                 OC::CalibrationStorage::PAGESIZE, OC::CalibrationStorage::PAGES, OC::CalibrationStorage::LENGTH);

  calibration_reset();
  if (!OC::calibration_storage.Load(OC::calibration_data)) {
    if (EEPROM.read(0x2) > 0) {
      SERIAL_PRINTLN("Calibration not loaded, non-zero data found, trying to import...");
      calibration_read_old();
    } else {
      SERIAL_PRINTLN("No calibration data found, using defaults");
    }
  } else {
    SERIAL_PRINTLN("Calibration data loaded...");
  }
}

void calibration_save() {
  SERIAL_PRINTLN("Saving calibration data...");
  OC::calibration_storage.Save(OC::calibration_data);
}

enum CALIBRATION_STEP {  
  HELLO,
  CENTER_DISPLAY,
  VOLT_0,
  VOLT_3m, VOLT_2m, VOLT_1m, VOLT_0m,
  VOLT_1, VOLT_2, VOLT_3, VOLT_4, VOLT_5, VOLT_6,
  DAC_FINE_A, DAC_FINE_B, DAC_FINE_C, DAC_FINE_D,
  CV_OFFSET,
  CV_OFFSET_0, CV_OFFSET_1, CV_OFFSET_2, CV_OFFSET_3,
  CV_SCALE_1V, CV_SCALE_3V,
  CALIBRATION_EXIT,
  CALIBRATION_STEP_LAST,
  CALIBRARION_STEP_FINAL = CV_SCALE_3V
};  

enum CALIBRATION_TYPE {
  CALIBRATE_NONE,
  CALIBRATE_OCTAVE,
  CALIBRATE_DAC_FINE,
  CALIBRATE_ADC_TRIMMER,
  CALIBRATE_ADC_OFFSET,
  CALIBRATE_DISPLAY
};

struct CalibrationStep {
  CALIBRATION_STEP step;
  const char *title;
  const char *message;
  const char *help; // optional
  const char *footer;

  CALIBRATION_TYPE calibration_type;
  int type_index;

  const char * const *value_str; // if non-null, use these instead of encoder value
  int min, max;
};

struct CalibrationState {
  CALIBRATION_STEP step;
  const CalibrationStep *current_step;
  int encoder_value;
  uint32_t CV;
};

OC::DigitalInputDisplay digital_input_displays[4];

// 128/6=21                  |                     |
const char *start_footer   = "              [START]";
const char *end_footer     = "[PREV]         [EXIT]";
const char *default_footer = "[PREV]         [NEXT]";
const char *default_help_r = "[R] => Adjust";
const char *select_help    = "[R] => Select";

const CalibrationStep calibration_steps[CALIBRATION_STEP_LAST] = {
  { HELLO, "O&C Calibration", "Use defaults? ", select_help, start_footer, CALIBRATE_NONE, 0, OC::Strings::no_yes, 0, 1 },
  { CENTER_DISPLAY, "Center Display", "Pixel offset ", default_help_r, default_footer, CALIBRATE_DISPLAY, 0, nullptr, 0, 2 },
  { VOLT_0, "DAC 0 volts", "->  0.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, _ZERO, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_3m, "DAC -3 volts", "-> -3.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 0, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_2m, "DAC -2 volts", "-> -2.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 1, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_1m, "DAC -1 volts", "-> -1.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 2, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_0, "DAC 0 volts", "->  0.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 3, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_1, "DAC 1 volts", "->  1.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 4, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_2, "DAC 2 volts", "->  2.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 5, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_3, "DAC 3 volts", "->  3.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 6, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_4, "DAC 4 volts", "->  4.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 7, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_5, "DAC 5 volts", "->  5.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 8, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_6, "DAC 6 volts", "->  6.000V ", default_help_r, default_footer, CALIBRATE_OCTAVE, 9, nullptr, 0, DAC::MAX_VALUE },

  { DAC_FINE_A, "DAC A fine", "->  1.000v ", default_help_r, default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_A, nullptr, -128, 127 },
  { DAC_FINE_B, "DAC B fine", "->  1.000v ", default_help_r, default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_B, nullptr, -128, 127 },
  { DAC_FINE_C, "DAC C fine", "->  1.000v ", default_help_r, default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_C, nullptr, -128, 127 },
  { DAC_FINE_D, "DAC D fine", "->  1.000v ", default_help_r, default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_D, nullptr, -128, 127 },

  { CV_OFFSET, "CV offset", "", "Adjust CV trimpot", default_footer, CALIBRATE_ADC_TRIMMER, 0, nullptr, 0, 4095 },
  { CV_OFFSET_0, "CV1 (sample)", "", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_1, nullptr, 0, 4095 },
  { CV_OFFSET_1, "CV2 (index)", "", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_2, nullptr, 0, 4095 },
  { CV_OFFSET_2, "CV3 (notes/scale)", "", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_3, nullptr, 0, 4095 },
  { CV_OFFSET_3, "CV4 (transpose)", "", default_help_r, default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_4, nullptr, 0, 4095 },

  { CV_SCALE_1V, "CV Scaling 1V", "TODO", "Lorem ipsum", default_footer, CALIBRATE_NONE, 0, nullptr, 0, 0 },
  { CV_SCALE_3V, "CV Scaling 3V", "TODO", "Lorem ipsum", default_footer, CALIBRATE_NONE, 0, nullptr, 0, 0 },

  { CALIBRATION_EXIT, "Calibration complete", "Save values? ", select_help, end_footer, CALIBRATE_NONE, 0, OC::Strings::no_yes, 0, 1 }
};

/* make DAC table from calibration values */ 

void init_DACtable() {
  SERIAL_PRINTLN("Initializing semitones LUT...");

  float _diff, _offset, _semitone;
  _offset = OC::calibration_data.dac.octaves[OCTAVES-2];          // = 5v
  semitones[SEMITONES-1] = OC::calibration_data.dac.octaves[OCTAVES-1]; // = 6v
  
  // 6v to -3v:
  for (int i = OCTAVES-1; i > 0; i--) {
    
      _diff = (float)(OC::calibration_data.dac.octaves[i] - _offset)/12.0f;
      LUT_PRINTF("%d --> %.4f\n", i, _diff);
  
      for (int j = 0; j <= 11; j++) {
        
           _semitone = j*_diff + _offset; 
           semitones[j+i*12-1] = (uint16_t)(0.5f + _semitone);
           if (j == 11 && i > 1) _offset = OC::calibration_data.dac.octaves[i-2];

           LUT_PRINTF("%2d: %.4f -> %d -> %u\n", j, _offset, j+i*12-1, semitones[j+i*12-1]);
      } 
   }
   // fill up remaining semitones (from -3.000v )
   int _fill = 10;
   while (_fill >= 0) {
     
          if (_offset > _diff*2) { 
              semitones[_fill] = _offset - _diff;
              _offset -= _diff;   
          }
          else semitones[_fill] = _diff;

         LUT_PRINTF("%.4f -> %d -> %u\n", _offset, _fill, semitones[_fill]);
         _fill--;    
   }
}

/*     loop calibration menu until done       */
void OC::Ui::Calibrate() {

  // Calibration data should be loaded (or defaults) by now
  SERIAL_PRINTLN("Starting calibration...");

  CalibrationState calibration_state = {
    HELLO,
    &calibration_steps[HELLO],
    1,
    0
  };
  for (auto &did : digital_input_displays)
    did.Init();

  TickCount tick_count;
  tick_count.Init();

  bool calibration_complete = false;
  while (!calibration_complete) {

    uint32_t ticks = tick_count.Update();
    digital_input_displays[0].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_1>());
    digital_input_displays[1].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_2>());
    digital_input_displays[2].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_3>());
    digital_input_displays[3].Update(ticks, DigitalInputs::read_immediate<DIGITAL_INPUT_4>());

    while (event_queue_.available()) {
      const UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;

      switch (event.control) {
        case CONTROL_BUTTON_L:
          if (calibration_state.step > CENTER_DISPLAY)
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step - 1);
          break;
        case CONTROL_BUTTON_R:
          if (calibration_state.step < CALIBRATION_EXIT)
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step + 1);
          else
            calibration_complete = true;
          break;
        case CONTROL_ENCODER_L:
          if (calibration_state.step > HELLO) {
            calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step + event.value);
            CONSTRAIN(calibration_state.step, CENTER_DISPLAY, CALIBRATION_EXIT);
          }
          break;
        case CONTROL_ENCODER_R:
          calibration_state.encoder_value += event.value;
          break;
        default:
          break;
      }
    }

    const CalibrationStep *next_step = &calibration_steps[calibration_state.step];
    if (next_step != calibration_state.current_step) {
      SERIAL_PRINTLN("Step: %s", next_step->title);
      // Special cases on exit current step
      switch (calibration_state.current_step->step) {
        case HELLO:
          if (calibration_state.encoder_value) {
            SERIAL_PRINTLN("Resetting to defaults...");
            calibration_reset();
          }
          break;
        default: break;
      }

      // Setup next step
      switch (next_step->calibration_type) {
      case CALIBRATE_OCTAVE:
        calibration_state.encoder_value = OC::calibration_data.dac.octaves[next_step->type_index];
        break;
      case CALIBRATE_DAC_FINE:
        calibration_state.encoder_value = OC::calibration_data.dac.fine[next_step->type_index];
        break;
      case CALIBRATE_ADC_TRIMMER:
        break;
      case CALIBRATE_ADC_OFFSET:
        calibration_state.encoder_value = OC::calibration_data.adc.offset[next_step->type_index];
        break;
      case CALIBRATE_DISPLAY:
        calibration_state.encoder_value = OC::calibration_data.display_offset;
        break;

      case CALIBRATE_NONE:
      default:
        if (CALIBRATION_EXIT != next_step->step)
          calibration_state.encoder_value = 0;
        else
          calibration_state.encoder_value = 1;
      }
      calibration_state.current_step = next_step;
    }

    calibration_update(calibration_state);
    calibration_draw(calibration_state);
  }

  if (calibration_state.encoder_value) {
    SERIAL_PRINTLN("Calibration complete");
    calibration_save();
  } else {
    SERIAL_PRINTLN("Calibration complete, not saving values...");
  }
}

void calibration_draw(const CalibrationState &state) {
  GRAPHICS_BEGIN_FRAME(true);
  const CalibrationStep *step = state.current_step;

  menu::DefaultTitleBar::Draw();
  graphics.print(step->title);

  weegfx::coord_t y = menu::CalcLineY(0);

  static constexpr weegfx::coord_t kValueX = menu::kDisplayWidth - 30;

  graphics.setPrintPos(menu::kIndentDx, y + 2);
  switch (step->calibration_type) {
    case CALIBRATE_OCTAVE:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.print((int)state.encoder_value, 5);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      break;

    case CALIBRATE_DAC_FINE:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.pretty_print((int)state.encoder_value, 5);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      break;

    case CALIBRATE_ADC_TRIMMER:
      graphics.print(_ADC_OFFSET, 4);
      graphics.print(" == ");
      graphics.print((int)state.CV, 4);
      break;

    case CALIBRATE_ADC_OFFSET:
      graphics.print("0 => ");
      graphics.pretty_print(state.encoder_value - state.CV, 4);
      break;

    case CALIBRATE_DISPLAY:
      graphics.print(step->message);
      graphics.setPrintPos(kValueX, y + 2);
      graphics.pretty_print((int)state.encoder_value, 2);
      menu::DrawEditIcon(kValueX, y, state.encoder_value, step->min, step->max);
      graphics.drawFrame(0, 0, 128, 64);
      break;

    case CALIBRATE_NONE:
    default:
      graphics.setPrintPos(menu::kIndentDx, y + 2);
      graphics.print(step->message);
      if (step->value_str)
        graphics.print(step->value_str[state.encoder_value]);
      break;
  }

  y += menu::kMenuLineH;
  graphics.setPrintPos(menu::kIndentDx, y + 2);
  if (step->help)
    graphics.print(step->help);

  weegfx::coord_t x = menu::kDisplayWidth - 22;
  y = 2;
  for (int input = OC::DIGITAL_INPUT_1; input < OC::DIGITAL_INPUT_LAST; ++input) {
    uint8_t state = (digital_input_displays[input].getState() + 3) >> 2;
    if (state)
      graphics.drawBitmap8(x, y, 4, OC::bitmap_gate_indicators_8 + (state << 2));
    x += 5;
  }

  graphics.drawStr(1, menu::kDisplayHeight - menu::kFontHeight - 3, step->footer);

  static constexpr uint16_t step_width = (menu::kDisplayWidth << 8 ) / (CALIBRATION_STEP_LAST - 1);
  graphics.drawRect(0, menu::kDisplayHeight - 2, (state.step * step_width) >> 8, 2);

  GRAPHICS_END_FRAME();
}

/* DAC output etc */ 

void calibration_update(CalibrationState &state) {

  CONSTRAIN(state.encoder_value, state.current_step->min, state.current_step->max);
  const CalibrationStep *step = state.current_step;

  switch (step->calibration_type) {
    case CALIBRATE_NONE:
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_OCTAVE:
      OC::calibration_data.dac.octaves[step->type_index] = state.encoder_value;
      DAC::set_all(OC::calibration_data.dac.octaves[step->type_index]);
      break;
    case CALIBRATE_DAC_FINE:
      OC::calibration_data.dac.fine[step->type_index] = state.encoder_value;
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO + 1]);
      break;
    case CALIBRATE_ADC_TRIMMER:
      state.CV = _average();
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_ADC_OFFSET:
      state.CV = (state.CV * (kCalibrationAdcSmoothing - 1) + OC::ADC::raw_value(static_cast<ADC_CHANNEL>(step->type_index))) / kCalibrationAdcSmoothing;
      OC::calibration_data.adc.offset[step->type_index] = state.encoder_value;
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_DISPLAY:
      OC::calibration_data.display_offset = state.encoder_value;
      display::AdjustOffset(OC::calibration_data.display_offset);
      break;
  }
}

/* misc */ 

uint16_t _average() {
#if 0
  float average = 0.0f;
  
  for (int i = 0; i < 50; i++) {
   
           average +=  OC::ADC::raw_value(ADC_CHANNEL_1);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_2);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_3);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_4);
           delay(1);
      }
      
  return average / 200.0f;
#endif
  uint32_t sum = 0;
  size_t count = 64;
  while (count--) {
    sum += OC::ADC::raw_value(ADC_CHANNEL_1) + OC::ADC::raw_value(ADC_CHANNEL_2);
    delay(1);
    sum += OC::ADC::raw_value(ADC_CHANNEL_3) + OC::ADC::raw_value(ADC_CHANNEL_4);
    delay(1);
  }

  return sum >> 8;
}

/* read settings from original O&C */

void calibration_read_old() {
  
   delay(1000);
   uint8_t byte0, byte1, adr;
   
   adr = 0; 
   SERIAL_PRINTLN("Loading original O&C calibration from eeprom:");
   
   for (int i = 0; i < OCTAVES; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       OC::calibration_data.dac.octaves[i] = (uint16_t)(byte0 << 8) + byte1;
       SERIAL_PRINTLN(" OCTAVE %2d: %u", i, OC::calibration_data.dac.octaves[i]);
   }
   
   uint16_t _offset[ADC_CHANNEL_LAST];
   
   for (int i = 0; i < ADC_CHANNEL_LAST; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       _offset[i] = (uint16_t)(byte0 << 8) + byte1;
       SERIAL_PRINTLN(" ADC %d: %u", i, _offset[i]);
   }
   
   OC::calibration_data.adc.offset[ADC_CHANNEL_1] = _offset[0];
   OC::calibration_data.adc.offset[ADC_CHANNEL_2] = _offset[1];
   OC::calibration_data.adc.offset[ADC_CHANNEL_3] = _offset[2];
   OC::calibration_data.adc.offset[ADC_CHANNEL_4] = _offset[3];
   SERIAL_PRINTLN("......");
}  
