#define DARKEST_TIMELINE_SETTING_LAST 66
#define DT_SEQ_STEPS 64
#define DT_TIMELINE_SIZE (DT_SEQ_STEPS /2)
#define DT_TIMELINE_CV 0
#define DT_TIMELINE_PROBABILITY 1
#define DT_VIEW_HEIGHT 25
#define DT_DATA_MAX 7800
#define DT_CURSOR_TICKS 8000
#define DT_TRIGGER_TICKS 100
#define DT_LENGTH 64
#define DT_INDEX 65

class DarkestTimeline : public settings::SettingsBase<DarkestTimeline, DARKEST_TIMELINE_SETTING_LAST> {
public:
    void Init() {
    		apply_value(DT_LENGTH, 16);
    		apply_value(DT_INDEX, 0);
    	    cursor = 0; // The sequence's record/play point
    	    clocked  = 0; // If 1, the sequencer needs to handle a clock input
    	    cv_record_enabled = 0; // When clocked, CV will be recorded into the cursor point
    	    prob_record_enabled = 0; // When clocked, probability will be recorded into the cursor point
    	    blink_countdown = DT_CURSOR_TICKS; // For record modes to blink the cursor
    	    trigger1_countdown = 0; // ISR cycles remaining for trigger 1
    	    trigger2_countdown = 0; // ISR cycles remaining for trigger 2
    	    screensaver = 0; // If 1, display a simplified column

    	    for (int i = 0; i < DT_SEQ_STEPS; i++) values_[i] = 0;
    }

    void ISR() {
    		UpdateInputState();
    		UpdateOutputState();
    		if (--blink_countdown < -DT_CURSOR_TICKS) blink_countdown = DT_CURSOR_TICKS;
    		if (--trigger1_countdown < 0) trigger1_countdown = 0;
    		if (--trigger2_countdown < 0) trigger2_countdown = 0;
    }

    void Resume() {
    		cursor = 0;
    		clocked = 0;
    		cv_record_enabled = 0;
    		prob_record_enabled = 0;
    }

    bool ScreenSaverMode(bool s) {
    		screensaver = s;
    		return screensaver;
    }

    void UpdateInputState() {
		int idx = data_index_at_position(0);
		int32_t adc_value;

    		// Update CV value if CV Record is enabled
		if (cv_record_enabled) {
			adc_value = OC::ADC::raw_pitch_value(ADC_CHANNEL_1);
			values_[idx] = (int)adc_value;
		}

		// Update Probability value if Probability Record is enabled
		if (prob_record_enabled) {
			adc_value = OC::ADC::raw_pitch_value(ADC_CHANNEL_2);
			if (adc_value < 0) adc_value = 0; // Constrain probability to positive
			values_[idx + DT_TIMELINE_SIZE] = (int)adc_value;
		}

		// Update Index value
		adc_value = OC::ADC::raw_pitch_value(ADC_CHANNEL_3);
		if (adc_value < 0) adc_value = -adc_value;
		apply_value(DT_INDEX, adc_value / (DT_DATA_MAX / (DT_TIMELINE_SIZE + 1)));

		// Step forward on digital input 1
		if (OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>()) {
			MoveCursor(1);
		}

		// Step backward on digital input 2
		if (OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_2>()) {
			MoveCursor(-1);
		}

		// Reset on digital input 3
		if (OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>()) {
			cursor = 0;
			apply_value(DT_INDEX, cursor);
			clocked = 1;
		}
    }

    void UpdateOutputState() {
    		int idx = data_index_at_position(0);

    		// Update normal CV value
    	    OC::DAC::set_pitch(DAC_CHANNEL_A, values_[idx], 0);

    	    // Update alternate universe timeline CV value
    	    int alternate_universe = (idx + length()) % DT_TIMELINE_SIZE;
    	    OC::DAC::set_pitch(DAC_CHANNEL_B, values_[alternate_universe], 0);

    	    if (clocked) { // Only do this once for each clock trigger
			// Check for probability-based normal trigger
			int prob = DT_DATA_MAX - values_[idx + DT_TIMELINE_SIZE];
			int comp = random(300, DT_DATA_MAX - 300);
			if (comp > prob) trigger1_countdown = DT_TRIGGER_TICKS;

			// Check for probability-based alternate trigger
			prob = values_[idx + DT_TIMELINE_SIZE];
			comp = random(300, DT_DATA_MAX - 300);
			if (comp > prob) trigger2_countdown = DT_TRIGGER_TICKS;

    	    		clocked = 0; // Reset and wait for next clock
    	    }

    	    // Update trigger CV values
		OC::DAC::set_pitch(DAC_CHANNEL_C, 1, (trigger1_countdown > 0) ? 5 : 0);
		OC::DAC::set_pitch(DAC_CHANNEL_D, 1, (trigger2_countdown > 0) ? 5 : 0);
    }

