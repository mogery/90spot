#include "audiokey.h"

#include <stdlib.h>
#include <string.h>
#include "session.h"
#include "log.h"
#include "idtool.h"
#include "conv.h"

int audiokey_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* ctx);

audiokey_ctx* audiokey_init(session_ctx* session)
{
    audiokey_ctx* res = malloc(sizeof(audiokey_ctx));
    memset(res, 0, sizeof(audiokey_ctx));

    res->session = session;
    res->next_seq = 0;
    res->requests = NULL;

    session_add_message_listener(session, audiokey_message_listener, res);

    return res; 
}

int audiokey_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* _ctx)
{
    audiokey_ctx* ctx = _ctx;

    if (cmd != 0xd && cmd != 0xe)
    {
        return 0;
    }

    uint32_t seq = conv_b2u32(buf);
    audiokey_pending_request* req = ctx->requests;
    audiokey_pending_request* rprev = NULL;
    while (req != NULL)
    {
        // if req is found
        if (req->seq == seq)
        {
            // pop req from list
            if (rprev == NULL)
            {
                ctx->requests = req->next;
            }
            else
            {
                rprev->next = req->next;
            }

            break;
        }

        rprev = req;
        req = req->next;
    }

    if (req == NULL)
    {
        log_warn("[AUDIOKEY] Called for seq %u, pending request not found?!\n", seq);
        return 0;
    }

    int res = 0;

    if (cmd == 0xd)
    {
        uint8_t* key = malloc(len - 4);
        memcpy(key, buf + 4, len - 4);

        res = req->handler(ctx, key, len - 4, req->handler_state);
    }
    else if (cmd == 0xe)
    {
        res = req->handler(ctx, NULL, 0, req->handler_state);
    }

    free(req);

    return res;
}

uint32_t audiokey_next_seq(audiokey_ctx* ctx, uint8_t* seq)
{
    conv_u322b(seq, ctx->next_seq);

    return (ctx->next_seq++);
}

int audiokey_request(audiokey_ctx* ctx, spotify_id track, spotify_file_id file, audiokey_response_handler handler, void* state)
{
    uint8_t seq_buf[4];
    uint32_t seq = audiokey_next_seq(ctx, seq_buf);

    audiokey_pending_request* req = malloc(sizeof(audiokey_pending_request));
    req->handler = handler;
    req->handler_state = state;
    req->seq = seq;
    req->next = ctx->requests;
    ctx->requests = req;

    uint8_t track_raw[SPOTIFY_ID_RAW_LENGTH];
    spotify_id_to_raw(track_raw, track);

    uint8_t* packet = malloc(SPOTIFY_FILE_ID_RAW_LENGTH + SPOTIFY_ID_RAW_LENGTH + 4 + 2);
    memcpy(packet, file.id, SPOTIFY_FILE_ID_RAW_LENGTH);
    memcpy(packet + SPOTIFY_FILE_ID_RAW_LENGTH, track_raw, SPOTIFY_ID_RAW_LENGTH);
    conv_u322b(packet + SPOTIFY_FILE_ID_RAW_LENGTH + SPOTIFY_ID_RAW_LENGTH, seq);
    conv_u162b(packet + SPOTIFY_FILE_ID_RAW_LENGTH + SPOTIFY_ID_RAW_LENGTH + 4, 0x0000);

    if (session_send_message(ctx->session, 0xc, packet, SPOTIFY_FILE_ID_RAW_LENGTH + SPOTIFY_ID_RAW_LENGTH + 4 + 2) < 0)
    {
        log_error("[AUDIOKEY] Failed to send request!\n");
        return -1;
    }

    return 0;
}

void audiokey_destroy(audiokey_ctx* ctx)
{
    session_remove_message_listener(ctx->session, audiokey_message_listener);

    // Free all pending messages
    audiokey_pending_request* rptr = ctx->requests;
    while (rptr != NULL)
    {
        audiokey_pending_request* rnext = rptr->next;
        free(rptr);
        rptr = rnext;
    }

    free(ctx);
}