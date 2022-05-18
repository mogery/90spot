#include "dh.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <switch/runtime/env.h>
#include <switch/kernel/random.h>
#include "mini-gmp.h"
#include "Shannon.h"
#include "log.h"

mpz_t dh_prime, dh_generator;

void dh_init() {
    // TODO: SEED RANDOM FOR DH

    if (!envHasRandomSeed()) {
        log_warn("[DH] WARNING! Your random seed is not set. This will mean that you will use the same auth keys over and over again. Do not share traffic captures of 90spot with anyone!");
    }

    mpz_init_set_ui(dh_generator, DH_GENERATOR);
    mpz_init_set_str(dh_prime, DH_PRIME, 16);

    log_info("[DH] Initialized!\n");
}

dh_keys dh_keygen()
{
    dh_keys keys;

    mpz_init(keys.private);
    mpz_init(keys.public);

    uint8_t buf[DH_SIZE];
    randomGet(buf, DH_SIZE);
    mpz_import(keys.private, DH_SIZE, 1, 1, 1, 0, buf);

    mpz_powm(keys.public, dh_generator, keys.private, dh_prime);

    return keys;
}

void dh_shared_key(mpz_t* out, mpz_t remote_key, mpz_t private_key)
{
    mpz_init(*out);
    mpz_powm(*out, remote_key, private_key, dh_prime);
}

void buf2mpz(mpz_t* out, uint8_t* buf, size_t len)
{
    mpz_init(*out);
    mpz_import(*out, len, 1, 1, 1, 0, buf);
}

uint8_t* mpz2buf(size_t* size, mpz_t inp)
{
    return mpz_export(NULL, size, 1, 1, 1, 0, inp);
}