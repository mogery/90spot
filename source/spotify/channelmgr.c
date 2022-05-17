
#include "channelmgr.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "session.h"
#include "log.h"
#include "idtool.h"
#include "conv.h"

int channelmgr_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* ctx);

channelmgr_ctx* channelmgr_init(session_ctx* session)
{
    channelmgr_ctx* res = malloc(sizeof(channelmgr_ctx));
    memset(res, 0, sizeof(channelmgr_ctx));

    res->session = session;
    res->next_seq = 0;
    res->channels = NULL;

    session_add_message_listener(session, channelmgr_message_listener, res);

    return res; 
}

uint32_t channelmgr_next_seq(channelmgr_ctx* ctx, uint8_t* seq)
{
    conv_u322b(seq, ctx->next_seq);

    return (ctx->next_seq++);
}

int channelmgr_message_listener(session_ctx* session, uint8_t cmd, uint8_t* buf, uint16_t len, void* _ctx)
{
    channelmgr_ctx* ctx = _ctx;

    if (cmd != 0x9 && cmd != 0xa)
        return 0;

    uint16_t seq = conv_b2u16(buf);
    channelmgr_channel* chan = ctx->channels;
    while (chan != NULL)
    {
        if (chan->seq == seq)
            break;
        
        chan = chan->next;
    }

    if (chan == NULL)
    {
        log_warn("[CHANNELMGR] Called for seq %u, channel not found?!\n", seq);
        return 0;
    }

    if (cmd == 0xa)
    {
        uint16_t code = conv_b2u16(buf + 2);
        log_error("[CHANNELMGR] Error on channel %u, code %u\n", seq, code);
        return -1;
    }
    else if (cmd == 0x9)
    {

        if (chan->state == CS_Header)
        {
            bool ended = false;
            uint8_t* ptr = buf + 2;

            while (ptr != buf + len)
            {
                channelmgr_header* head = malloc(sizeof(channelmgr_header));
                head->len = conv_b2u16(ptr);

                if (head->len == 0)
                {
                    free(head);
                    ptr += 2;
                    ended = true;
                    break;
                }

                head->id = ptr[2];
                head->buf = malloc(head->len - 1);
                memcpy(head->buf, ptr + 3, head->len - 1);
                head->next = chan->headers;
                chan->headers = head;

                ptr += 2 + head->len;
            }

            if (ptr != buf + len)
                log_warn("[CHANNELMGR] PTR %p != BUF + LEN %p\n", ptr, buf + len);
            
            if (ended)
            {
                chan->state = CS_Data;

                if (chan->header_handler(ctx, chan->headers, chan->handler_state) < 0)
                    return -1;
            }
            
        }
        else if (chan->state == CS_Data)
        {
            if (len == 2) // if buf is only seq, no data
            {
                log_debug("[CHANNELMGR] Channel %u closed\n", seq);
                chan->state = CS_Closed;
                
                if (chan->close_handler(ctx, chan->handler_state) < 0)
                    return -1;

                goto skip;
            }

            if (chan->data_handler(ctx, buf + 2, len - 2, chan->data_handler) < 0)
                return -1;
        }
        else if (chan->state == CS_Closed)
        {
            log_warn("[CHANNELMGR] Received packet for closed channel %u\n", seq);;
        }
    }

skip:
    return 0;
}

void channelmgr_destroy(channelmgr_ctx* ctx)
{
    session_remove_message_listener(ctx->session, channelmgr_message_listener);

    // Free all pending messages
    channelmgr_channel* cptr = ctx->channels;
    while (cptr != NULL)
    {
        channelmgr_channel* cnext = cptr->next;

        channelmgr_header* hptr = cptr->headers;
        while (hptr != NULL)
        {
            channelmgr_header* hnext = hptr->next;
            free(hptr->buf);
            free(hptr);
            hptr = hnext;
        }

        free(cptr);
        cptr = cnext;
    }

    free(ctx);
}