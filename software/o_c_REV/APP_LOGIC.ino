bool LOGIC_AND(bool s1, bool s2) {return s1 & s2;}
bool LOGIC_OR(bool s1, bool s2) {return s1 | s2;}
bool LOGIC_XOR(bool s1, bool s2) {return s1 != s2;}
bool LOGIC_NAND(bool s1, bool s2) {return !LOGIC_AND(s1, s2);}
bool LOGIC_NOR(bool s1, bool s2) {return !LOGIC_OR(s1, s2);}
bool LOGIC_XNOR(bool s1, bool s2) {return !LOGIC_XOR(s1, s2);}
typedef bool(*LogicGateFunction)(bool, bool);


// TUTORIAL 1: Set the settings. This should be familar from the Saving Settings tutorial */
enum LOGIC_SETTINGS {
    LOGIC_SETTING_OPERATION_A,
    LOGIC_SETTING_SOURCE_A,
    LOGIC_SETTING_OPERATION_B,
    LOGIC_SETTING_SOURCE_B,
    LOGIC_SETTING_OPERATION_C,
    LOGIC_SETTING_SOURCE_C,
    LOGIC_SETTING_OPERATION_D,
    LOGIC_SETTING_SOURCE_D,
	LOGIC_SETTING_LAST
};

class LOGIC : public settings::SettingsBase<LOGIC, LOGIC_SETTING_LAST> {
public:
	void Init() {
	    // Set callback functions in an array
	    LogicGateFunction logic_gate_list[] = {LOGIC_AND, LOGIC_OR, LOGIC_XOR, LOGIC_NAND, LOGIC_NOR, LOGIC_XNOR};
        for(int i = 0; i < 6; i++)
        {
            logic_gate[i] = logic_gate_list[i];
        }
	}

    void ISR() {
        // Current state of all digital inputs
        bool in1 = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_1>();
        bool in2 = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();
        bool in3 = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_3>();
        bool in4 = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_4>();

        for (int ch = 0; ch < 4; ch++)
        {
            // Set the potential sources
            bool s1 = in1;
            bool s2 = in2;
            int op = values_[ch * 2];
            int sc = values_[ch * 2 + 1];
            if (sc == 1) {
                s1 = in2;
                s2 = in3;
            }
            if (sc == 2) {
                s1 = in3;
                s2 = in4;
            }

            // Get the result based on the selected logic gate
            bool result = logic_gate[op](s1, s2);

            // Output the result to the DAC on this channel
            DAC_CHANNEL channel = (DAC_CHANNEL)(ch);
            OC::DAC::set_pitch(channel, 0, (result ? 5 :0));
        }
    }

    // TUTORIAL 4: Add a public cursor property to your app class
    menu::ScreenCursor<menu::kScreenLines> cursor;

private:
    LogicGateFunction logic_gate[6];
};

// TUTORIAL 2: Setting up some value lists
const char* const gates[6] = {
  "AND", "OR", "XOR", "NAND", "NOR", "XNOR"
};

const char* const input_pairs[3] = {
  "1,2", "2, 3", "3,4"
};

// TUTORIAL 3: This should also be familiar
SETTINGS_DECLARE(LOGIC, LOGIC_SETTING_LAST) {
    { 0, 0, 5, "Logic Gate A", gates, settings::STORAGE_TYPE_U8 },
    { 0, 0, 2, "Source for A", input_pairs, settings::STORAGE_TYPE_U8 },
    { 0, 0, 5, "Logic Gate B", gates, settings::STORAGE_TYPE_U8 },
    { 0, 0, 2, "Source for B", input_pairs, settings::STORAGE_TYPE_U8 },
    { 0, 0, 5, "Logic Gate C", gates, settings::STORAGE_TYPE_U8 },
    { 0, 0, 2, "Source for C", input_pairs, settings::STORAGE_TYPE_U8 },
    { 0, 0, 5, "Logic Gate D", gates, settings::STORAGE_TYPE_U8 },
    { 0, 0, 2, "Source for D", input_pairs, settings::STORAGE_TYPE_U8 }
};

LOGIC logic_instance;

// App stubs
void LOGIC_init() {
    // TUTORIAL 5: Initialize the cursor like this, with the name of the first parmeter, and the
    // index of the last parameter that you want in the menu.
    logic_instance.cursor.Init(LOGIC_SETTING_OPERATION_A, LOGIC_SETTING_LAST - 1);
    logic_instance.Init();
}

size_t LOGIC_storageSize() {
  return LOGIC::storageSize();
}

size_t LOGIC_save(void *storage) {
  return logic_instance.Save(storage);
}

size_t LOGIC_restore(const void *storage) {
  return logic_instance.Restore(storage);
}

void LOGIC_isr() {
    logic_instance.ISR();
}

void LOGIC_handleAppEvent(OC::AppEvent event) {

}

void LOGIC_loop() {
}

// TUTORIAL 6: The loop below is streamlined menu logic, and it handles the menu display
// pretty much automagically.
void LOGIC_menu() {
	menu::DefaultTitleBar::Draw();
    graphics.print("Logic");

    menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX - 1> settings_list(logic_instance.cursor);
    menu::SettingsListItem list_item;
    while (settings_list.available())
    {
        const int current = settings_list.Next(list_item);
        const int value = logic_instance.get_value(current);
        list_item.DrawDefault(value, LOGIC::value_attr(current));
    }
}

void LOGIC_screensaver() {
    // Whatever
}

// TUTORIAL 7: Handle conventional right button behavior: switch between changing the editing/selecting
// modes of the cursor.
void LOGIC_handleButtonEvent(const UI::Event &event) {
    if (event.control == OC::CONTROL_BUTTON_R) {
        if (event.type == UI::EVENT_BUTTON_PRESS) {
            logic_instance.cursor.toggle_editing();
        }
    }
}

// TUTORIAL 7, part II: Handle conventional right encoder behavior: select the parameter or edit it,
// depending on the edit mode selected by the button
void LOGIC_handleEncoderEvent(const UI::Event &event) {
    if (event.control == OC::CONTROL_ENCODER_R) {
        if (logic_instance.cursor.editing()) {
            logic_instance.change_value(logic_instance.cursor.cursor_pos(), event.value);
        } else {
            logic_instance.cursor.Scroll(event.value);
        }
    }
}
