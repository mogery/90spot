#include "session.h"

#ifndef _mercury_H
#define _mercury_H

struct mercury_ctx {
    session_ctx* session;

    uint64_t next_seq;
    struct mercury_pending_message* messages;
};

struct mercury_pending_message {
    uint64_t seq;

    uint8_t* buf;
    size_t len;

    struct mercury_pending_message* next;
};

struct mercury_payload {
    uint8_t* buf;
    uint16_t len;

    struct mercury_payload* next;
};

typedef struct mercury_ctx mercury_ctx;
typedef struct mercury_pending_message mercury_pending_message;
typedef struct mercury_payload mercury_payload;

mercury_ctx* mercury_init(session_ctx* session);
int mercury_get_request(mercury_ctx* ctx, char* uri);
void mercury_destroy(mercury_ctx* ctx);

#endif