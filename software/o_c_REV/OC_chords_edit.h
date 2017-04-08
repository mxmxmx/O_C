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

  enum EDIT_PAGE {
  	CHORD_SELECT,
  	CHORD_EDIT,
  	CHORD_EDIT_PAGE_LAST
  };

  void Init() {
    owner_ = nullptr;
    chord_ = &dummy_chord;
    cursor_pos_ = 0;
    cursor_quality_pos_ = 0;
    edit_this_chord_ = 0;
    edit_page_ = CHORD_SELECT;
    //
    chord_quality_ = 0;
    chord_voicing_ = 0;
    chord_inversion_ = 0;
    chord_base_note_ = 0;
    chord_octave_ = 0;
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
  size_t cursor_quality_pos_;
  int8_t chord_quality_;
  int8_t chord_voicing_;
  int8_t chord_inversion_;
  int8_t chord_base_note_;
  int8_t chord_octave_;
  int8_t max_chords_;
  bool edit_page_;

  void BeginEditing();
  void move_cursor(int offset, int page);
  void update_chord(int8_t chord_num);
  void copy_chord(); 
  void paste_chord(); 
  
  void change_property(size_t chord_property, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void ChordEditor<Owner>::Draw() {
    
    weegfx::coord_t w = 128;
    weegfx::coord_t x = 0;
    weegfx::coord_t y = 0;
    weegfx::coord_t h = 64;

    graphics.clearRect(x, y, w, h);
    graphics.drawFrame(x, y, w, h);

    // chord select:
    size_t max_chords = max_chords_ + 1;
    int8_t indicator = owner_->active_chord();
    x = 6;
    y = 6;

    for (size_t i = 0; i < max_chords; ++i, x += 14) {
      
      if (i == indicator) {
        graphics.drawFrame(x, y, 10, 10);
        graphics.drawRect(x + 2, y + 2, 6, 6);
      }
      else if (edit_page_ == CHORD_SELECT)
        graphics.drawRect(x, y, 10, 10);
      else 
        graphics.drawRect(x + 2, y + 2, 6, 6);  
        
      // cursor:
      if (i == cursor_pos_) 
        graphics.drawFrame(x - 2, y - 2, 14, 14);
    }
    
    graphics.drawBitmap8(x, y + 2, 4, bitmap_end_marker4x8);
    if (cursor_pos_ == max_chords)
      graphics.drawFrame(x - 2, y - 2, 8, 14);
    
    // chord properties:
    x = 6;
    y = 23;
    
    for (size_t i = 0; i < sizeof(Chord); ++i, x += 24) {
      
      // draw value
    
      switch(i) {
        
          case 0: // quality
          graphics.setPrintPos(x + 1, y + 7);
          graphics.print(OC::quality_very_short_names[chord_quality_]);
          break;
          case 1: // voicing
          graphics.setPrintPos(x + 1, y + 7);
          graphics.print(voicing_names_short[chord_voicing_]);
          break;
          case 2: // inversion
          graphics.setPrintPos(x + 7, y + 7);
          graphics.print(inversion_names[chord_inversion_]);
          break;
          case 3: // base note
          {
          if (chord_base_note_ > 9)
            graphics.setPrintPos(x + 1, y + 7);
          else
            graphics.setPrintPos(x + 4, y + 7);
          graphics.print(base_note_names[chord_base_note_]);
          }
          break;
          case 4: // octave 
          {
          if (chord_octave_ < 0)
             graphics.setPrintPos(x + 4, y + 7);
          else
            graphics.setPrintPos(x + 7, y + 7);
          graphics.print((int)chord_octave_);
          }
          break;
          default:
          break;
      }
      // draw property name
      graphics.setPrintPos(x + 7, y + 26);
      graphics.print(OC::Strings::chord_property_names[i]);
      
      // cursor:  
      if (i == cursor_quality_pos_) {
        graphics.invertRect(x, y, 21, 21); 
        if (edit_page_ == CHORD_EDIT)
          graphics.invertRect(x, y + 22, 21, 14);
        else 
          graphics.drawFrame(x, y + 22, 21, 14); 
      }
      else {
        graphics.drawFrame(x, y, 21, 21);
        graphics.drawFrame(x, y + 22, 21, 14);  
      }
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
    move_cursor(event.value, edit_page_);
  } 
  else if (OC::CONTROL_ENCODER_R == event.control) {

  	if (cursor_pos_ < max_chords_ + 1) {

      OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
      
	    switch(cursor_quality_pos_) {

	      case 0: // quality:
	      {
	        chord_quality_ += event.value;
	        CONSTRAIN(chord_quality_, 0, OC::Chords::CHORDS_QUALITY_LAST - 1);
	        edit_user_chord_->quality = chord_quality_;
	      }
	      break;
	      case 1: // voicing:
	      {
	        chord_voicing_ += event.value;
	        CONSTRAIN(chord_voicing_, 0, OC::Chords::CHORDS_VOICING_LAST - 1);
	        edit_user_chord_->voicing = chord_voicing_;
	      }
	      break;
	      case 2: // inversion:
	      {
	        chord_inversion_ += event.value;
	        CONSTRAIN(chord_inversion_, 0, OC::Chords::CHORDS_INVERSION_LAST - 1);
	        edit_user_chord_->inversion = chord_inversion_;
	      }
	      break;
	      case 3: // base note
	      {
	        chord_base_note_ += event.value;
          const OC::Scale &scale_def = OC::Scales::GetScale(owner_->get_scale(DUMMY));
	        CONSTRAIN(chord_base_note_, 0, scale_def.num_notes);
	        edit_user_chord_->base_note = chord_base_note_;
	      }
	      break;
        case 4: // octave
        {
          chord_octave_ += event.value;
          CONSTRAIN(chord_octave_, -4, 4);
          edit_user_chord_->octave = chord_octave_;
        }
        break;
	      default:
	      break;
	    }
	}
	else {
        // expand/contract
        int max_chords = max_chords_;
        max_chords += event.value;
        CONSTRAIN(max_chords, 0, 7);

        max_chords_ = max_chords;
        cursor_pos_ = max_chords_ + 1;
        owner_->set_num_chords(max_chords);
	  }
  }
}

template <typename Owner>
void ChordEditor<Owner>::update_chord(int8_t chord_num) {
   // update chord properties:
   const OC::Chord &chord_def = OC::Chords::GetChord(chord_num);
   chord_quality_ = chord_def.quality;
   chord_voicing_ = chord_def.voicing;
   chord_inversion_ = chord_def.inversion;
   chord_base_note_ = chord_def.base_note;
   chord_octave_ = chord_def.octave;
}

template <typename Owner>
void ChordEditor<Owner>::move_cursor(int offset, int page) {

  if (page == CHORD_SELECT) {
    int cursor_pos = cursor_pos_ + offset;
    CONSTRAIN(cursor_pos, 0, max_chords_ + 1);  
    cursor_pos_ = cursor_pos;
    edit_this_chord_ = cursor_pos; 
    update_chord(cursor_pos);
  }
  else {
    int cursor_quality_pos = cursor_quality_pos_ + offset;
    CONSTRAIN(cursor_quality_pos, 0, sizeof(Chord) - 1); 
    cursor_quality_pos_ = cursor_quality_pos;
  }
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonUp(const UI::Event &event) {
   // to do : go to next sequence
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonDown(const UI::Event &event) {
   // to do : go to previous sequence
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonLeft(const UI::Event &) {

  if (edit_page_ == CHORD_SELECT) {

    if (cursor_pos_ < max_chords_ + 1) {
    	edit_page_ = CHORD_EDIT;
    	// edit chord:
      edit_this_chord_ = cursor_pos_;
      update_chord(cursor_pos_);
      //cursor_quality_pos_ = 0;
    }
  }
  else {
  	edit_page_ = CHORD_SELECT;
    cursor_pos_ = edit_this_chord_;
  }
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

  cursor_pos_ = edit_this_chord_= owner_->get_chord_slot();
  const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
  chord_quality_ = chord_def.quality;
  chord_voicing_ = chord_def.voicing;
  chord_inversion_ = chord_def.inversion;
  chord_base_note_ = chord_def.base_note;
  chord_octave_ = chord_def.octave;
  edit_page_ = CHORD_SELECT;
}

template <typename Owner>
void ChordEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace OC

#endif // OC_CHORD_EDIT_H_
