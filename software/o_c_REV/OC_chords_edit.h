#ifndef OC_CHORDS_EDIT_H_
#define OC_CHORDS_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_chords.h"
#include "OC_chords_presets.h"

namespace OC {

// Chords editor
// based on scale editor written by Patrick Dowling, adapted for TU, re-adapted for OC

template <typename Owner>
class ChordEditor {
public:

  void Init() {
    owner_ = nullptr;
    chord_ = &dummy_chord;
    cursor_pos_ = 0;
    edit_this_chord_ = 0;
    chord_property_ = 0;
    //
    chord_quality_ = 0;
    chord_voicing_ = 0;
    chord_inversion_ = 0;
    chord_interchange_ = 0;
    max_chords_ = OC::Chords::CHORDS_USER_LAST - 1;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int chord, int num_chords) {

    if (chord > OC::Chords::CHORDS_USER_LAST - 1)
      return;

    chord_ = &OC::user_chords[chord];
    max_chords_ = num_chords;
    //Serial.print("Editing user chord "); Serial.println(chord);
    owner_ = owner;

    BeginEditing();
  }

  void Close();
  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);

private:

  Owner *owner_;
  const OC::Chord *chord_;
  int8_t edit_this_chord_;
  size_t cursor_pos_;
  int8_t chord_property_;
  int8_t chord_quality_;
  int8_t chord_voicing_;
  int8_t chord_inversion_;
  int8_t chord_interchange_;
  int8_t max_chords_;

  void BeginEditing();
  void move_cursor(int offset);
  void copy_chord(); 
  void paste_chord(); 
  
  void change_property(size_t chord_property, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void ChordEditor<Owner>::Draw() {

  //todo
  const weegfx::coord_t w = 95;
  const weegfx::coord_t x_offset = 8;
  const weegfx::coord_t h = 36;

  weegfx::coord_t x = 64 - (w + 1)/ 2;
  weegfx::coord_t y = 16 - 3;

  graphics.clearRect(x, y, w + 4, h);
  graphics.drawFrame(x, y, w + 5, h);

  x += x_offset;
  y += 4;

  graphics.setPrintPos(x, y);
  graphics.print("#");
  graphics.print(edit_this_chord_ + 1);

  switch(chord_property_) {

    case 0: // quality
    graphics.print(" -- quality");
    break;
    case 1: // voicing
    graphics.print(" -- voicing");
    break;
    case 2: // inversion
    graphics.print(" - inversion");
    break;
    case 3: // interchange
    graphics.print(" interchange");
    break;
    default:
    break;
  }

  x = (64 - (w + 1)/ 2) + x_offset;
  y += 13;
  graphics.setPrintPos(x, y);
  graphics.setPrintPos(x, y);
  graphics.print("> ");

  switch(chord_property_) {

    case 0: // quality
    graphics.print(OC::quality_names[chord_quality_]);
    break;
    case 1: // voicing
    graphics.print(OC::voicing_names[chord_voicing_]);
    break;
    default:
    graphics.print(" - ");
    break;
  }
  
}

template <typename Owner>
void ChordEditor<Owner>::HandleButtonEvent(const UI::Event &event) {

   if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        handleButtonUp(event);
        break;
      case OC::CONTROL_BUTTON_DOWN:
        handleButtonDown(event);
        break;
      case OC::CONTROL_BUTTON_L:
        handleButtonLeft(event);
        break;    
      case OC::CONTROL_BUTTON_R:
        Close();
        break;
      default:
        break;
    }
  }
  else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        // screensaver    // TODO: ideally, needs to be overridden ... invert_mask();
      break;
      case OC::CONTROL_BUTTON_DOWN:
      break;
      case OC::CONTROL_BUTTON_L: 
      break;
      case OC::CONTROL_BUTTON_R:
       // app menu
      break;  
      default:
      break;
     }
  }
}

template <typename Owner>
void ChordEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
 
  if (OC::CONTROL_ENCODER_L == event.control) {
    //move_cursor(event.value);

      // next chord / edit 'offline':
    edit_this_chord_ += event.value;
    CONSTRAIN(edit_this_chord_, 0, max_chords_); 

    const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
    chord_quality_ = chord_def.quality;
    chord_voicing_ = chord_def.voicing;
    chord_inversion_ = chord_def.inversion;
    chord_interchange_ = chord_def.parallel_scale;

  } else if (OC::CONTROL_ENCODER_R == event.control) {

    switch(chord_property_) {

      case 0: // quality:
      {
        chord_quality_ += event.value;
        CONSTRAIN(chord_quality_, 0, OC::Chords::CHORDS_QUALITY_LAST - 1);
        OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
        edit_user_chord_->quality = chord_quality_;
      }
      break;
      case 1: // voicing:
      {
        chord_voicing_ += event.value;
        CONSTRAIN(chord_voicing_, 0, OC::Chords::CHORDS_VOICING_LAST - 1);
        OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
        edit_user_chord_->voicing = chord_voicing_;
      }
      break;
      case 2: // inversion:
      {
        chord_inversion_ += event.value;
        CONSTRAIN(chord_inversion_, 0, OC::Chords::CHORDS_INVERSION_LAST - 1);
        OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
        edit_user_chord_->inversion = chord_inversion_;
      }
      break;
      case 3: // parallel scale/interchange
      {
        chord_interchange_ += event.value;
        CONSTRAIN(chord_interchange_, 0, OC::Scales::NUM_SCALES - 1);
        OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
        edit_user_chord_->parallel_scale = chord_interchange_;
      }
      break;
      default:
      break;
    }
  }
}

template <typename Owner>
void ChordEditor<Owner>::move_cursor(int offset) {

  int cursor_pos = cursor_pos_ + offset;
  CONSTRAIN(cursor_pos, 0, OC::Chords::NUM_CHORDS_PROPERTIES); 
  cursor_pos_ = cursor_pos;
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonUp(const UI::Event &event) {

  chord_property_++;
  CONSTRAIN(chord_property_, 0, sizeof(Chord) - 1);
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonDown(const UI::Event &event) {

    chord_property_--;
    CONSTRAIN(chord_property_, 0, sizeof(Chord) - 1);
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonLeft(const UI::Event &) {
 // todo
}

template <typename Owner>
void ChordEditor<Owner>::copy_chord() {
 // todo
}

template <typename Owner>
void ChordEditor<Owner>::paste_chord() {
 // todo
}

template <typename Owner>
void ChordEditor<Owner>::BeginEditing() {

  cursor_pos_ = 0;
  edit_this_chord_= owner_->get_chord_slot();
  const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
  chord_quality_ = chord_def.quality;
  chord_voicing_ = chord_def.voicing;
  chord_inversion_ = chord_def.inversion;
  chord_interchange_ = chord_def.parallel_scale;
}

template <typename Owner>
void ChordEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace OC

#endif // OC_CHORD_EDIT_H_