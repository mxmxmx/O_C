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
  // flags
  0,
  // display_offset
  SH1106_128x64_Driver::kDefaultOffset
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
      calibration_save();
    } else {
      SERIAL_PRINTLN("No calibration data found, using defaults");
    }
  }
}

void calibration_save() {
  SERIAL_PRINTLN("Saving calibration data... DISABLED!");
//  OC::calibration_storage.Save(OC::calibration_data);
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
  EXIT,
  CALIBRATION_STEP_LAST
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
  const char *footer;

  CALIBRATION_TYPE calibration_type;
  int type_index;

  const char * const *value_str; // if non-null, use these instead of encoder value
  long min, max;
};

struct CalibrationState {
  CALIBRATION_STEP step;
  const CalibrationStep *current_step;
  long encoder_data;
  uint32_t CV;
};

OC::DigitalInputDisplay digital_input_displays[4];

const char * default_footer = "[prev]         [next]";

const CalibrationStep calibration_steps[CALIBRATION_STEP_LAST] = {
  { HELLO, "Calibrate", "Use defaults? ", "              [start]", CALIBRATE_NONE, 0, OC::Strings::no_yes, 0, 1 },
  { CENTER_DISPLAY, "Center Display", "offset ", default_footer, CALIBRATE_DISPLAY, 0, nullptr, 0, 2 },
  { VOLT_0, "trim to 0 volts", "->  0.000V ", default_footer, CALIBRATE_OCTAVE, _ZERO, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_3m, "trim to -3 volts", "-> -3.000V ", default_footer, CALIBRATE_OCTAVE, 0, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_2m, "trim to -2 volts", "-> -2.000V ", default_footer, CALIBRATE_OCTAVE, 1, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_1m, "trim to -1 volts", "-> -1.000V ", default_footer, CALIBRATE_OCTAVE, 2, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_0, "trim to 0 volts", "->  0.000V ", default_footer, CALIBRATE_OCTAVE, 3, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_1, "trim to 1 volts", "->  1.000V ", default_footer, CALIBRATE_OCTAVE, 4, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_2, "trim to 2 volts", "->  2.000V ", default_footer, CALIBRATE_OCTAVE, 5, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_3, "trim to 3 volts", "->  3.000V ", default_footer, CALIBRATE_OCTAVE, 6, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_4, "trim to 4 volts", "->  4.000V ", default_footer, CALIBRATE_OCTAVE, 7, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_5, "trim to 5 volts", "->  5.000V ", default_footer, CALIBRATE_OCTAVE, 8, nullptr, 0, DAC::MAX_VALUE },
  { VOLT_6, "trim to 6 volts", "->  6.000V ", default_footer, CALIBRATE_OCTAVE, 9, nullptr, 0, DAC::MAX_VALUE },

  { DAC_FINE_A, "DAC A fine", "->  1.000v ", default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_A, nullptr, -128, 127 },
  { DAC_FINE_B, "DAC B fine", "->  1.000v ", default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_B, nullptr, -128, 127 },
  { DAC_FINE_C, "DAC C fine", "->  1.000v ", default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_C, nullptr, -128, 127 },
  { DAC_FINE_D, "DAC D fine", "->  1.000v ", default_footer, CALIBRATE_DAC_FINE, DAC_CHANNEL_D, nullptr, -128, 127 },

  { CV_OFFSET, "trim CV offset", "use trimpot!", default_footer, CALIBRATE_ADC_TRIMMER, 0, nullptr, 0, 4095 },
  { CV_OFFSET_0, "CV1 (sample)", "use encoder!", default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_1, nullptr, 0, 4095 },
  { CV_OFFSET_1, "CV2 (index)", "use encoder!", default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_2, nullptr, 0, 4095 },
  { CV_OFFSET_2, "CV3 (notes/scale)", "use encoder!", default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_3, nullptr, 0, 4095 },
  { CV_OFFSET_3, "CV4 (transpose)", "use encoder!", default_footer, CALIBRATE_ADC_OFFSET, ADC_CHANNEL_4, nullptr, 0, 4095 },

  { CV_SCALE_1V, "CV Scaling 1V", "TODO", default_footer, CALIBRATE_NONE, 0, nullptr, 0, 0 },
  { CV_SCALE_3V, "CV Scaling 3V", "TODO", default_footer, CALIBRATE_NONE, 0, nullptr, 0, 0 },

  { EXIT, "done?", "", "[prev]         [SAVE]", CALIBRATE_NONE, 0, nullptr, 0, 0 }
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

void calibration_menu() {
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
  OC::TickCount tick_count;
    tick_count.Init();

  encoder[RIGHT].setPos(calibration_state.encoder_data);
  while (true) {

    uint32_t ticks = tick_count.Update();
    digital_input_displays[0].Update(ticks, OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>());
    digital_input_displays[1].Update(ticks, OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_2>());
    digital_input_displays[2].Update(ticks, OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>());
    digital_input_displays[3].Update(ticks, OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_4>());

    calibration_update(calibration_state);
    calibration_draw(calibration_state);

    button_right.read();
    if (button_right.event()) {
      if (calibration_state.step < EXIT)
        calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step + 1);
      else
        break;
    }
    button_left.read();
    if (button_left.event()) {
      if (calibration_state.step > CENTER_DISPLAY)
        calibration_state.step = static_cast<CALIBRATION_STEP>(calibration_state.step - 1);
    }

    const CalibrationStep *next_step = &calibration_steps[calibration_state.step];
    if (next_step != calibration_state.current_step) {
      // Special cases on exit current step
      switch (calibration_state.current_step->step) {
        case HELLO:
          if (calibration_state.encoder_data) {
            SERIAL_PRINTLN("Resetting to defaults...");
            calibration_reset();
          }
          break;
        default: break;
      }

      // Setup next step
      switch (next_step->calibration_type) {
      case CALIBRATE_NONE: break;
      case CALIBRATE_OCTAVE:
        encoder[RIGHT].setPos(OC::calibration_data.dac.octaves[next_step->type_index]);
        break;
      case CALIBRATE_DAC_FINE:
        encoder[RIGHT].setPos(OC::calibration_data.dac.fine[next_step->type_index]);
        break;
      case CALIBRATE_ADC_TRIMMER:
        break;
      case CALIBRATE_ADC_OFFSET:
        encoder[RIGHT].setPos(OC::calibration_data.adc.offset[next_step->type_index]);
        break;
      case CALIBRATE_DISPLAY:
        encoder[RIGHT].setPos(OC::calibration_data.display_offset);
        break;
      }
      calibration_state.current_step = next_step;
    }
  }

  SERIAL_PRINTLN("Calibration complete");
  calibration_save();
}

