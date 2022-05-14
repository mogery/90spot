#include "hmac.h"

/* function doing the HMAC-SHA-1 calculation */
void hmac_sha1(const uint8_t* key, const uint32_t keysize, const uint8_t* msg, const uint32_t msgsize, uint8_t* output)
{
  struct sha1 outer, inner;
  uint8_t tmp;

  sha1_reset(&outer);
  sha1_reset(&inner);

  uint32_t i;
  for (i = 0; i < keysize; ++i)
  {
    tmp = key[i] ^ 0x5C;
    sha1_input(&outer, &tmp, 1);
    tmp = key[i] ^ 0x36;
    sha1_input(&inner, &tmp, 1);
  }
  for (; i < 64; ++i)
  {
    tmp = 0x5C;
    sha1_input(&outer, &tmp, 1);
    tmp = 0x36;
    sha1_input(&inner, &tmp, 1);
  }

  sha1_input(&inner, msg, msgsize);
  sha1_result(&inner, output);

  sha1_input(&outer, output, HMAC_SHA1_HASH_SIZE);
  sha1_result(&outer, output);
}