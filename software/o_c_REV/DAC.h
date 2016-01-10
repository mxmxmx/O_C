#ifndef DAC_H_
#define DAC_H_

enum DAC_CHANNEL {
  DAC_CHANNEL_A, DAC_CHANNEL_B, DAC_CHANNEL_C, DAC_CHANNEL_D, DAC_CHANNEL_LAST
};

extern void set8565_CHA(uint32_t data);
extern void set8565_CHB(uint32_t data);
extern void set8565_CHC(uint32_t data);
extern void set8565_CHD(uint32_t data);

class DAC {
public:

  static void Init();

  static void set_all(uint32_t value) {
    for (int i = DAC_CHANNEL_A; i < DAC_CHANNEL_LAST; ++i)
      values_[i] = value;
  }

  template <DAC_CHANNEL channel>
  static void set(uint32_t value) {
    values_[channel] = value;
  }

  template <DAC_CHANNEL channel>
  static uint32_t get() {
    return values_[channel];
  }

  static uint32_t value(size_t index) {
    return values_[index];
  }

  static void WriteAll() {
    set8565_CHA(values_[DAC_CHANNEL_A]);
    set8565_CHB(values_[DAC_CHANNEL_B]);
    set8565_CHC(values_[DAC_CHANNEL_C]);
    set8565_CHD(values_[DAC_CHANNEL_D]);
  }

private:
  static uint32_t values_[DAC_CHANNEL_LAST];
};

#endif // DAC_H_
