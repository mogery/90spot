#include <arpa/inet.h>
#include <stdlib.h>
#include "dh.h"
#include "Shannon.h"

#ifndef _handshake_H
#define _handshake_H

typedef struct {
    shn_ctx decode_cipher;
    uint32_t decode_nonce;

    shn_ctx encode_cipher;
    uint32_t encode_nonce;
} hs_ciphers;

typedef struct {
    hs_ciphers ciphers;
    int socket_desc;
} hs_res;


hs_res* spotify_handshake(struct sockaddr_in *ap, dh_keys keys);

#endif