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
  void copy_chord(); 
  void paste_chord(); 
  
  void change_property(size_t chord_property, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void ChordEditor<Owner>::Draw() {

  switch(edit_page_) {	

  	case CHORD_SELECT: 
  	{
  		size_t num_chords = max_chords_;
  		size_t max_chords = num_chords + 1;
  		static constexpr weegfx::coord_t kMinWidth = 8 * 13;

  		weegfx::coord_t w = num_chords * 13;

  		if (w < kMinWidth) w = kMinWidth;

  		weegfx::coord_t x = 64 - (w + 1)/ 2;
  		weegfx::coord_t y = 16 - 2;
  		weegfx::coord_t h = 36;

  		graphics.clearRect(x, y, w + 4, h);
  		graphics.drawFrame(x, y, w + 5, h);

  		x += 2;
  		y += 3;

  		graphics.setPrintPos(x, y);
  		if (cursor_pos_ != max_chords) {
  			graphics.print("#");
  			graphics.print((int)cursor_pos_ + 1); 
  		}
  		else {
  			graphics.print("<");
  			graphics.print((int)cursor_pos_);
  			graphics.print(">");
  		}

  		graphics.setPrintPos(x, y + 24);

  		x += 2; y += 10;
      int8_t indicator = owner_->active_chord();
      
  		for (size_t i = 0; i < max_chords; ++i, x += 12) {
  		  
  		  if (i == indicator)
  		    graphics.drawFrame(x, y, 8, 8);
        else 
          graphics.drawRect(x, y, 8, 8);  
        // cursor:  
        if (i == cursor_pos_)
          graphics.drawFrame(x - 2, y - 2, 12, 12);
        
  		}
  		graphics.drawBitmap8(x, y, 4, bitmap_end_marker4x8);
   	    if (cursor_pos_ == max_chords)
      		graphics.drawFrame(x - 2, y - 2, 8, 12);
  	}	
  	break;
    
  	case CHORD_EDIT: 
  	{
    
      weegfx::coord_t w = 128;
      weegfx::coord_t x = 0;
      weegfx::coord_t y = 16 - 2;
      weegfx::coord_t h = 45;

      graphics.clearRect(x, y, w, h + 10);
      graphics.drawFrame(x, y, w, h);
     
      // chord:
      graphics.setPrintPos(85, 2);
      graphics.print("chord#");
      graphics.print((int)edit_this_chord_ + 1); 
      
      x += 6;
      y += 5;
      
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
          graphics.invertRect(x, y + 22, 21, 14); 
        }
        else {
          graphics.drawFrame(x, y, 21, 21);
          graphics.drawFrame(x, y + 22, 21, 14);  
        }
      }
      if(edit_this_chord_ == owner_->active_chord())
        graphics.drawLine(0, 62, 128, 62);
	  }
	  break;
    
	default:
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
    move_cursor(event.value, edit_page_);
  } else if (OC::CONTROL_ENCODER_R == event.control) {

  	if (edit_page_ == CHORD_EDIT) {

	    switch(cursor_quality_pos_) {

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
	      case 3: // base note
	      {
	        chord_base_note_ += event.value;
          const OC::Scale &scale_def = OC::Scales::GetScale(owner_->get_scale(DUMMY));
	        CONSTRAIN(chord_base_note_, 0, scale_def.num_notes);
	        OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
	        edit_user_chord_->base_note = chord_base_note_;
	      }
	      break;
        case 4: // octave
        {
          chord_octave_ += event.value;
          CONSTRAIN(chord_octave_, -4, 4); //todo
          OC::Chord *edit_user_chord_ = &OC::user_chords[edit_this_chord_];
          edit_user_chord_->octave = chord_octave_;
        }
        break;
	      default:
	      break;
	    }
	}
	else {

		if (cursor_pos_ == max_chords_ + 1) {

        int max_chords = max_chords_;
        max_chords += event.value;
        CONSTRAIN(max_chords, 0, 7);

        max_chords_ = max_chords;
        cursor_pos_ = max_chords_ + 1;
        owner_->set_num_chords(max_chords);
      }
	  }
  }
}

template <typename Owner>
void ChordEditor<Owner>::move_cursor(int offset, int page) {

  if (page == CHORD_SELECT) {
    int cursor_pos = cursor_pos_ + offset;
    CONSTRAIN(cursor_pos, 0, max_chords_ + 1);  
    cursor_pos_ = cursor_pos;
  }
  else {
    int cursor_quality_pos = cursor_quality_pos_ + offset;
    CONSTRAIN(cursor_quality_pos, 0, sizeof(Chord) - 1); 
    cursor_quality_pos_ = cursor_quality_pos;
  }
}

template <typename Owner>
void ChordEditor<Owner>::handleButtonUp(const UI::Event &event) {
  
 if (edit_page_ == CHORD_SELECT) {
  // to do: go to next sequence
 }
 else {
   // go to next chord
   edit_this_chord_++;
   if (edit_this_chord_ > max_chords_)
      edit_this_chord_ = 0;
   const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
   chord_quality_ = chord_def.quality;
   chord_voicing_ = chord_def.voicing;
   chord_inversion_ = chord_def.inversion;
   chord_base_note_ = chord_def.base_note;
   chord_octave_ = chord_def.octave;
 }

}

template <typename Owner>
void ChordEditor<Owner>::handleButtonDown(const UI::Event &event) {

  if (edit_page_ == CHORD_SELECT) {
  // to do: go to prev. sequence
 }
 else {
   // go to previous chord
   edit_this_chord_--;
   if (edit_this_chord_ < 0)
      edit_this_chord_ = max_chords_;
   const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
   chord_quality_ = chord_def.quality;
   chord_voicing_ = chord_def.voicing;
   chord_inversion_ = chord_def.inversion;
   chord_base_note_ = chord_def.base_note;
   chord_octave_ = chord_def.octave;
 }

}

template <typename Owner>
void ChordEditor<Owner>::handleButtonLeft(const UI::Event &) {

  if (edit_page_ == CHORD_SELECT) {
  	edit_page_ = CHORD_EDIT;
  	// edit chord:
    edit_this_chord_ = cursor_pos_;
    const OC::Chord &chord_def = OC::Chords::GetChord(edit_this_chord_);
    chord_quality_ = chord_def.quality;
    chord_voicing_ = chord_def.voicing;
    chord_inversion_ = chord_def.inversion;
    chord_base_note_ = chord_def.base_note;
    chord_octave_ = chord_def.octave;
    cursor_quality_pos_ = 0;
  }
  else
  	edit_page_ = CHORD_SELECT;
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
