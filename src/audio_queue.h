#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AUDIO_QUEUE_CAPACITY 2048u
#define AUDIO_SILENCE 128u

typedef struct {
    uint8_t samples[AUDIO_QUEUE_CAPACITY];
    size_t read_pos;
    size_t count;
} audio_queue_t;

/* The caller serializes access against the audio ISR, including reset. */
void audio_queue_reset(audio_queue_t *queue);
bool audio_queue_push(audio_queue_t *queue, const uint8_t *samples, size_t count);
uint8_t audio_queue_pop(audio_queue_t *queue);

#endif
