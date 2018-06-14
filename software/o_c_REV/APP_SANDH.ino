// TUTORIAL STEP 1: Add these #includes for things we need to use
#include "OC_DAC.h"
#include "OC_ADC.h"
#include "OC_digital_inputs.h"

enum SANDH_SETTINGS {
	SANDH_SETTING_LAST
};

class SANDH : public settings::SettingsBase<SANDH, SANDH_SETTING_LAST> {
public:
	void Init() {
	}

    void ISR() {
    		// TUTORIAL STEP 3 (continued): This is the actual ISR() method, which reads the first digital input
    		// to see if there's a new clock. If so, the TriggerAndSetSample() method is executed:
    		bool update = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
    		if (update) {
    			this->TriggerAndSetSample();
    		}
    }

    void TriggerAndSetSample() {
    		// TUTORIAL STEP 4: When the ISR() method detects a new clock signal, this method uses OC::ADC
    	    // to check the raw pitch value of of Input 1. It sets Output A to the same value, where it stays
    	    // until there's a new trigger.
    	    int32_t current_value;
    	    current_value = OC::ADC::raw_pitch_value(ADC_CHANNEL_1);
    	    OC::DAC::set_pitch(DAC_CHANNEL_A, current_value, 0);
    }
};

SETTINGS_DECLARE(SANDH, SANDH_SETTING_LAST) {
};

// TUTORIAL STEP 2: Make sure there's an instance of your main app class (in this case, SANDH)
SANDH sandh_instance;

// App stubs
void SANDH_init() {
}

size_t SANDH_storageSize() {
	return 0;
}

size_t SANDH_save(void *storage) {
	return 0;
}

size_t SANDH_restore(const void *storage) {
	return 0;
}

void SANDH_isr() {
	// TUTORIAL STEP 3: Complete the ISR() function, specified in OC_apps.ini. In this case, th ISR
	// scans for a trigger at the first digital input. See the SANDH::ISR() method for the rest of the
	// story...
	return sandh_instance.ISR();
}

void SANDH_handleAppEvent(OC::AppEvent event) {

}

void SANDH_loop() {
}

void SANDH_menu() {
	menu::DefaultTitleBar::Draw();
	graphics.setPrintPos(1,1);
    graphics.print("Tutorial 2: S&H") ;
}

void SANDH_screensaver() {

}

void SANDH_handleButtonEvent(const UI::Event &event) {

}

void SANDH_handleEncoderEvent(const UI::Event &event) {

}
