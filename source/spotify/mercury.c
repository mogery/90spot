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

int mercury_general_request(mercury_ctx* ctx, char* uri, uint8_t method_code, char* method, mercury_payload* payloads, mercury_response_handler handler, void* state)
{
    uint8_t seq_buf[8];
    uint64_t seq = mercury_next_seq(ctx, seq_buf);

    uint8_t* packet;
    size_t packet_len = mercury_encode_request(&packet, seq_buf, 8, uri, "GET", payloads);

    log_debug("[MERCURY] %ld %s with seq %ld, size %ld\n", seq, uri, seq, packet_len);

    mercury_pending_message* msg = malloc(sizeof(mercury_pending_message));
    msg->seq = seq;
    msg->parts = NULL;
    msg->next = ctx->messages;
    msg->handler = handler;
    msg->handler_state = state;
    ctx->messages = msg;

    if (session_send_message(ctx->session, 0xB2, packet, packet_len) < 0)
    {
        log_error("[MERCURY] Failed to send %s request!", method);
        return -1;
    }

    return 0;
}

int mercury_get_request(mercury_ctx* ctx, char* uri, mercury_response_handler handler, void* state)
{
    return mercury_general_request(ctx, uri, 0xB2, "GET", NULL, handler, state);
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

        mercury_response_part* part = malloc(sizeof(mercury_response_part));
        part->len = part_len;
        part->next = NULL;

        // We need to allocate and copy here because session
        // will free buf after this handler is done running.
        part->buf = malloc(part_len);
        memcpy(part->buf, ptr, part->len);

        if (msg->parts == NULL)
        {
            msg->parts = part;
        }
        else
        {
            // We need to insert the part to the end of the list
            // instead of the usual "slap it onto the beginning"
            //

            // Traverse to the end of the linked list
            mercury_response_part* rptr = msg->parts;
            while (rptr->next != NULL)
                rptr = rptr->next;
            
            // Insert part to the end
            rptr->next = part;
        }

        ptr += part_len;
    }

    log("mercury leftovers: %ld (overall len: %d)\n", ptr - buf, len);

    if (flags == 0x1)
    {
        mercury_response_part* header_part = NULL;
        int res = 0;

        // Make sure we have at least 1 part (Header)
        if (msg->parts == NULL)
        {
            log_warn("[MERCURY] Request (seq %ld) arrived with 0 parts. Skipping handler.\n", seq);
            goto skip_handler;
        }

        // Parse Header part
        header_part = msg->parts;
        msg->parts = header_part->next; // Skip header in msg->parts
        Header* header = header__unpack(NULL, header_part->len, header_part->buf);
        if (header == NULL)
        {
            log_warn("[MERCURY] Failed to parse header for request (seq %ld). Skipping handler.\n", seq);
            goto skip_handler;
        }
        log_debug("[MERCURY] %ld %s %d\n", seq, header->method, header->status_code);

        if (msg->handler != NULL && msg->handler(ctx, header, msg->parts, msg->handler_state) < 0)
        {
            log_debug("[MERCURY] Message handler failed, passing error through...\n");
            res = -1;
        }

        // Free unpacket header
        header__free_unpacked(header, NULL);

skip_handler:
        // Free raw header part
        if (header_part != NULL)
        {
            free(header_part->buf);
            free(header_part);
        }
        
        mercury_response_part* fptr = msg->parts;
        while (fptr != NULL)
        {
            mercury_response_part* next = fptr->next;

            free(fptr->buf);
            free(fptr);

            fptr = next;
        }

        // Remove message from pending message list
        mercury_pending_message* mptr = ctx->messages;
        while (mptr != NULL)
        {
            if (mptr->next == msg)
            {
                mptr->next = mptr->next->next;
                break;
            }

            mptr = mptr->next;
        }

        // Free message
        free(msg);

        if (res < 0) return res;
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