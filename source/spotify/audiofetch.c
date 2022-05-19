#include "audiofetch.h"
#include "log.h"

#include <string.h>

AUDIOFETCH_AUDIO_IV

audiofetch_ctx* audiofetch_init(fetch_ctx* fetch, audiokey_ctx* audiokey)
{
    audiofetch_ctx* res = malloc(sizeof(audiofetch_ctx));
    res->fetch = fetch;
    res->audiokey = audiokey;
    res->session = fetch->session;
    res->requests = NULL;

    return res;
}

int audiofetch_fetch_data_handler(fetch_ctx* fetch, fetch_pending_request* freq, uint8_t* buf, size_t len, void* _req)
{
    audiofetch_request* req = _req;

    uint8_t* ogg_buf = ogg_sync_buffer(&req->ogg_sync, len);
    if (ogg_buf == NULL)
    {
        log_error("[AUDIOFETCH] failed to allocate ogg_buf :(\n");
        return -1;
    }

    aes128CtrCrypt(&req->enc, ogg_buf, buf, len);
    if (ogg_sync_wrote(&req->ogg_sync, len) != 0)
    {
        log_error("[AUDIOFETCH] libogg sync failed to handle byte write :(\n");
        return -1;
    }

    int page_cnt = 0;
    ogg_page page;
    while (ogg_sync_pageout(&req->ogg_sync, &page) == 1)
    {
        page_cnt++;

        if (ogg_stream_pagein(&req->ogg_stream, &page) != 0)
        {
            log_error("[AUDIOFETCH] libogg stream failed to handle page :(\n");
            return -1;
        }

        ogg_packet packet;
        int packet_cnt = 0;
        while (ogg_stream_packetout(&req->ogg_stream, &packet) == 1)
        {
            if (!req->vorbis_started)
            {
                if (vorbis_synthesis_idheader(&packet) == 0)
                    continue;
                else
                    req->vorbis_started = true;
            }

            packet_cnt++;

            if (req->vorb_packet_cnt < 3)
            {
                if (vorbis_synthesis_headerin(&req->vorb_info, &req->vorb_comment, &packet) != 0)
                {
                    log_error("[AUDIOFETCH] failed to parse Vorbis packet :(\n");
                    return -1;
                }

                req->vorb_packet_cnt++;
                if (req->vorb_packet_cnt == 3)
                {
                    log_info("[AUDIOFETCH] Headers received! Initializing Vorbis DSP...\n");

                    if (req->header_handler && req->header_handler(req->ctx, req, req->vorb_info, req->handler_state) < 0)
                        return -1;

                    if (vorbis_synthesis_init(&req->vorb_dsp, &req->vorb_info) != 0)
                    {
                        log_error("[AUDIOFETCH] failed to initialize Vorbis DSP :(");
                        return -1;
                    }

                    if (vorbis_block_init(&req->vorb_dsp, &req->vorb_blok) != 0)
                    {
                        log_error("[AUDIOFETCH] failed to initialize vorbis block :(");
                        return -1;
                    }
                }
            }
            else
            {
                if (vorbis_synthesis(&req->vorb_blok, &packet) != 0)
                {
                    log_error("[AUDIOFETCH] failed to decode audio packet :(");
                    return -1;
                }

                int binres = vorbis_synthesis_blockin(&req->vorb_dsp, &req->vorb_blok);

                if (binres != 0)
                {
                    log_error("[AUDIOFETCH] vorbis rejected its block :( %d", binres);
                    return -1;
                }

                float** samples;
                int sample_cnt = vorbis_synthesis_pcmout(&req->vorb_dsp, &samples);

                int samples_used = req->frame_handler(req->ctx, req, samples, sample_cnt, req->vorb_info.channels, req->handler_state);
                if (samples_used < 0)
                    return -1;

                if (vorbis_synthesis_read(&req->vorb_dsp, samples_used) != 0)
                {
                    log_error("[AUDIOFETCH] failed to report no of read samples to vorbis :(\n");
                    return -1;
                }
            }
        }
        //log_info("[AUDIOFETCH] Page had %d packets.\n", packet_cnt);
    }

    //log_info("[AUDIOFETCH] Processed %d pages!\n", page_cnt);

    return 0;
}

int audiofetch_fetch_end_handler(fetch_ctx* fetch, fetch_pending_request* freq, void* _req)
{
    audiofetch_request* req = _req;

    if (req->end_handler && req->end_handler(req->ctx, req, req->handler_state) < 0)
        return -1;
    
    return 0;
}

int audiofetch_audiokey_response_handler(audiokey_ctx* audiokey, uint8_t* key, size_t len, void* _req)
{
    audiofetch_request* req = _req;

    req->key = key;
    aes128CtrContextCreate(&req->enc, key, audio_iv);

    char track_str[SPOTIFY_ID_B62_LENGTH + 1];
    spotify_id_to_b62(track_str, req->track);
    log_info("[AUDIOFETCH] Received key for track %s! Doing initial fetch...\n", track_str);

    req->fetch = fetch_stream(
        req->ctx->fetch,
        req->file,
        0, 1024 * 16 / FETCH_BLOCK_SIZE,
        audiofetch_fetch_data_handler,
        audiofetch_fetch_end_handler,
        req
    );

    return 0;
}

audiofetch_request* audiofetch_create(
    audiofetch_ctx* ctx,
    spotify_id track,
    spotify_file_id file,
    audiofetch_end_handler end_handler,
    audiofetch_header_handler header_handler,
    audiofetch_frame_handler frame_handler,
    void* state
)
{
    audiofetch_request* req = malloc(sizeof(audiofetch_request));
    req->ctx = ctx;

    req->track = track;
    req->file = file;

    req->key = NULL;

    req->fetch = NULL;

    ogg_sync_init(&req->ogg_sync);
    ogg_stream_init(&req->ogg_stream, 0);
    vorbis_comment_init(&req->vorb_comment);
    vorbis_info_init(&req->vorb_info);
    req->vorb_packet_cnt = 0;
    req->vorbis_started = false;

    req->end_handler = end_handler;
    req->header_handler = header_handler;
    req->frame_handler = frame_handler;
    req->handler_state = state;

    char track_str[SPOTIFY_ID_B62_LENGTH + 1];
    spotify_id_to_b62(track_str, track);
    log_info("[AUDIOFETCH] Initializing request for track %s...\n", track_str);

    audiokey_request(ctx->audiokey, track, file, audiofetch_audiokey_response_handler, req);

    req->next = ctx->requests;
    ctx->requests = req;
    return req;
}

void audiofetch_destroy(audiofetch_ctx* ctx)
{
    audiofetch_request* rptr = ctx->requests;
    while (rptr != NULL)
    {
        audiofetch_request* rnext = rptr->next;
        free(rptr);
        rptr = rnext;
    }

    free(ctx);
}