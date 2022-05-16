#include "mercury.h"

#include <stdio.h>
#include <string.h>
#include "log.h"
#include "conv.h"

#include "proto/mercury.pb-c.h"

int mercury_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* ctx);

mercury_ctx* mercury_init(session_ctx* session)
{   
    mercury_ctx* res = malloc(sizeof(mercury_ctx));
    memset(res, 0, sizeof(mercury_ctx));

    res->session = session;
    res->next_seq = 0;
    res->messages = NULL;

    session_add_message_listener(session, mercury_message_listener, res);

    return res;
}

#pragma region Utilities

uint64_t mercury_next_seq(mercury_ctx* ctx, uint8_t* seq)
{
    conv_u642b(seq, ctx->next_seq);

    return (ctx->next_seq++);
}

uint64_t mercury_buf_to_seq(uint8_t* buf, uint16_t len)
{
    uint64_t acc = 0;

    for (int i = len - 1; i >= 0; i--)
    {
        acc |= ((uint64_t)buf[i]) >> ((len - i - 1) * 8);
    }

    return acc;
}

#pragma endregion Utilities

#pragma region Requests

size_t mercury_encode_request(uint8_t** out, uint8_t* seq, uint16_t seq_len, char* uri, char* method, mercury_payload* payloads)
{
    // Assemble protobuf header
    Header header;
    header__init(&header);
    header.uri = uri;
    header.method = method;
    uint16_t header_size = header__get_packed_size(&header);

    // Calculate the count & byte size of provided payloads
    mercury_payload* ptr = payloads;
    uint16_t payload_count = 1; // this is 1 instead of 0 because the header counts as a payload
    size_t payloads_size = 0;
    while (ptr != NULL)
    {
        payload_count++;
        payloads_size += ptr->len + 2;
        ptr = ptr->next;
    }

    size_t full_len = 2 + seq_len + 1 + 2 + 2 + header_size + payloads_size;
    uint8_t* packet = malloc(full_len);

    // Copy sequence to packet
    packet[0] = seq_len >> 8;
    packet[1] = seq_len;
    memcpy(packet + 2, seq, seq_len);

    // Write FINAL flag
    packet[2 + seq_len] = 1;

    // Write number of payloads
    packet[2 + seq_len + 1] = payload_count >> 8;
    packet[2 + seq_len + 1 + 1] = payload_count;

    // Write size of protobuf header
    packet[2 + seq_len + 1 + 2] = header_size >> 8;
    packet[2 + seq_len + 1 + 2 + 1] = header_size;

    // Write protobuf header
    header__pack(&header, packet + 2 + seq_len + 1 + 2 + 2);

    // Copy payloads to packet
    uint8_t* pptr = packet + 2 + seq_len + 1 + 2 + 2 + header_size;
    ptr = payloads;
    while (ptr != NULL)
    {
        *(pptr++) = ptr->len >> 8;
        *(pptr++) = ptr->len;
        memcpy(pptr, ptr->buf, ptr->len);
        pptr += ptr->len;
    }

    *out = packet;

    return full_len;
}

int mercury_get_request(mercury_ctx* ctx, char* uri)
{
    // 
    //0xB2

    uint8_t seq_buf[8];
    uint64_t seq = mercury_next_seq(ctx, seq_buf);

    uint8_t* packet;
    size_t packet_len = mercury_encode_request(&packet, seq_buf, 8, uri, "GET", NULL);

    log_debug("[MERCURY] GET %s with seq %ld, size %ld\n", uri, seq, packet_len);

    mercury_pending_message* msg = malloc(sizeof(mercury_pending_message));
    msg->seq = seq;
    msg->buf = NULL;
    msg->len = 0;
    msg->next = ctx->messages;
    ctx->messages = msg;

    if (session_send_message(ctx->session, 0xB2, packet, packet_len) < 0)
    {
        log_error("[MERCURY] Failed to send GET request!");
        return -1;
    }

    return 0;
}

#pragma endregion Requests

#pragma region Message handling

int mercury_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* _ctx) {
    mercury_ctx* ctx = _ctx;

    if (cmd < 0xb2 || cmd > 0xb6)
    {
        return 0;
    }

    uint16_t seq_len = conv_b2u16(buf);
    uint8_t* seq_buf = buf + 2;
    uint8_t flags = buf[seq_len + 2];
    uint16_t count = conv_b2u16(buf + seq_len + 3);

    uint64_t seq = mercury_buf_to_seq(seq_buf, seq_len);

    log_debug("[MERCURY] Received command %02X for seq %ld (flags: %02X, count: %d)\n", cmd, seq, flags, count);

    mercury_pending_message* msg = ctx->messages;
    while (msg != NULL)
    {
        if (msg->seq == seq)
            break;
        
        msg = msg->next;
    }

    if (msg == NULL)
    {
        log_debug("[MERCURY] Seq %ld not found in message list, ignoring.\n", seq);
        return 0;
    }

    uint8_t* ptr = buf + seq_len + 5;
    for (int i = 0; i < count; i++)
    {
        uint16_t part_len = conv_b2u16(ptr);
        ptr += 2;

        log_debug("[MERCURY] Part %d length %d\n", i, part_len);

        if (msg->len == 0)
        {
            msg->buf = malloc(part_len);
        }
        else
        {
            msg->buf = realloc(msg->buf, msg->len + part_len);
        }

        memcpy(msg->buf + msg->len, ptr, part_len);

        msg->len += part_len;
        ptr += part_len;
    }

    if (flags == 0x1)
    {
        log_debug("[MERCURY] Request with seq %ld completed!\n", seq);
    }
    
    // TODO: handle subscriptions (0xB5)

    return 0;
}

#pragma endregion Message handling

void mercury_destroy(mercury_ctx* ctx)
{
    session_remove_message_listener(ctx->session, mercury_message_listener);

    free(ctx);
}