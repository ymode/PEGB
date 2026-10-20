#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "audio_queue.h"
#include "timing.h"

int main(void) {
    uint32_t phase = 0;
    uint64_t us = 0;
    for (uint64_t sample = 1; sample <= 32768 * 10; ++sample) {
        unsigned delta = audio_clock_next_us(&phase);
        assert(delta == 30 || delta == 31);
        us += delta;
        assert(us == sample * 1000000 / 32768);
    }
    assert(us == 10000000 && phase == 0);
    frame_clock_t clock;
    frame_clock_reset(&clock, 100);
    phase = 0;
    uint64_t samples = 0;
    for (uint64_t frame = 1; frame <= 100000; ++frame) {
        uint64_t deadline = frame_clock_next(&clock, clock.deadline_us + 1000);
        assert(deadline == 100 + frame * 70224 * 1000000 / 4194304);
        unsigned count = audio_frame_samples(&phase);
        assert(count == 548 || count == 549);
        samples += count;
        assert(samples == frame * 70224 * 32768 / 4194304);
    }
    uint64_t late = clock.deadline_us + 1000000;
    assert(frame_clock_next(&clock, late) == late);
    assert(frame_clock_next(&clock, late + 1000) == late + 16742);
    frame_clock_reset(&clock, 9000000000ULL); // menu resume
    assert(frame_clock_next(&clock, 9000001000ULL) == 9000016742ULL);
    puts("PASS: sample/frame clocks, fractional samples, slow frames and menu resume");

    audio_queue_t queue = {0};
    uint8_t reference[AUDIO_QUEUE_CAPACITY], batch[600];
    size_t count = 0;
    unsigned rng = 7;
    for (unsigned step = 0; step < 10000; ++step) {
        rng = rng * 1664525u + 1013904223u;
        if (step % 97 == 0) {
            audio_queue_reset(&queue);
            count = 0;
        } else if (rng & 1) {
            size_t n = (rng >> 8) % sizeof(batch);
            for (size_t i = 0; i < n; ++i) batch[i] = (uint8_t)(step + i);
            bool expected = n <= AUDIO_QUEUE_CAPACITY - count;
            assert(audio_queue_push(&queue, batch, n) == expected);
            if (expected) { memcpy(reference + count, batch, n); count += n; }
        } else {
            size_t n = (rng >> 8) % 700;
            for (size_t i = 0; i < n; ++i) {
                uint8_t expected = count ? reference[0] : AUDIO_SILENCE;
                assert(audio_queue_pop(&queue) == expected);
                if (count) { --count; memmove(reference, reference + 1, count); }
            }
        }
        assert(queue.count == count);
    }
    audio_queue_reset(&queue);
    assert(audio_queue_push(&queue, NULL, 0));
    assert(!audio_queue_push(&queue, batch, SIZE_MAX));
    assert(audio_queue_pop(&queue) == AUDIO_SILENCE);
    puts("PASS: FIFO order, wraparound, full/empty queue, reset and rejected writes");
}
