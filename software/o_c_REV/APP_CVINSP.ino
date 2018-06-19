/* TUTORIAL STEP 1: Make an enum containing representations of your setting names.
 * You can use these enum values to reference the settings later. This app has only
 * one setting, but two enum values. Why? Because enum values are assigned ints starting
 * from 0, so the APPNAME_SETTING_LAST is used to represent the total number of settings
 * (See STEP 2)
 */
enum CVINSP_SETTINGS {
  CVINSP_CHANNEL,
  CVINSP_SETTING_LAST
};

/* TUTORIAL STEP 2: Your app class should be a subclass of settings::SettingsBase. Include
 * the class name, and the APPNAME_SETTING_LAST, as shown below. This lets the base class
 * know the size of the settings array.
 */
class CVInspector : public settings::SettingsBase<CVInspector, CVINSP_SETTING_LAST> {
public:
	void Init() {
	}

    void ISR() {
    		int32_t adc_value;

    		/* TUTORIAL STEP 5: Reading a setting value using values_[ENUM_VALUE] */
    		ADC_CHANNEL channel = (ADC_CHANNEL)values_[CVINSP_CHANNEL];

    		// value()
		adc_value = OC::ADC::value(channel);
		value = (int)adc_value;

		// raw_value()
		adc_value = OC::ADC::raw_value(channel);
		raw_value = (int)adc_value;

		// smoothed_raw_value()
		adc_value = OC::ADC::smoothed_raw_value(channel);
		smoothed_raw_value = (int)adc_value;

		// pitch_value()
		adc_value = OC::ADC::pitch_value(channel);
		pitch_value = (int)adc_value;

		// raw_pitch_value()
		adc_value = OC::ADC::raw_pitch_value(channel);
		raw_pitch_value = (int)adc_value;
    }

    void ChangeChannel(int dir)
    {
    		/* TUTORIAL STEP 6: Writing a setting value. In this case, I'm using change_value(),
    		 * which takes the setting index (or enum value) and the delta, which is the amount
    		 * by which the int is changing.
    		 *
    		 * If you want to just set the value instead of changing it, you can instead use
    		 * apply_value(index, value). Both change_value() and apply_value() do bounds
    		 * checking, using the range supplied in DECLARE_SETTINGS (see STEP 3).
    		 *
    		 * Both of these methods are in the base class SettingsBase.
    		 *
    		 * You can also just use values_[ENUM_VALUE] = int, which will bypass the range
    		 * checking.
    		 *
    		 * Note that changing a value in an array does not save the settings. To save the
    		 * settings, long-hold the right encoder button to go to the menu; then long-hold the
    		 * right encoder button again.
    		 */
    		change_value(CVINSP_CHANNEL, dir);
    }

    void DrawHeader() {
		graphics.setPrintPos(0,0);
        graphics.print("OC::ADC::          ");

        /* TUTORIAL STEP 7: Making use of the value names by accessing the attributes.
         * value_attr() is a method from SettingsBase. The value_names property contains
         * an array of strings, which you simply reference with the value.
         */
        const settings::value_attr &attr = value_attr(CVINSP_CHANNEL);
        graphics.print_right(attr.value_names[values_[CVINSP_CHANNEL]]);
    }

    void DrawBody() {
    		DrawLine(0, "value          ", value);
    		DrawLine(1, "raw_value      ", raw_value);
    		DrawLine(2, "smoothed_raw_va", smoothed_raw_value);
    		DrawLine(3, "pitch_value    ", pitch_value);
    		DrawLine(4, "raw_pitch_value", raw_pitch_value);
    }

    void DrawLine(int line, const char* label, int value) {
    		graphics.setPrintPos(0, 10 + (10 * line));
    		graphics.print(label);
    		graphics.pretty_print(value);
    }

private:
    int value;
    int raw_value;
    int smoothed_raw_value;
    int pitch_value;
    int raw_pitch_value;
};

/* TUTORIAL STEP 3: Each setting needs a list of attributes, provided using the
 * SETTINGS_DECLARE macro. Each setting is a six-element array that's used to
 * initialize a settings::value_attr struct (see util/util_settings.h)
 *
 * The values are:
 *     default value
 *     minimum value
 *     maximum value
 *     setting name
 *     option list (an array of strings, or NULL if there's no list)
 *     storage type (a member of the StorageType enum from util_settings.h)
 */
SETTINGS_DECLARE(CVInspector, CVINSP_SETTING_LAST) {
	{0, 0, 3, "channel", OC::Strings::cv_input_names, settings::STORAGE_TYPE_U4}
};

CVInspector inspector_instance;

// App stubs
void CVINSP_init() {
}

/* TUTORIAL STEP 4: implement APPNAME_storageSize(), APPNAME_save(), and APPNAME_restore().
 * Don't miss that storageSize() operates on the class, while save() and restore() operate
 * on the instance. All of these methods are from the base class, SettingsBase.
 */
size_t CVINSP_storageSize() {
	return CVInspector::storageSize();
}

size_t CVINSP_save(void *storage) {
	return inspector_instance.Save(storage);
}

size_t CVINSP_restore(const void *storage) {
	return inspector_instance.Restore(storage);
}

void CVINSP_isr() {
	return inspector_instance.ISR();
}

void CVINSP_handleAppEvent(OC::AppEvent event) {

}

void CVINSP_loop() {
}

void CVINSP_menu() {
	inspector_instance.DrawHeader();
	inspector_instance.DrawBody();
}

void CVINSP_screensaver() {
	inspector_instance.DrawHeader();
	inspector_instance.DrawBody();
}

void CVINSP_handleButtonEvent(const UI::Event &event) {

}

void CVINSP_handleEncoderEvent(const UI::Event &event) {
	inspector_instance.ChangeChannel(event.value);
}
