#ifndef OC_DIGITAL_INPUTS_H_
#define OC_DIGITAL_INPUTS_H_

namespace OC {

class DigitalInputs {
public:


  template <unsigned pin>
  static inline bool clocked() {
    bool clk = CLK_STATE[pin];
    if (clk)
      CLK_STATE[pin] = false;
    return clk;
  }

  template <unsigned pin>
  static inline bool read_immediate() {
    return !digitalReadFast(pin);
  }
};

};

#endif // OC_DIGITAL_INPUTS_H_
