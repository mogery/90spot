#include "session.h"

#ifndef _channelmgr_H
#define _channelmgr_H

struct channelmgr_ctx {
    session_ctx* session;

    uint32_t next_seq;
    struct channelmgr_channel* channels;
};

typedef enum {
    CS_Header,
    CS_Data,
    CS_Closed
} channelmgr_channel_state;

struct channelmgr_header {
    uint8_t id;
    uint8_t* buf;
    uint16_t len;
    
    struct channelmgr_header* next;
};

// Return <0 for an error.
typedef int (*channelmgr_header_handler)(
    struct channelmgr_ctx* ctx, // The ChannelMgr context the response originates from
    struct channelmgr_header* headers, // header linked list
    void* state // Optional state arg
);

// Return <0 for an error.
typedef int (*channelmgr_data_handler)(
    struct channelmgr_ctx* ctx, // The ChannelMgr context the response originates from
    uint8_t* buf, // Data buffer
    uint16_t len, // Data buffer length
    void* state // Optional state arg
);

// Return <0 for an error.
typedef int (*channelmgr_close_handler)(
    struct channelmgr_ctx* ctx, // The ChannelMgr context the response originates from
    void* state // Optional state arg
);

struct channelmgr_channel {
    uint16_t seq;

    channelmgr_channel_state state;
    struct channelmgr_header* headers;

    channelmgr_header_handler header_handler;
    channelmgr_data_handler data_handler;
    channelmgr_close_handler close_handler;
    void* handler_state;

    struct channelmgr_channel* next;
};

typedef struct channelmgr_ctx channelmgr_ctx;
typedef struct channelmgr_header channelmgr_header;
typedef struct channelmgr_channel channelmgr_channel;

channelmgr_ctx* channelmgr_init(session_ctx* session);
void channelmgr_destroy(channelmgr_ctx* ctx);

#endif