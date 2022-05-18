#include <stdlib.h>
#include <stdbool.h>
#include <switch/crypto/aes_ctr.h>
#include "session.h"
#include "fetch.h"
#include "audiokey.h"
#include "idtool.h"
#include "ogg/ogg.h"
#include "vorbis/codec.h"

#ifndef _audiofetch_H
#define _audiofetch_H

#define AUDIOFETCH_AUDIO_IV static uint8_t audio_iv[0x10] = {0x72, 0xe0, 0x67, 0xfb, 0xdd, 0xcb, 0xcf, 0x77, 0xeb, 0xe8, 0xbc, 0x64, 0x3f, 0x63, 0x0d, 0x93};

struct audiofetch_ctx {
    fetch_ctx* fetch;
    audiokey_ctx* audiokey;
    session_ctx* session;

    struct audiofetch_request* requests;
};

// Return <0 for an error.
typedef int (*audiofetch_frame_handler)(
    struct audiofetch_ctx* ctx, // The Fetch context the response originates from
    struct audiofetch_request* req, // The Fetch request the response originates from
    float** samples, // Samples of each channel
    int count, // Number of samples (per channel)
    int channels, // Number of channels
    void* state // Optional state arg
);

typedef int (*audiofetch_end_handler)(
    struct audiofetch_ctx* ctx, // The Fetch context the response originates from
    struct audiofetch_request* req, // The Fetch request the response originates from
    void* state // Optional state arg
);

struct audiofetch_request {
    struct audiofetch_ctx* ctx;

    spotify_id track;
    spotify_file_id file;

    uint8_t* key;
    Aes128CtrContext enc;

    fetch_pending_request* fetch;

    ogg_sync_state ogg_sync;
    ogg_stream_state ogg_stream;
    vorbis_comment vorb_comment;
    vorbis_info vorb_info;
    vorbis_dsp_state vorb_dsp;
    vorbis_block vorb_blok;
    bool vorbis_started;
    int vorb_packet_cnt;

    audiofetch_frame_handler frame_handler;
    void* handler_state;

    struct audiofetch_request* next;
};

typedef struct audiofetch_ctx audiofetch_ctx;
typedef struct audiofetch_request audiofetch_request;

audiofetch_ctx* audiofetch_init(fetch_ctx* fetch, audiokey_ctx* audiokey);
audiofetch_request* audiofetch_create(audiofetch_ctx* ctx, spotify_id track, spotify_file_id file, audiofetch_frame_handler frame_handler, void* state);
void audiofetch_destroy(audiofetch_ctx* ctx);

#endif