    void ChangeLength(int direction) {
    		change_value(DT_LENGTH, direction);
    }

    int MoveCursor(int direction) {
    		cursor += direction;
    		if (cursor < 0) cursor = length() - 1;
    		if (cursor >= length()) cursor = 0;
    		clocked = 1;
    		return cursor;
    }

    void ToggleRecord(int timeline_type) {
    		if (timeline_type == DT_TIMELINE_CV) {
    			cv_record_enabled = (1 - cv_record_enabled);
    		} else {
    			prob_record_enabled = (1 - prob_record_enabled);
    		}
    }

    void ClearTimeline(int timeline_type) {
    		int offset = (timeline_type == DT_TIMELINE_CV) ? 0 : DT_TIMELINE_SIZE;
    		for (int i = 0; i < DT_TIMELINE_SIZE; i++)
    		{
    			values_[i + offset] = 0;
    		}
    }

    void RandomizeTimeline() {
    		for (int i = 0; i < DT_SEQ_STEPS; i++)
    		{
    			values_[i] = random(0, DT_DATA_MAX);
    		}
    }

    void DrawHeader() {
		graphics.setPrintPos(0, 0);
		graphics.print("Darkest Timeline ");
		graphics.pretty_print(length());
    }

    void DrawView(int timeline_type) {
		int y_offset = (timeline_type == DT_TIMELINE_CV) ? 10 : 36;
		int data_offset = (timeline_type == DT_TIMELINE_CV) ? 0 : DT_TIMELINE_SIZE;
		for (int position = 0; position < length(); position++)
		{
			int idx = data_index_at_position(position);
			int data = values_[idx + data_offset]; // CV data will be proportioned for display
			bool is_recording = (timeline_type == DT_TIMELINE_CV) ? cv_record_enabled : prob_record_enabled;
			bool is_index = (idx == index()) ? 1 : 0;
			DrawColumn(position, y_offset, data, is_recording, is_index);
		}
    }

    void DrawCVView() {
    		DrawView(DT_TIMELINE_CV);
    }

    void DrawProbabilityView() {
		DrawView(DT_TIMELINE_PROBABILITY);
    }

    void DrawColumn(int position, int y_offset, int data, bool is_recording, bool is_index) {
    		int units_per_pixel = DT_DATA_MAX / DT_VIEW_HEIGHT;
    		int height = data / units_per_pixel;
    		int width = (128 / length()) / 2;
    		int x_pos = (128 / length()) * position;
    		int x_offset = width / 2;
    		int y_pos = DT_VIEW_HEIGHT - height + y_offset; // Find the top
    		if (position == 0) { // The cursor position is at the leftmost column
    			graphics.drawRect(x_pos + x_offset, y_pos, width, height);
    			if (is_recording && blink_countdown > 0) {
    				// If in record mode, draw a full-sized frame around the cursor
    				graphics.invertRect(x_pos, y_offset, width * 2, DT_VIEW_HEIGHT);
    			}
    		} else {
    			if (screensaver) {height = 3; width *= 2;}
    			graphics.drawFrame(x_pos + x_offset, y_pos, width, height);
    		}

    		if (index() > 0 && is_index) {
    			for (int y = 10; y < 63; y += 4)
    			{
    				// Check raw y for safety, as I keep playing with the numbers above.
    				if (y + 2 < 63) graphics.drawLine(x_pos, y, x_pos, y + 2);
    			}
    		}
    }

    int length() {
    		return values_[DT_LENGTH];
    }

    int index() {
    		return values_[DT_INDEX];
    }

private:
    int cursor;
    bool clocked;
    bool cv_record_enabled;
    bool prob_record_enabled;
    bool screensaver;

    int blink_countdown;
    int trigger1_countdown;
    int trigger2_countdown;

