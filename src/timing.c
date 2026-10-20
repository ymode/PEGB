#include "timing.h"

#define DMG_CYCLES_PER_FRAME 70224u
#define DMG_CYCLES_PER_SECOND 4194304u

void frame_clock_reset(frame_clock_t *clock, uint64_t now_us) {
    clock->deadline_us = now_us;
    clock->remainder = 0;
}

uint64_t frame_clock_next(frame_clock_t *clock, uint64_t now_us) {
    uint64_t ticks = (uint64_t)DMG_CYCLES_PER_FRAME * 1000000u + clock->remainder;
    clock->deadline_us += ticks / DMG_CYCLES_PER_SECOND;
    clock->remainder = ticks % DMG_CYCLES_PER_SECOND;
    // A late frame rebases the schedule instead of accumulating catch-up work.
    if (clock->deadline_us < now_us)
        frame_clock_reset(clock, now_us);
    return clock->deadline_us;
}

uint32_t audio_clock_next_us(uint32_t *remainder) {
    uint32_t ticks = 1000000u + *remainder;
    *remainder = ticks % PEGB_SAMPLE_RATE;
    return ticks / PEGB_SAMPLE_RATE;
}

unsigned audio_frame_samples(uint32_t *remainder) {
    uint64_t ticks = (uint64_t)DMG_CYCLES_PER_FRAME * PEGB_SAMPLE_RATE + *remainder;
    *remainder = ticks % DMG_CYCLES_PER_SECOND;
    return (unsigned)(ticks / DMG_CYCLES_PER_SECOND);
}
