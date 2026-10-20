#include "audio_queue.h"

#include <string.h>

void audio_queue_reset(audio_queue_t *queue) {
    queue->read_pos = 0;
    queue->count = 0;
}

bool audio_queue_push(audio_queue_t *queue, const uint8_t *samples, size_t count) {
    if (count > AUDIO_QUEUE_CAPACITY - queue->count)
        return false;
    if (!count)
        return true;
    size_t write_pos = (queue->read_pos + queue->count) % AUDIO_QUEUE_CAPACITY;
    size_t first = AUDIO_QUEUE_CAPACITY - write_pos;
    if (first > count)
        first = count;
    memcpy(queue->samples + write_pos, samples, first);
    memcpy(queue->samples, samples + first, count - first);
    queue->count += count;
    return true;
}

uint8_t audio_queue_pop(audio_queue_t *queue) {
    if (!queue->count)
        return AUDIO_SILENCE;
    uint8_t value = queue->samples[queue->read_pos];
    queue->read_pos = (queue->read_pos + 1) % AUDIO_QUEUE_CAPACITY;
    --queue->count;
    return value;
}
