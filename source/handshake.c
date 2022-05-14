#include "handshake.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <switch/kernel/random.h>
#include "mini-gmp.h"
#include "Shannon.h"
#include "hmac.h"

#include "proto/keyexchange.pb-c.h"

uint8_t* add_to_accumulator(uint8_t* accumulator, size_t acc_size, uint8_t* new, size_t new_size)
{
    uint8_t* new_acc = NULL;

    if (accumulator == NULL)
    {
        new_acc = malloc(new_size);
    }
    else
    {
        new_acc = realloc(accumulator, acc_size + new_size);
    }

    memcpy(new_acc + acc_size, new, new_size);

    return new_acc;
}

int client_hello(uint8_t** accumulator, size_t* acc_size, int socket_desc, uint8_t* public_key) {
    bool success = false;

    BuildInfo build_info;
    build_info__init(&build_info);
    build_info.product = PRODUCT__PRODUCT_PARTNER;
    build_info.platform = PLATFORM__PLATFORM_LINUX_ARM;
    build_info.version = 109800078;

    Cryptosuite* cryptosuites_supported = malloc(sizeof(Cryptosuite) * 1);
    cryptosuites_supported[0] = CRYPTOSUITE__CRYPTO_SUITE_SHANNON;

    LoginCryptoDiffieHellmanHello login_crypto_dh_hello;
    login_crypto_diffie_hellman_hello__init(&login_crypto_dh_hello);
    login_crypto_dh_hello.gc.data = public_key;
    login_crypto_dh_hello.gc.len = DH_SIZE;
    login_crypto_dh_hello.server_keys_known = 1;

    LoginCryptoHelloUnion login_crypto_hello;
    login_crypto_hello_union__init(&login_crypto_hello);
    login_crypto_hello.diffie_hellman = &login_crypto_dh_hello;

    ClientHello message;
    client_hello__init(&message);
    message.build_info = &build_info;
    message.n_cryptosuites_supported = 1;
    message.cryptosuites_supported = cryptosuites_supported;
    message.login_crypto_hello = &login_crypto_hello;

    uint8_t client_nonce[0x10];
    randomGet(client_nonce, 0x10);
    message.client_nonce.data = client_nonce;
    message.client_nonce.len = 0x10;

    message.has_padding = 1;
    uint8_t padding[1] = { 0x1e };
    message.padding.data = padding;
    message.padding.len = 1;

    uint32_t packet_size = 2 + 4 + client_hello__get_packed_size(&message);
    uint8_t* packet = malloc(packet_size);
    packet[0] = 0;
    packet[1] = 4;

    // Write message size as a big endian uint32_t
    packet[2] = packet_size >> 24;
    packet[3] = packet_size >> 16;
    packet[4] = packet_size >>  8;
    packet[5] = packet_size;

    client_hello__pack(&message, packet + 6);

    *accumulator = add_to_accumulator(*accumulator, *acc_size, packet, packet_size);
    *acc_size = *acc_size + packet_size;

    if (send(socket_desc, packet, packet_size, 0) < 0)
    {
        printf("[HANDSHAKE] Failed to send ClientHello :(\n");
        goto cleanup;
    }

    success = true;

    cleanup:
    free(cryptosuites_supported);
    free(packet);

    if (success)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

uint8_t* recv_generic_packet(uint32_t* size, int socket_desc)
{
    uint8_t* packet = NULL;

    uint8_t header[4];
    if (recv(socket_desc, header, 4, 0) < 0)
    {
        printf("[HANDSHAKE] Failed to read packet header\n");
        goto cleanup;
    }

    *size = (((uint32_t)header[0] << 24) | ((uint32_t)header[1] << 16) | ((uint32_t)header[2] << 8) | (uint32_t)header[3]);

    printf("[HANDSHAKE] Received packet of length %d\n", *size);

    packet = malloc(*size);
    if (packet == NULL)
    {
        printf("[HANDSHAKE] Failed to allocate buffer for packet content\n");
        goto cleanup;
    }

    memcpy(packet, header, 4);

    if (recv(socket_desc, packet + 4, *size - 4, 0) < 0)
    {
        printf("[HANDSHAKE] Failed to read packet content\n");
        goto cleanup;
    }

cleanup:
    return packet;
}

APResponseMessage* recv_ap_response(uint8_t** accumulator, size_t* acc_size, int socket_desc)
{
    APResponseMessage* message = NULL;

    uint32_t packet_size = 0;
    uint8_t* packet = recv_generic_packet(&packet_size, socket_desc);
    if (packet == NULL)
    {
        goto cleanup;
    }

    *accumulator = add_to_accumulator(*accumulator, *acc_size, packet, packet_size);

    message = apresponse_message__unpack(NULL, packet_size - 4, packet + 4);
    if (message == NULL)
    {
        printf("[HANDSHAKE] Failed to parse APResponse :(\n");
        goto cleanup;
    }

cleanup:
    if (packet != NULL)
    {
        free(packet);
    }

    return message;
}

hs_res* spotify_handshake(struct sockaddr_in *ap, dh_keys keys)
{
    hs_res* res = NULL;
    int socket_desc = -1;
    uint8_t* accumulator = NULL;
    size_t acc_size = 0;

    if (ap == NULL)
    {
        printf("[HANDSHAKE] AP is nullptr :(\n");
        goto cleanup;
    }

    uint8_t* public_key_bytes = mpz2buf(NULL, keys.public);

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("[HANDSHAKE] Could not create socket :(\n");
        goto cleanup;
    }

    if (connect(socket_desc, (struct sockaddr *)ap, sizeof(struct sockaddr_in)) < 0)
    {
        printf("[HANDSHAKE] Connection error :(\n");
        goto cleanup;
    }

    printf("[HANDSHAKE] Saying hello...\n");
    if (client_hello(&accumulator, &acc_size, socket_desc, public_key_bytes) == -1)
    {
        printf("[HANDSHAKE] client_hello failed :(\n");
        goto cleanup;
    }

    APResponseMessage* ap_response = recv_ap_response(&accumulator, &acc_size, socket_desc);

    printf("[HANDSHAKE] Received APResponse!\n");

    size_t remote_key_length = ap_response->challenge->login_crypto_challenge->diffie_hellman->gs.len;
    uint8_t* remote_key_buf = ap_response->challenge->login_crypto_challenge->diffie_hellman->gs.data;

    mpz_t remote_key;
    buf2mpz(&remote_key, remote_key_buf, remote_key_length);

    printf("[HANDSHAKE] Remote key:\n");
    mpz_out_str(stdout, 16, remote_key);
    printf("\n");

    mpz_t shared_key;
    dh_shared_key(&shared_key, remote_key, keys.private);

    printf("[HANDSHAKE] Shared key:\n");
    mpz_out_str(stdout, 16, shared_key);
    printf("\n");
    
    size_t shared_key_len = 0;
    uint8_t* shared_key_buf = mpz2buf(&shared_key_len, shared_key);

    uint8_t mac_buf[100];
    acc_size++;
    accumulator = realloc(accumulator, acc_size);

    for (int i = 1; i < 6; i++)
    {
        accumulator[acc_size - 1] = i;
        hmac_sha1(shared_key_buf, shared_key_len, accumulator, acc_size, mac_buf + ((i - 1) * 20));
    }

    uint8_t challenge[20];
    hmac_sha1(mac_buf, 20, accumulator, acc_size - 1, challenge);

    printf("[HANDSHAKE] Challenge:\n");
    for (int i = 0; i < 20; i++)
    {
        printf("%02x", challenge[i]);
    }
    printf("\n");

    shn_ctx encode_cipher, decode_cipher;

    shn_key(&encode_cipher, mac_buf + 20, 32);
    shn_key(&decode_cipher, mac_buf + 20 + 32, 32);

    hs_ciphers ciphers;
    ciphers.decode_cipher = decode_cipher;
    ciphers.encode_cipher = encode_cipher;

    res = malloc(sizeof(hs_res));
    res->ciphers = ciphers;
    res->socket_desc = socket_desc;
    
cleanup:
    if (res == NULL)
    {
        if (socket_desc != -1)
        {
            close(socket_desc);
        }
    }

    if (accumulator != NULL) {
        free(accumulator);
    }

    return res;
}