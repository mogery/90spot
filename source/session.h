#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include "Shannon.h"
#include "handshake.h"
#include "proto/authentication.pb-c.h"

#ifndef _session_H
#define _session_H

#define SESSION_MAC_SIZE 4

typedef enum {
    SPS_Empty = 0,
    SPS_HasHeader
} session_parser_status;

typedef enum {
    SAS_Unauthorized = 0,
    SAS_Pending,
    SAS_Success,
    SAS_Failure
} session_auth_status;

typedef struct {
    session_auth_status status;

    char* username;
    AuthenticationType auth_type;
    uint8_t* token;
    size_t token_len;
} session_auth_state;

typedef struct session_message_listener_entry session_message_listener_entry;

typedef struct {
    session_parser_status status;

    // Pending message stats
    uint8_t cmd; // Pending command ID
    uint16_t size; // Reported message size
    uint8_t* pile; // So-far collected message bytes
    size_t pile_len; // So-far collected message size
} session_parser_state;

typedef struct {
    int socket_desc;
    hs_ciphers ciphers;
    session_parser_state parser;
    session_auth_state auth;
    session_message_listener_entry* listeners;

    int32_t time_delta;
    char* country;
} session_ctx;

// Return <0 for an error.
typedef int (*session_message_listener)(
    session_ctx* ctx, // The session context the message originates from
    uint8_t cmd, // The command ID of the message
    uint8_t* buf, // The message buffer [DO NOT MODIFY]
    uint16_t len, // The message buffer's length
    void* state // Optional state arg
);

struct session_message_listener_entry {
    session_message_listener func;
    void* state;
    session_message_listener_entry* next;
};

session_ctx* session_init(hs_res* handshake);
int session_update(session_ctx* ctx);
void session_add_message_listener(session_ctx* ctx, session_message_listener func, void* state);
void session_remove_message_listener(session_ctx* ctx, session_message_listener func);
int session_authenticate(session_ctx* ctx, char* username, char* password, void (*result)(session_ctx* ctx, bool success));
void session_destroy(session_ctx* ctx);

#endif