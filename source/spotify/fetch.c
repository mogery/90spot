#include "fetch.h"

#include <string.h>
#include "log.h"
#include "conv.h"

fetch_ctx* fetch_init(channelmgr_ctx* channelmgr)
{
    fetch_ctx* res = malloc(sizeof(fetch_ctx));
    res->channelmgr = channelmgr;
    res->session = channelmgr->session;
    res->requests = NULL;

    return res;
}

int fetch_channelmgr_data_handler(struct channelmgr_ctx* ctx, uint8_t* buf, uint16_t len, void* _req)
{
    fetch_pending_request* req = _req;

    if (req->collect_data)
    {
        if (req->collect_buf == NULL)
        {
            req->collect_buf = malloc(len);
        }
        else
        {
            req->collect_buf = realloc(req->collect_buf, req->collect_len + len);
        }

        memcpy(req->collect_buf + req->collect_len, buf, len);
        req->collect_len += len;
    }

    if (req->data_handler)
    {
        if (req->data_handler(req->ctx, req, buf, len, req->handler_state) < 0)
            return -1;
    }

    return 0;
}

int fetch_channelmgr_close_handler(struct channelmgr_ctx* ctx, void* _req)
{
    fetch_pending_request* req = _req;

    if (req->end_handler)
    {
        if (req->end_handler(req->ctx, req, req->handler_state) < 0)
            return -1;
    }

    return 0;
}

channelmgr_channel* fetch_generic(fetch_ctx* ctx, fetch_pending_request* req, spotify_file_id file, uint32_t from, uint32_t to)
{
    channelmgr_channel* auch = channelmgr_channel_allocate(ctx->channelmgr, NULL, fetch_channelmgr_data_handler, fetch_channelmgr_close_handler, req);

    uint8_t* packet = malloc(26 + SPOTIFY_FILE_ID_RAW_LENGTH);
    conv_u162b(packet, auch->seq);
    packet[2] = 0;
    packet[3] = 1;
    conv_u162b(packet + 4 , 0x0000    );
    conv_u322b(packet + 6 , 0x00000000);
    conv_u322b(packet + 10, 0x00009C40);
    conv_u322b(packet + 14, 0x00020000);
    memcpy(packet + 18, file.id, SPOTIFY_FILE_ID_RAW_LENGTH);
    conv_u322b(packet + 18 + SPOTIFY_FILE_ID_RAW_LENGTH, from);
    conv_u322b(packet + 18 + SPOTIFY_FILE_ID_RAW_LENGTH + 4, to);

    session_send_message(ctx->session, 0x8, packet, 26 + SPOTIFY_FILE_ID_RAW_LENGTH);

    return auch;
}

fetch_pending_request* fetch_block(fetch_ctx* ctx, spotify_file_id file, uint32_t from, uint32_t to, fetch_end_handler handler, void* state)
{
    fetch_pending_request* req = malloc(sizeof(fetch_pending_request));
    req->ctx = ctx;
    req->data_handler = NULL;
    req->end_handler = handler;
    req->handler_state = state;
    req->collect_data = true;
    req->collect_buf = NULL;
    req->collect_len = 0;
    req->chan = fetch_generic(ctx, req, file, from, to);

    req->next = ctx->requests;
    ctx->requests = req;
    return req;
}

fetch_pending_request* fetch_stream(fetch_ctx* ctx, spotify_file_id file, uint32_t from, uint32_t to, fetch_data_handler data_handler, fetch_end_handler end_handler, void* state)
{
    fetch_pending_request* req = malloc(sizeof(fetch_pending_request));
    req->ctx = ctx;
    req->data_handler = data_handler;
    req->end_handler = end_handler;
    req->handler_state = state;
    req->collect_data = false;
    req->collect_buf = NULL;
    req->collect_len = 0;
    req->chan = fetch_generic(ctx, req, file, from, to);

    req->next = ctx->requests;
    ctx->requests = req;
    return req;
}

void fetch_destroy(fetch_ctx* ctx)
{
    // clear request list
    fetch_pending_request* rptr = ctx->requests;
    while (rptr != NULL)
    {
        if (rptr->collect_data && rptr->collect_buf != NULL)
            free(rptr->collect_buf);
        
        fetch_pending_request* rnext = rptr->next;
        free(rptr);
        rptr = rnext;
    }

    free(ctx);
}