    /* Geez, don't even ask me to explain this. It's 11PM. (position + cursor) % length constrains the
	 * pattern to length steps. Then the index is added. Then the % DT_TIMELINE_SIZE constrains
	 * the pattern to the entire indexed pattern.
	 *
	 * Note that this is th data index for the CV timeline. For the probability timeline, add
	 * DT_TIMELINE_SIZE to the returned value.
	 */
    int data_index_at_position(int position) {
    		int idx = (((position + cursor) % length()) + index()) % DT_TIMELINE_SIZE;
    		return idx;
    }
};

#define DARKEST_TIMELINE_VALUE_ATTRIBUTE {0, 0, 8192, "step", NULL, settings::STORAGE_TYPE_U16},
#define DO_SIXTEEN_TIMES(A) A A A A A A A A A A A A A A A A
SETTINGS_DECLARE(DarkestTimeline, DARKEST_TIMELINE_SETTING_LAST) {
	DO_SIXTEEN_TIMES(DARKEST_TIMELINE_VALUE_ATTRIBUTE)
	DO_SIXTEEN_TIMES(DARKEST_TIMELINE_VALUE_ATTRIBUTE)
	DO_SIXTEEN_TIMES(DARKEST_TIMELINE_VALUE_ATTRIBUTE)
	DO_SIXTEEN_TIMES(DARKEST_TIMELINE_VALUE_ATTRIBUTE)
	{16, 1, 32, "length", NULL, settings::STORAGE_TYPE_U8},
	{16, 1, 32, "index", NULL, settings::STORAGE_TYPE_U8},
};

DarkestTimeline timeline_instance;

// App stubs
void DARKESTTIMELINE_init() {
  timeline_instance.Init();
}

size_t DARKESTTIMELINE_storageSize() {
	return DarkestTimeline::storageSize();
}

size_t DARKESTTIMELINE_save(void *storage) {
	return timeline_instance.Save(storage);
}

size_t DARKESTTIMELINE_restore(const void *storage) {
	return timeline_instance.Restore(storage);
}

void DARKESTTIMELINE_isr() {
	return timeline_instance.ISR();
}

void DARKESTTIMELINE_handleAppEvent(OC::AppEvent event) {
    if (event ==  OC::APP_EVENT_RESUME) {
    		timeline_instance.Resume();
    }
}

void DARKESTTIMELINE_loop() {
}

void DARKESTTIMELINE_menu() {
	timeline_instance.ScreenSaverMode(0);
	timeline_instance.DrawHeader();
	timeline_instance.DrawCVView();
	timeline_instance.DrawProbabilityView();
}

void DARKESTTIMELINE_screensaver() {
	timeline_instance.ScreenSaverMode(1);
	timeline_instance.DrawCVView();
	timeline_instance.DrawProbabilityView();
}

void DARKESTTIMELINE_handleButtonEvent(const UI::Event &event) {

	if (event.control == OC::CONTROL_BUTTON_UP) {
		if (event.type == UI::EVENT_BUTTON_PRESS) timeline_instance.ToggleRecord(DT_TIMELINE_CV);
	}

	if (event.control == OC::CONTROL_BUTTON_DOWN) {
		if (event.type == UI::EVENT_BUTTON_PRESS) timeline_instance.ToggleRecord(DT_TIMELINE_PROBABILITY);
		if (event.type == UI::EVENT_BUTTON_LONG_PRESS) {
			timeline_instance.ClearTimeline(DT_TIMELINE_CV);
			timeline_instance.ClearTimeline(DT_TIMELINE_PROBABILITY);
		}
	}

	if (event.control == OC::CONTROL_BUTTON_R) {
		timeline_instance.MoveCursor(1);
	}

	if (event.control == OC::CONTROL_BUTTON_L) {
		timeline_instance.RandomizeTimeline();
	}
}

void DARKESTTIMELINE_handleEncoderEvent(const UI::Event &event) {
	if (event.control == OC::CONTROL_ENCODER_L) {
		timeline_instance.ChangeLength(event.value > 0 ? -1 : 1);
	}

	if (event.control == OC::CONTROL_ENCODER_R) {
		timeline_instance.MoveCursor(event.value > 0 ? 1 : -1);
	}
}

void FillValueAttributes(settings::value_attr attributes[]) {
	for (int i = 0; i < DT_SEQ_STEPS; i++)
	{
		settings::value_attr tmp = {0, 0, 65536, "seq", NULL, settings::STORAGE_TYPE_U16};
		attributes[i] = tmp;
	}
}
