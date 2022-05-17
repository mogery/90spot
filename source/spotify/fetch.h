#include <stdlib.h>
#include <stdbool.h>
#include "session.h"
#include "channelmgr.h"
#include "idtool.h"

#ifndef _fetch_H
#define _fetch_H

#define FETCH_BLOCK_SIZE 4 // size of fetch blocks in bytes
#define FETCH_MAX_BLOCKS 5 * 1024 * 1024 / FETCH_BLOCK_SIZE // max fetch file size = 5MiB

struct fetch_ctx {
    channelmgr_ctx* channelmgr;
    session_ctx* session;

    struct fetch_pending_request* requests;
};

// Return <0 for an error.
typedef int (*fetch_data_handler)(
    struct fetch_ctx* ctx, // The Fetch context the response originates from
    struct fetch_pending_request* req, // The Fetch request the response originates from
    uint8_t* buf,
    size_t len,
    void* state // Optional state arg
);

typedef int (*fetch_end_handler)(
    struct fetch_ctx* ctx, // The Fetch context the response originates from
    struct fetch_pending_request* req, // The Fetch request the response originates from
    void* state // Optional state arg
);

struct fetch_pending_request {
    struct fetch_ctx* ctx;
    channelmgr_channel* chan;

    fetch_data_handler data_handler;
    fetch_end_handler end_handler;
    void* handler_state;

    bool collect_data;
    uint8_t* collect_buf;
    size_t collect_len;

    struct fetch_pending_request* next;
};

typedef struct fetch_ctx fetch_ctx;
typedef struct fetch_pending_request fetch_pending_request;

fetch_ctx* fetch_init(channelmgr_ctx* channelmgr);
fetch_pending_request* fetch_block(fetch_ctx* ctx, spotify_file_id file, uint32_t from, uint32_t to, fetch_end_handler handler, void* state);
fetch_pending_request* fetch_stream(fetch_ctx* ctx, spotify_file_id file, uint32_t from, uint32_t to, fetch_data_handler data_handler, fetch_end_handler end_handler, void* state);
void fetch_destroy(fetch_ctx* ctx);

#endif