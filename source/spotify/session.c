#include "session.h"

#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <switch/services/set.h>
#include "handshake.h"
#include "log.h"
#include "conv.h"

#include "proto/authentication.pb-c.h"
#include "proto/keyexchange.pb-c.h"

int session_message_handler(session_ctx* ctx, uint8_t cmd, uint8_t* buf, uint16_t len, void* _);

session_ctx* session_init(hs_res* handshake)
{
    session_ctx* ctx = malloc(sizeof(session_ctx));
    if (ctx == NULL)
    {
        return NULL;
    }

    memset(ctx, 0, sizeof(session_ctx));

    ctx->ciphers = handshake->ciphers;
    ctx->socket_desc = handshake->socket_desc;
    ctx->parser.status = SPS_Empty;
    ctx->auth.status = SAS_Unauthorized;
    ctx->listeners = NULL;
    ctx->time_delta = 0;
    ctx->country = NULL;

    free(handshake);

    session_add_message_listener(ctx, session_message_handler, NULL);

    log("[SESSION] Initialized!\n");

    return ctx;
}

#pragma region Message parsing and events

int session_message_handler(session_ctx* ctx, uint8_t cmd, uint8_t* buf, uint16_t len, void* _)
{
    if (cmd == 0x04)
    {
        uint32_t server_time = conv_b2u32(buf);
        int64_t unix_time = time(NULL);

        ctx->time_delta = server_time - unix_time;
        log("[SESSION] Time delta between server and client: %d\n", ctx->time_delta);
    }
    else if (cmd == 0x1B)
    {
        ctx->country = malloc(len + 1);
        memcpy(ctx->country, buf, len);
        ctx->country[len] = 0;

        log("[SESSION] Country: %s\n", ctx->country);
    }

    return 0;
}

int session_announce_message(session_ctx* ctx, uint8_t cmd, uint8_t* buf, uint16_t len)
{
    log_debug("[SESSION] Received command 0x%02X with a %d-long buffer\n", cmd, len);

    session_message_listener_entry* ptr = ctx->listeners;
    session_message_listener_entry* next = NULL;

    while (ptr != NULL)
    {
        next = ptr->next; // Entry might self-destruct and free itself after running.

        if (ptr->func(ctx, cmd, buf, len, ptr->state) < 0)
        {
            return -1;
        }

        ptr = next;
    }

    return 0;
}

void session_add_message_listener(session_ctx* ctx, session_message_listener func, void* state)
{
    session_message_listener_entry* entry = malloc(sizeof(session_message_listener_entry));
    entry->func = func;
    entry->state = state;
    entry->next = ctx->listeners;
    ctx->listeners = entry;
}

void session_remove_message_listener(session_ctx* ctx, session_message_listener func)
{
    session_message_listener_entry* prev = NULL;
    session_message_listener_entry* ptr = ctx->listeners;
    
    while (ptr != NULL)
    {
        if (ptr->func == func)
        {
            if (prev == NULL)
            {
                ctx->listeners = ptr->next;
            }
            else
            {
                prev->next = ptr->next;
            }

            free(ptr);
            break;
        }

        prev = ptr;
        ptr = ptr->next;
    }
}

