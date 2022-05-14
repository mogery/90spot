#include <stdio.h>
#include "mini-gmp.h"

#ifndef _dh_H
#define _dh_H

#define DH_PRIME "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffffffffffff"
#define DH_SIZE 96
#define DH_GENERATOR 2

typedef struct {
    mpz_t public;
    mpz_t private;
} dh_keys;

void dh_init();
dh_keys dh_keygen();
void dh_shared_key(mpz_t* out, mpz_t remote_key, mpz_t private_key);

void buf2mpz(mpz_t* out, uint8_t* buf, size_t len);
uint8_t* mpz2buf(size_t* size, mpz_t inp);

#endif