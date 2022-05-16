#include <stdlib.h>
#include "session.h"

#include "proto/mercury.pb-c.h"

#ifndef _audiokey_H
#define _audiokey_H

struct audiokey_ctx {
    session_ctx* session;

    uint64_t next_seq;
    struct audiokey_pending_request* requests;
};

// Return <0 for an error.
typedef int (*audiokey_response_handler)(
    struct audiokey_ctx* ctx, // The AudioKey context the response originates from
    uint8_t* key, // The key for the requested file. If NULL, request failed.
    size_t len, // The key's length. Always 20, but it's good practice to use this in case it changes. If 0, request failed.
    void* state // Optional state arg
);

struct audiokey_pending_request {
    uint32_t seq;

    audiokey_response_handler handler;
    void* handler_state;

    struct audiokey_pending_request* next;
};

typedef struct audiokey_ctx audiokey_ctx;
typedef struct audiokey_pending_request audiokey_pending_request;

audiokey_ctx* audiokey_init(session_ctx* session);
int audiokey_request(audiokey_ctx* ctx, spotify_id track, spotify_file_id file, audiokey_response_handler handler, void* state);
void audiokey_destroy(audiokey_ctx* ctx);

#endif