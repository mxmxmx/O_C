enum HELLO_SETTINGS {
	HELLO_SETTING_LAST
};

class HELLO : public settings::SettingsBase<HELLO, HELLO_SETTING_LAST> {
public:
	void Init() {

	}

    void ISR() {

    }
};

SETTINGS_DECLARE(HELLO, HELLO_SETTING_LAST) {
};

HELLO hello_instance;

// App stubs
void HELLO_init() {
}

size_t HELLO_storageSize() {
	return 0;
}

size_t HELLO_save(void *storage) {
	return 0;
}

size_t HELLO_restore(const void *storage) {
	return 0;
}

void HELLO_isr() {
	return hello_instance.ISR();
}

void HELLO_handleAppEvent(OC::AppEvent event) {

}

void HELLO_loop() {
}

void HELLO_menu() {
	menu::DefaultTitleBar::Draw();
	graphics.setPrintPos(1,1);
    graphics.print("Hello, World!") ;
}

void HELLO_screensaver() {
}

void HELLO_handleButtonEvent(const UI::Event &event) {

}

void HELLO_handleEncoderEvent(const UI::Event &event) {

}
