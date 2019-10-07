// Copyright (c) 2019, Jason Justian
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

// This singleton class has a few jobs related to keeping track of the bias value
// selected by the user and/or the app:
//
// (1) It allows advancing bias through three settings, one at a time
// (2) It allows setting the bias directly with a state
// (3) It shows a popup indicator for one second when the setting is advanced

#ifndef VBIAS_MANAGER_H
#define VBIAS_MANAGER_H

#include "OC_options.h"

#ifdef VOR 

#define BIAS_EDITOR_TIMEOUT 16666

class VBiasManager {
    static VBiasManager *instance;
    int bias_state;
    uint32_t last_advance_tick;

    VBiasManager() {
        bias_state = 0;
        last_advance_tick = 0;
    }

public:
    static const int UNI = 0;
    static const int ASYM = 1;
    static const int BI = 2;

    static VBiasManager *get() {
        if (!instance) instance = new VBiasManager;
        return instance;
    }

    /*
     * Advance to the next state, when the button is pushed
     */
    void AdvanceBias() {
        // Only advance the bias if it's been less than a second since the last button press.
        // This is so that the first button press shows the popup without changing anything.
        if (OC::CORE::ticks - last_advance_tick < BIAS_EDITOR_TIMEOUT) {
            if (++bias_state > 2) bias_state = 0;
            instance->ChangeBiasToState(bias_state);
        }
        last_advance_tick = OC::CORE::ticks;
    }

    int IsEditing() {
        return (OC::CORE::ticks - last_advance_tick < BIAS_EDITOR_TIMEOUT);
    }

    /*
     * Change to a specific state. This should replace a direct call to OC::DAC::set_Vbias(), because it
     * allows VBiasManager to keep track of the current state so that the button advances the state as
     * expected. For example:
     *
     * #ifdef VOR
     *     VBiasManager *vbias_m = vbias_m->get();
     *     vbias_m->ChangeBiasToState(VBiasManager::UNI);
     * #endif
     *
     */
    void ChangeBiasToState(int new_bias_state) {
        int new_bias_value = OC::calibration_data.v_bias & 0xFFFF; // Bipolar = lower 2 bytes
        if (new_bias_state == VBiasManager::UNI) new_bias_value = OC::DAC::VBiasUnipolar;
        if (new_bias_state == VBiasManager::ASYM) new_bias_value = (OC::calibration_data.v_bias >> 16); // asym. = upper 2 bytes
        OC::DAC::set_Vbias(new_bias_value);
        bias_state = new_bias_state;
    }

    void SetStateForApp(int app_index) {
        int new_state = VBiasManager::ASYM;
        switch (app_index)
        {
        case 0: new_state = VBiasManager::ASYM; break;  // CopierMachine (or) ASR
        case 1: new_state = VBiasManager::ASYM; break;  // Harrington 1200 (or) Triads
        case 2: new_state = VBiasManager::ASYM; break;  // Automatonnetz (or) Vectors
        case 3: new_state = VBiasManager::ASYM; break;  // Quantermain (or) 4x Quantizer
        case 4: new_state = VBiasManager::ASYM; break;  // Meta-Q (or) 2x Quantizer
        case 5: new_state = VBiasManager::BI; break;    // Quadraturia (or) Quadrature LFO
        case 6: new_state = VBiasManager::BI; break;  // Low-rents (or) Lorenz
        case 7: new_state = VBiasManager::UNI; break;   // Piqued (or) 4x EG
        case 8: new_state = VBiasManager::ASYM; break;  // Sequins (or) 2x Sequencer
        case 9: new_state = VBiasManager::UNI; break;   // Dialectic Ping Pong (or) Balls
        case 10: new_state = VBiasManager::UNI; break;  // Viznutcracker sweet (or) Bytebeats
        case 11: new_state = VBiasManager::ASYM; break; // Acid Curds (or) Chords
        case 12: new_state = VBiasManager::UNI; break;  // References (or) Voltages
        }
        instance->ChangeBiasToState(new_state);
    }

    /*
     * If the last state advance (with the button) was less than a second ago, draw the popup indicator
     */
    void DrawPopupPerhaps() {
        if (OC::CORE::ticks - last_advance_tick < BIAS_EDITOR_TIMEOUT) {
            graphics.clearRect(17, 7, 82, 43);
            graphics.drawFrame(18, 8, 80, 42);

            graphics.setPrintPos(20, 10);
            graphics.print("VOR:");
            graphics.setPrintPos(30, 20);
            graphics.print(" 0V -> 10V");
            graphics.setPrintPos(30, 30);
            graphics.print("-3V -> 7V");
            graphics.setPrintPos(30, 40);
            graphics.print("-5V -> 5V");

            graphics.setPrintPos(20, 20 + (bias_state * 10));
            graphics.print("> ");
        }
    }
};

#endif // VBIAS_MANAGER_H
#endif
