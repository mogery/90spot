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

    uint8_t* dec = malloc(len);
    aes128CtrCrypt(&req->enc, dec, buf, len);

    if (req->fbuf == NULL)
    {
        len -= 0xA7;
        buf += 0xA7;
    }

    if (req->fbuf == NULL)
        req->fbuf = malloc(len);
    else
        req->fbuf = realloc(req->fbuf, req->flen + len);
    memcpy(req->fbuf + req->flen, dec + 0xA7, len);
    req->flen += len;

    if (req->vorbis == NULL)
    {
        int datablock_memory_consumed, error;

        // stb_vorbis* try = stb_vorbis_open_pushdata(
        //     req->fbuf,
        //     req->flen,
        //     &datablock_memory_consumed,
        //     &error,
        //     NULL
        // );

        // if (try == NULL)
        // {
        //     if (error == VORBIS_need_more_data)
        //     {
        //         log_info("[AUDIOFETCH] Needs more data...\n");
        //     }
        //     else
        //     {
        //         log_error("[AUDIOFETCH] VORBIS error %i\n", error);
        //         return -1;
        //     }
        // }
        // else
        //     req->vorbis = try;
    }
    
    if (req->vorbis != NULL)
    {
        int bytes_used = 0, samples_output = 0;
        while (true)
        {
            int channels = 0;
            float** output = NULL;

            // bytes_used = stb_vorbis_decode_frame_pushdata(
            //     req->vorbis,
            //     req->fbuf, req->flen,
            //     &channels,
            //     &output,
            //     &samples_output
            // );

            log_info("[AUDIOFETCH] Bytes: %i Samples: %i Channels: %i\n", bytes_used, samples_output, channels);

            if (bytes_used == 0)
                break;
            
            req->flen -= bytes_used;
            memcpy(req->fbuf, req->fbuf + bytes_used, req->flen);
            req->fbuf = realloc(req->fbuf, req->flen);

            if (req->frame_handler != NULL && req->frame_handler(req->ctx, req, &output, channels, req->handler_state) < 0)
                return -1;
            
            // TODO: output memory leak
        }
    }

    //free(dec);

    return 0;
}

int audiofetch_fetch_end_handler(fetch_ctx* fetch, fetch_pending_request* freq, void* _req)
{
    audiofetch_request* req = _req;

    log_info("[AUDIOFETCH] Fetch ended!\n");
    
    return 0;
}

int audiofetch_audiokey_response_handler(audiokey_ctx* audiokey, uint8_t* key, size_t len, void* _req)
{
    audiofetch_request* req = _req;

    req->key = key;
    aes128CtrContextCreate(&req->enc, key, audio_iv);

    char track_str[SPOTIFY_ID_B62_LENGTH + 1];
    spotify_id_to_b62(track_str, req->track);
    log_info("[AUDIOFETCH] Received key for track %s! Doing initial fetch...", track_str);

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
    req->vorbis = NULL;

    req->fbuf = NULL;
    req->flen = 0;

    req->frame_handler = frame_handler;
    req->handler_state = state;

    char track_str[SPOTIFY_ID_B62_LENGTH + 1];
    spotify_id_to_b62(track_str, track);
    log_info("[AUDIOFETCH] Initializing request for track %s...", track_str);

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