void calibration_draw(const CalibrationState &state) {
  GRAPHICS_BEGIN_FRAME(true);
  const CalibrationStep *step = state.current_step;

  graphics.setPrintPos(2, kUiTitleTextY + 2);
  graphics.print(step->title);
  graphics.drawHLine(0, kUiDefaultFontH, kUiDisplayWidth);

  graphics.setPrintPos(2, 28);
  switch (step->calibration_type) {
    case CALIBRATE_OCTAVE:
      graphics.print(step->message);
      graphics.print(state.encoder_data);
      break;

    case CALIBRATE_DAC_FINE:
      graphics.print(step->message);
      graphics.pretty_print(state.encoder_data, 4);
      break;

    case CALIBRATE_ADC_TRIMMER:
      graphics.print(_ADC_OFFSET);
      graphics.print(" == ");
      graphics.print((long)state.CV);
      graphics.setPrintPos(2, 28 + kUiLineH);
      graphics.print(step->message);
      break;

    case CALIBRATE_ADC_OFFSET:
      graphics.pretty_print(state.encoder_data - state.CV, 5);
      graphics.print(" --> 0");
      graphics.setPrintPos(2, 28 + kUiLineH);
      graphics.print(step->message);
      break;

    case CALIBRATE_DISPLAY:
      graphics.print(step->message);
      graphics.pretty_print(state.encoder_data, 4);
      graphics.drawFrame(0, 0, 128, 64);
      break;

    case CALIBRATE_NONE:
    default:
      graphics.print(step->message);
      if (step->value_str)
        graphics.print(step->value_str[state.encoder_data]);
      break;
  }

  weegfx::coord_t x = 64 - 10;
  for (int input = OC::DIGITAL_INPUT_1; input < OC::DIGITAL_INPUT_LAST; ++input) {
    uint8_t state = (digital_input_displays[input].getState() + 3) >> 2;
    if (state)
      graphics.drawBitmap8(x, 31 + kUiLineH * 2, 4, OC::bitmap_gate_indicators_8 + (state << 2));
    x += 5;
  }

  graphics.drawStr(0, 31 + kUiLineH * 2, step->footer);
  GRAPHICS_END_FRAME();
}

/* DAC output etc */ 

void calibration_update(CalibrationState &state) {

  state.encoder_data = encoder[RIGHT].pos();
  if (state.encoder_data < state.current_step->min)
    state.encoder_data = state.current_step->min;
  else if (state.encoder_data > state.current_step->max)
    state.encoder_data = state.current_step->max;
  encoder[RIGHT].setPos(state.encoder_data);

  const CalibrationStep *step = state.current_step;

  switch (step->calibration_type) {
    case CALIBRATE_NONE:
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_OCTAVE:
      OC::calibration_data.dac.octaves[step->type_index] = state.encoder_data;
      DAC::set_all(OC::calibration_data.dac.octaves[step->type_index]);
      break;
    case CALIBRATE_DAC_FINE:
      OC::calibration_data.dac.fine[step->type_index] = state.encoder_data;
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO + 1]);
      break;
    case CALIBRATE_ADC_TRIMMER:
      state.CV = _average();
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_ADC_OFFSET:
      state.CV = (state.CV * (kCalibrationAdcSmoothing - 1) + OC::ADC::raw_value(static_cast<ADC_CHANNEL>(step->type_index))) / kCalibrationAdcSmoothing;
      OC::calibration_data.adc.offset[step->type_index] = state.encoder_data;
      DAC::set_all(OC::calibration_data.dac.octaves[_ZERO]);
      break;
    case CALIBRATE_DISPLAY:
      OC::calibration_data.display_offset = state.encoder_data;
      SH1106_128x64_Driver::AdjustOffset(OC::calibration_data.display_offset);
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
