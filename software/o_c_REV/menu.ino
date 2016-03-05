/* 
*
* hello etc ... display stuff 
*
*/

/* ----------- screensaver ----------------- */
void screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

  weegfx::coord_t x = 8;
  uint8_t y, width = 8;
  for(int i = 0; i < 4; i++, x += 32 ) { 
    y = DAC::value(i) >> 10; 
    y++; 
    graphics.drawRect(x, 64-y, width, width); // replace second 'width' with y for bars.
  }

  GRAPHICS_END_FRAME();
}

static const size_t kScopeDepth = 64;

uint16_t scope_history[DAC::kHistoryDepth];
uint16_t averaged_scope_history[DAC_CHANNEL_LAST][kScopeDepth];
size_t averaged_scope_tail = 0;
DAC_CHANNEL scope_update_channel = DAC_CHANNEL_A;

template <size_t size>
inline uint16_t calc_average(const uint16_t *data) {
  uint32_t sum = 0;
  size_t n = size;
  while (n--)
    sum += *data++;
  return sum / size;
}

void scope() {
  GRAPHICS_BEGIN_FRAME(false);
  scope_render();
  GRAPHICS_END_FRAME();
}

void scope_render() {
  switch (scope_update_channel) {
    case DAC_CHANNEL_A:
      DAC::getHistory<DAC_CHANNEL_A>(scope_history);
      averaged_scope_history[DAC_CHANNEL_A][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_B;
      break;
    case DAC_CHANNEL_B:
      DAC::getHistory<DAC_CHANNEL_B>(scope_history);
      averaged_scope_history[DAC_CHANNEL_B][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_C;
      break;
    case DAC_CHANNEL_C:
      DAC::getHistory<DAC_CHANNEL_C>(scope_history);
      averaged_scope_history[DAC_CHANNEL_C][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_D;
      break;
    case DAC_CHANNEL_D:
      DAC::getHistory<DAC_CHANNEL_D>(scope_history);
      averaged_scope_history[DAC_CHANNEL_D][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_A;
      averaged_scope_tail = (averaged_scope_tail + 1) % kScopeDepth;
      break;
    default: break;
  }

  for (weegfx::coord_t x = 0; x < (weegfx::coord_t)kScopeDepth - 1; ++x) {
    size_t index = (x + averaged_scope_tail + 1) % kScopeDepth;
    graphics.setPixel(x, 0 + averaged_scope_history[DAC_CHANNEL_A][index]);
    graphics.setPixel(64 + x, 0 + averaged_scope_history[DAC_CHANNEL_B][index]);
    graphics.setPixel(x, 32 + averaged_scope_history[DAC_CHANNEL_C][index]);
    graphics.setPixel(64 + x, 32 + averaged_scope_history[DAC_CHANNEL_D][index]);
  }
}