int session_update(session_ctx* ctx)
{
    bool success = false;
    int res = 0;

    while (true)
    {
        if (ctx->parser.status == SPS_Empty)
        {
            uint8_t header[3];

            res = recv(ctx->socket_desc, header, 3, MSG_DONTWAIT);

            if (res < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    // Header has not arrived yet. We're done with our processing.
                    break;
                }
                else
                {
                    log_error("[SESSION] Failed to recv (SPS_Empty), errno %d\n", errno);
                    goto cleanup;
                }
            }

            shn_nonce_u32(&ctx->ciphers.decode_cipher, ctx->ciphers.decode_nonce);
            ctx->ciphers.decode_nonce++;

            shn_decrypt(&ctx->ciphers.decode_cipher, header, 3);

            ctx->parser.cmd = header[0];
            ctx->parser.size = conv_b2u16(header + 1) + SESSION_MAC_SIZE;
            log_debug("[SESSION] Receiving message (cmd 0x%02X) of size %d\n", ctx->parser.cmd, ctx->parser.size);

            ctx->parser.pile = malloc(ctx->parser.size);
            ctx->parser.pile_len = 0;

            if (ctx->parser.pile == NULL)
            {
                log_error("[SESSION] Failed to malloc size %d\n", ctx->parser.size);
                goto cleanup;
            }

            memset(ctx->parser.pile, 0, ctx->parser.size);

            ctx->parser.status = SPS_HasHeader;
        }
        else if (ctx->parser.status == SPS_HasHeader)
        {
            res = recv(ctx->socket_desc, ctx->parser.pile + ctx->parser.pile_len, ctx->parser.size - ctx->parser.pile_len, MSG_DONTWAIT);

            if (res < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    // Content has not arrived yet. We're done with our processing.
                    break;
                }
                else
                {
                    log_error("[SESSION] Failed to recv (SRS_HasHeader), errno %d\n", errno);
                    goto cleanup;
                }
            }

            if (res == 0)
            {
                // Content has not arrived yet. We're done with our processing.
                break;
            }

            ctx->parser.pile_len += res;

            if (ctx->parser.pile_len == ctx->parser.size)
            {
                // Packet has been fully received, decrypt & hand it off.

                // Decrypt payload part of pile
                shn_decrypt(&ctx->ciphers.decode_cipher, ctx->parser.pile, ctx->parser.size - SESSION_MAC_SIZE);

                // Get calculated MAC
                uint8_t actual_mac[SESSION_MAC_SIZE];
                shn_finish(&ctx->ciphers.decode_cipher, actual_mac, SESSION_MAC_SIZE);

                uint8_t* expected_mac = ctx->parser.pile + ctx->parser.size - SESSION_MAC_SIZE;
                
                // Check if expected and actual MACs match
                bool macs_match = true;
                for (int i = 0; i < SESSION_MAC_SIZE && macs_match; i++)
                {
                    if (expected_mac[i] != actual_mac[i])
                    {
                        macs_match = false;
                    }
                }

                if (!macs_match)
                {
                    log_error("[SESSION] MAC mismatch :(\n");
                    goto cleanup;
                }

                if (session_announce_message(ctx, ctx->parser.cmd, ctx->parser.pile, ctx->parser.size - SESSION_MAC_SIZE) < 0)
                {
                    free(ctx->parser.pile);
                    goto cleanup;
                }
                free(ctx->parser.pile);

                ctx->parser.status = SPS_Empty;
            }
        }
        else
        {
            log_error("[SESSION] Unexpected parser status %d ?!?!?!?!\n", ctx->parser.status);
            goto cleanup;
        }
    }

    success = true;

cleanup:
    if (success)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

#pragma endregion Message parsing and events

int session_send_message(session_ctx* ctx, uint8_t cmd, uint8_t* buf, uint16_t len)
{
    uint8_t* packet = malloc(3 + len + SESSION_MAC_SIZE);
    if (packet == NULL)
    {
        log_error("[SESSION] Failed to allocate memory for packet 0x%02X (length %d)\n", cmd, len);
        return -1;
    }

    packet[0] = cmd;
    packet[1] = len >> 8;
    packet[2] = len;
    memcpy(packet + 3, buf, len);

    shn_nonce_u32(&ctx->ciphers.encode_cipher, ctx->ciphers.encode_nonce);
    ctx->ciphers.encode_nonce++;
    shn_encrypt(&ctx->ciphers.encode_cipher, packet, len + 3);
    shn_finish(&ctx->ciphers.encode_cipher, packet + 3 + len, SESSION_MAC_SIZE);

    if (send(ctx->socket_desc, packet, len + 3 + SESSION_MAC_SIZE, 0) < 0)
    {
        log_error("[SESSION] Failed to send packet 0x%02X (length %d, errno %d)\n", cmd, len, errno);
        free(packet);
        return -1;
    }

    free(packet);
    return 0;
}

#pragma region Authentication

