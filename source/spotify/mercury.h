#include "session.h"
#include "proto/mercury.pb-c.h"

#ifndef _mercury_H
#define _mercury_H

struct mercury_ctx {
    session_ctx* session;

    uint64_t next_seq;
    struct mercury_pending_message* messages;
};

struct mercury_message_part {
    uint8_t* buf;
    uint16_t len;

    struct mercury_message_part* next;
};

// Return <0 for an error.
typedef int (*mercury_response_handler)(
    struct mercury_ctx* ctx, // The Mercury context the response originates from
    Header* header, // Mercury response header. Handler should check status_code
    struct mercury_message_part* parts, // The parts of the rest of the message. Handler should be prepared for this to be NULL.
    void* state // Optional state arg
);

struct mercury_pending_message {
    uint64_t seq;

    struct mercury_message_part* parts;

    mercury_response_handler handler;
    void* handler_state;

    struct mercury_pending_message* next;
};

typedef struct mercury_ctx mercury_ctx;
typedef struct mercury_pending_message mercury_pending_message;
typedef struct mercury_message_part mercury_message_part;

mercury_ctx* mercury_init(session_ctx* session);
int mercury_get_request(mercury_ctx* ctx, char* uri, mercury_response_handler handler, void* state);
int mercury_send_request(mercury_ctx* ctx, char* uri, uint8_t* buf, uint16_t len, mercury_response_handler handler, void* state);
void mercury_destroy(mercury_ctx* ctx);

#endif