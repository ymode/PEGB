#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void audio_init(void);
void audio_pause(void);
void audio_reset(void);
void audio_play_frame(void);
uint8_t audio_read(uint16_t addr);
void audio_write(uint16_t addr, uint8_t val);

#endif