int session_send_client_response_encryped(session_ctx* ctx, char* username, char* password, char* device_id)
{
    LoginCredentials creds;
    login_credentials__init(&creds);
    creds.username = username;
    creds.typ = AUTHENTICATION_TYPE__AUTHENTICATION_USER_PASS;
    creds.has_auth_data = 1;
    creds.auth_data.data = (uint8_t*)password;
    creds.auth_data.len = strlen(password);

    SystemInfo sysinfo;
    system_info__init(&sysinfo);
    sysinfo.cpu_family = CPU_FAMILY__CPU_ARM;
    sysinfo.os = OS__OS_UNKNOWN; // no option for HOS :D
    sysinfo.system_information_string = "librespot-esque switchspot-" SWITCHSPOT_VERSION; // system_information_string must start with "librespot", otherwise we'll get denied by APs
    sysinfo.device_id = device_id;

    ClientResponseEncrypted message;
    client_response_encrypted__init(&message);
    message.login_credentials = &creds;
    message.system_info = &sysinfo;
    message.version_string = "switchspot-" SWITCHSPOT_VERSION;

    size_t buf_len = client_response_encrypted__get_packed_size(&message);
    uint8_t* buf = malloc(buf_len);
    client_response_encrypted__pack(&message, buf);

    int res = session_send_message(ctx, 0xAB, buf, buf_len);

    free(buf);
    return res;
}

int session_auth_result_handler(session_ctx* ctx, uint8_t cmd, uint8_t* buf, uint16_t len, void* _arh)
{
    void (*auth_result_handler)(session_ctx* ctx, bool success) = _arh;

    if (cmd == 0xAC)
    {
        APWelcome* message = apwelcome__unpack(NULL, len, buf);
        if (message == NULL)
        {
            log_error("[SESSION] Failed to unpack APWelcome!\n");
            return -1;
        }

        ctx->auth.status = SAS_Success;
        ctx->auth.username = malloc(strlen(message->canonical_username) + 1);
        memcpy(ctx->auth.username, message->canonical_username, strlen(message->canonical_username) + 1);
        ctx->auth.auth_type = message->reusable_auth_credentials_type;
        ctx->auth.token_len = message->reusable_auth_credentials.len;
        ctx->auth.token = malloc(ctx->auth.token_len);
        memcpy(ctx->auth.token, message->reusable_auth_credentials.data, ctx->auth.token_len);

        log("[SESSION] Authentication successful!\n[SESSION] Username: %s\n", ctx->auth.username);

        apwelcome__free_unpacked(message, NULL);
        
        if (auth_result_handler != NULL)
        {
            auth_result_handler(ctx, true);
        }
    }
    else if (cmd == 0xAD)
    {
        APLoginFailed* message = aplogin_failed__unpack(NULL, len, buf);
        aplogin_failed__free_unpacked(message, NULL);

        if (auth_result_handler != NULL)
        {
            char* err_message = "<no message specified>";

            if (message->error_description != NULL)
            {
                err_message = message->error_description;
            }
            log_error("[SESSION] Authentication failed!\n\"%s\" (0x%02X)\n", err_message, message->error_code);
            if (message->has_expiry)
            {
                log("[SESSION] Error expires in: %d\n", message->expiry);
            }
            if (message->retry_delay)
            {
                log("[SESSION] Authentication should be retried in: %d\n", message->retry_delay);
            }
            auth_result_handler(ctx, false);
        }
    }
    else
    {
        log_warn("[SESSION] WARNING: session_auth_result_handler alive for non-auth commands\n");
        return 0;
    }

    session_remove_message_listener(ctx, session_auth_result_handler);
    return 0;
}

int session_authenticate(session_ctx* ctx, char* username, char* password, void (*result)(session_ctx* ctx, bool success))
{
    char* device_id;
    SetSysDeviceNickName* nick = malloc(sizeof(SetSysDeviceNickName));
    Result nick_res = 0;
    if ((nick_res = setsysGetDeviceNickname(nick)) != 0)
    {
        log_warn("[SESSION] WARNING: Failed to get device nickname (err %d). Defaulting to \"Nintendo Switch\".\n", nick_res);
        device_id = "Nintendo Switch";
    }
    else
    {
        device_id = nick->nickname;
    }

    log("[SESSION] Authenticating with user %s (device name: %s)\n", username, device_id);

    session_add_message_listener(ctx, session_auth_result_handler, result);

    if (session_send_client_response_encryped(ctx, username, password, device_id) < 0)
    {
        log_error("[SESSION] Failed to send ClientResponseEncrypted message!\n");
        return -1;
    }

    free(nick);

    return 0;
}

#pragma endregion Authentication

void session_destroy(session_ctx* ctx)
{
    // TODO: This might be memory-leaky. Check it out.

    // Traverse listener linked list and free all elements
    session_message_listener_entry *ptr = ctx->listeners, *next;
    while (ptr != NULL)
    {
        next = ptr->next;

        free(ptr);

        ptr = next;
    }

    free(ctx);
}