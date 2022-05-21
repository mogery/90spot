#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <switch/audio/driver.h>
#include <switch/audio/audio.h>
#include "spotify/idtool.h"
#include "spotify/audiokey.h"
#include "spotify/audiofetch.h"
#include <vorbis/codec.h>

#define AUDIOPLAY_WAVEBUF_COUNT 2 // default wavebuf count
#define AUDIOPLAY_WAVEBUF_DEF_RATE 48000 // default wavebuf sample rate
#define AUDIOPLAY_WAVEBUF_DEF_LEN 5 // default wavebuf length in seconds
#define AUDIOPLAY_WAVEBUF_DEF_CHANNELS 2 // default wavebuf channels
#define AUDIOPLAY_LOW_SAMPLE_THRESHOLD AUDIOPLAY_WAVEBUF_DEF_RATE * AUDIOPLAY_WAVEBUF_DEF_LEN * AUDIOPLAY_WAVEBUF_DEF_CHANNELS

struct audioplay_ctx {
    audiofetch_ctx* audiofetch;

    AudioDriver audrv;

    int mempool_id;
    uint16_t* mempool_ptr;
    size_t mempool_size;

    AudioDriverWaveBuf wavebufs[AUDIOPLAY_WAVEBUF_COUNT];

    struct audioplay_track* tracks;
};

// sample pile
struct audioplay_sampile {
    uint16_t* buf;
    int samples; // NOT per channel, overall.

    struct audioplay_sampile* next;
};

struct audioplay_track {
    struct audioplay_ctx* ctx;
    audiofetch_request* req;

    spotify_id track_id;
    spotify_file_id file_id;

    bool has_info;
    vorbis_info info;

    bool is_busy;
    bool is_playing;
    bool is_ending;

    struct audioplay_sampile* sampiles;

    struct audioplay_track* next;
};

typedef struct audioplay_sampile audioplay_sampile;
typedef struct audioplay_track audioplay_track;
typedef struct audioplay_ctx audioplay_ctx;

bool audioplay_svc_init();
audioplay_ctx* audioplay_init(audiofetch_ctx* audiofetch);
int audioplay_track_create(audioplay_ctx* ctx, spotify_id track_id, spotify_file_id file_id);
int audioplay_update(audioplay_ctx* ctx);
void audioplay_destroy(audioplay_ctx* ctx);
void audioplay_svc_cleanup();