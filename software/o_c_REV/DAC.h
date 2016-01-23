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
  static const size_t kHistoryDepth = 8;

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


    size_t tail = history_tail_;
    history_[DAC_CHANNEL_A][tail] = values_[DAC_CHANNEL_A];
    history_[DAC_CHANNEL_B][tail] = values_[DAC_CHANNEL_B];
    history_[DAC_CHANNEL_C][tail] = values_[DAC_CHANNEL_C];
    history_[DAC_CHANNEL_D][tail] = values_[DAC_CHANNEL_D];
    history_tail_ = (tail + 1) % kHistoryDepth;
  }

  template <DAC_CHANNEL channel>
  static void getHistory(uint16_t *dst){
    size_t head = (history_tail_ + 1) % kHistoryDepth;

    size_t count = kHistoryDepth - head;
    const uint16_t *src = history_[channel] + head;
    while (count--)
      *dst++ = *src++;

    count = head;
    src = history_[channel];
    while (count--)
      *dst++ = *src++;
  }

private:
  static uint32_t values_[DAC_CHANNEL_LAST];
  static uint16_t history_[DAC_CHANNEL_LAST][kHistoryDepth];
  static volatile size_t history_tail_;
};

#endif // DAC_H_
