#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#define PEGB_SAMPLE_RATE 32768u
#define PEGB_FRAME_SAMPLES_MAX 549u

typedef struct {
    uint64_t deadline_us;
    uint32_t remainder;
} frame_clock_t;

void frame_clock_reset(frame_clock_t *clock, uint64_t now_us);
uint64_t frame_clock_next(frame_clock_t *clock, uint64_t now_us);
uint32_t audio_clock_next_us(uint32_t *remainder);
unsigned audio_frame_samples(uint32_t *remainder);

#endif
