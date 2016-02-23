#ifndef OC_DIGITAL_INPUTS_H_
#define OC_DIGITAL_INPUTS_H_

namespace OC {

enum DigitalInput {
  DIGITAL_INPUT_1,
  DIGITAL_INPUT_2,
  DIGITAL_INPUT_3,
  DIGITAL_INPUT_4,
  DIGITAL_INPUT_LAST
};

static constexpr uint32_t DIGITAL_INPUT_1_MASK = (0x1 << DIGITAL_INPUT_1);
static constexpr uint32_t DIGITAL_INPUT_2_MASK = (0x1 << DIGITAL_INPUT_2);
static constexpr uint32_t DIGITAL_INPUT_3_MASK = (0x1 << DIGITAL_INPUT_3);
static constexpr uint32_t DIGITAL_INPUT_4_MASK = (0x1 << DIGITAL_INPUT_4);

class DigitalInputs {
public:

  static void Init();

  // @return mask if pin clocked since last call and reset state
  template <DigitalInput input> static inline uint32_t clocked() {
    // This leaves a very small window where a ::clock() might interrupt.
    // With LDREX/STREX it should be possible to close the window, but maybe
    // not necessary
    uint32_t clk = clocked_[input];
    clocked_[input] = 0;
    return clk << input;
  }

  static inline uint32_t peek() {
    uint32_t mask = 0;
    if (clocked_[DIGITAL_INPUT_1]) mask |= DIGITAL_INPUT_1_MASK;
    if (clocked_[DIGITAL_INPUT_2]) mask |= DIGITAL_INPUT_2_MASK;
    if (clocked_[DIGITAL_INPUT_3]) mask |= DIGITAL_INPUT_3_MASK;
    if (clocked_[DIGITAL_INPUT_4]) mask |= DIGITAL_INPUT_4_MASK;
    return mask;
  }

  template <DigitalInput input> static inline bool read_immediate() {
    return !digitalReadFast(input);
  }

  template <DigitalInput input> static inline void clock() {
    clocked_[input] = 1;
  }

private:
  static volatile uint32_t clocked_[DIGITAL_INPUT_LAST];
};

};

#endif // OC_DIGITAL_INPUTS_H_
