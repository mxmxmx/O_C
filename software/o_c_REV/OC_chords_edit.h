// Copyright (c)  2015, 2016, 2017 Patrick Dowling, Max Stadler, Tim Churches
//
// Author of original O+C firmware: Max Stadler (mxmlnstdlr@gmail.com)
// Author of app scaffolding: Patrick Dowling (pld@gurkenkiste.com)
// Modified for bouncing balls: Tim Churches (tim.churches@gmail.com)
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

#ifndef OC_CHORDS_EDIT_H_
#define OC_CHORDS_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_chords.h"
#include "OC_chords_presets.h"

namespace OC {

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
    cursor_pos_ = 0;
    cursor_quality_pos_ = 0;
    edit_this_chord_ = 0;
    edit_this_progression_ = 0;
    edit_page_ = CHORD_SELECT;
    //
    chord_quality_ = 0;
    chord_voicing_ = 0;
    chord_inversion_ = 0;
    chord_base_note_ = 0;
    chord_octave_ = 0;
    max_chords_ = OC::Chords::NUM_CHORDS - 1;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int chord, int num_chords, int num_progression) {

    if (chord > OC::Chords::CHORDS_USER_LAST - 1)
      return;
    
    edit_this_progression_ = num_progression;
    chord_ = &OC::user_chords[chord + edit_this_progression_ * OC::Chords::NUM_CHORDS];
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
  int8_t edit_this_progression_;
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
    uint8_t indicator = owner_->active_chord();
    bool active = owner_->get_active_progression() == edit_this_progression_;
    
    x = 6;
    y = 6;
    // which progression ? ...  
    graphics.setPrintPos(x, y + 2);
    graphics.print(edit_this_progression_ + 0x1);
    // now draw steps:
    x += 11;
    
    for (size_t i = 0; i < max_chords; ++i, x += 13) {
      
      if (active && i == indicator) {
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

    // end marker:
    graphics.drawFrame(x, y, 2, 2);
    graphics.drawFrame(x, y + 4, 2, 2);
    graphics.drawFrame(x, y + 8, 2, 2);
  
    if (cursor_pos_ == max_chords)
        graphics.drawFrame(x - 2, y - 2, 6, 14);

    // draw extra cv num chords?
    if (owner_->get_num_chords_cv()) {
      int extra_slots = owner_->get_display_num_chords() - (max_chords - 0x1);
      if (active && extra_slots > 0x0) {
        x += 5;
        indicator -= max_chords;
        for (size_t i = 0; i < (uint8_t)extra_slots; ++i, x += 10) {
          if (i == indicator)
            graphics.drawRect(x, y + 2, 6, 6);
          else
            graphics.drawFrame(x, y + 2, 6, 6);
        }
      }
    }
    
    // chord properties:
    x = 6;
    y = 23;

    for (size_t i = 0; i < sizeof(Chord); ++i, x += 24) {
      
      // draw values
    
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
            // indicate if note is out-of-range:
            if (chord_base_note_ > (uint8_t)OC::Scales::GetScale(owner_->get_scale(DUMMY)).num_notes) {
              graphics.drawBitmap8(x + 3, y + 25, 4, OC::bitmap_indicator_4x8);
              graphics.drawBitmap8(x + 14, y + 25, 4, OC::bitmap_indicator_4x8);
            }
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

  	if (cursor_pos_ < (uint8_t)(max_chords_ + 1)) {
      
      // write to the right slot, at the right index/offset (a nicer struct would be nicer, but well)
      OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_ + edit_this_progression_ * OC::Chords::NUM_CHORDS]; 
      
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
	        CONSTRAIN(chord_base_note_, 0, (uint8_t)scale_def.num_notes);
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
        CONSTRAIN(max_chords, 0, OC::Chords::NUM_CHORDS - 0x1);

        max_chords_ = max_chords;
        cursor_pos_ = max_chords_ + 1;
        owner_->set_num_chords(max_chords, edit_this_progression_);
	  }
  }
}

template <typename Owner>
void ChordEditor<Owner>::update_chord(int8_t chord_num) {
   // update chord properties:
   const OC::Chord &chord_def = OC::Chords::GetChord(chord_num, edit_this_progression_); 
   chord_quality_ = chord_def.quality;
   chord_voicing_ = chord_def.voicing;
   chord_inversion_ = chord_def.inversion;
   chord_base_note_ = chord_def.base_note;
   chord_octave_ = chord_def.octave;
   max_chords_ = owner_->get_num_chords(edit_this_progression_);
}

template <typename Owner>
void ChordEditor<Owner>::move_cursor(int offset, int page) {

  if (page == CHORD_SELECT) {
    int cursor_pos = cursor_pos_ + offset;
    CONSTRAIN(cursor_pos, 0, max_chords_ + 1);  
    cursor_pos_ = cursor_pos;
    edit_this_chord_ = cursor_pos; 
    // update .. 
    if (cursor_pos <= max_chords_)
      update_chord(cursor_pos);
  }
  else {
    int cursor_quality_pos = cursor_quality_pos_ + offset;
    CONSTRAIN(cursor_quality_pos, 0, (int8_t)(sizeof(Chord) - 1)); 
    cursor_quality_pos_ = cursor_quality_pos;
  }
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonUp(const UI::Event &event) {
   // go to next chords progression
   edit_this_progression_++;
   if (edit_this_progression_ >= OC::Chords::NUM_CHORD_PROGRESSIONS)
    edit_this_progression_ = 0x0;
   update_chord(cursor_pos_);
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonDown(const UI::Event &event) {
   // go to previous chords progression
   edit_this_progression_--;
   if (edit_this_progression_ < 0x0)
    edit_this_progression_ = OC::Chords::NUM_CHORD_PROGRESSIONS - 1;
   update_chord(cursor_pos_);
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonLeft(const UI::Event &) {

  if (edit_page_ == CHORD_SELECT) {

    if (cursor_pos_ < (uint8_t)(max_chords_ + 1)) {
    	edit_page_ = CHORD_EDIT;
    	// edit chord:
      edit_this_chord_ = cursor_pos_;
      update_chord(cursor_pos_);
    }
    // select previous chord if clicking on end-marker:
    else if (cursor_pos_ == (uint8_t)(max_chords_ + 1)) {
      edit_page_ = CHORD_EDIT;
      cursor_pos_--;
      edit_this_chord_ = cursor_pos_;
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
  const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_, edit_this_progression_);
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
