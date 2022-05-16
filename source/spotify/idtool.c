#include "idtool.h"
#include "conv.h"
#include "log.h"
#include <string.h>

void to_base16(char* out, uint8_t* buf, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        uint8_t v = buf[i];
        out[i * 2] = BASE16_DIGITS[(v >> 4)];
        out[i * 2 + 1] = BASE16_DIGITS[(v & 0x0f)];
    }

    out[len * 2] = 0;
}

#pragma region spotify_id
spotify_id spotify_id_from_raw(uint8_t* src, spotify_audio_type type)
{
    spotify_id id;
    id.id = conv_b2u128(src);
    id.type = type;

    return id;
}

spotify_id spotify_id_from_b16(char* src, spotify_audio_type type)
{
    spotify_id id;
    id.id = 0;
    id.type = type;

    size_t len = strlen(src);

    for (int i = 0; i < len; i++)
    {
        char c = src[i];

        __uint128_t p = 0;
        if (c >= '0' && c <= '9')
            p = c - '0';
        else if (c >= 'a' && c <= 'f')
            p = c - 'a' + 10;
        else
            log_warn("[IDTOOL] Received unsupported character %c in _from_b16\n", c);

        id.id <<= 4;
        id.id += p;
    }

    return id;
}

spotify_id spotify_id_from_b62(char* src, spotify_audio_type type)
{
    spotify_id id;
    id.id = 0;
    id.type = type;

    size_t len = strlen(src);

    for (int i = 0; i < len; i++)
    {
        char c = src[i];

        __uint128_t p = 0;
        if (c >= '0' && c <= '9')
            p = c - '0';
        else if (c >= 'a' && c <= 'z')
            p = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z')
            p = c - 'A' + 36;
        else
            log_warn("[IDTOOL] Received unsupported character %c in _from_b62\n", c);

        id.id *= 62;
        id.id += p;
    }

    return id;
}

void spotify_id_to_raw(uint8_t* buf, spotify_id id)
{
    conv_u1282b(buf, id.id);
}

void spotify_id_to_b16(char* out, spotify_id id)
{
    uint8_t buf[SPOTIFY_ID_RAW_LENGTH];
    conv_u1282b(buf, id.id);
    to_base16(out, buf, SPOTIFY_ID_RAW_LENGTH);
}

void spotify_id_to_b62(char* out, spotify_id id)
{
    // based on: https://github.com/trezor/trezor-crypto/blob/c316e775a2152db255ace96b6b65ac0f20525ec0/base58.c
    uint8_t dst[SPOTIFY_ID_B62_LENGTH] = { 0 };
    size_t i = 0;
    __uint128_t n = id.id;

    for (int s = 3; s >= 0; s--)
    {
        int shift = 32 * s;
        uint64_t carry = (uint64_t)((uint32_t)(n >> shift));

        for (int t = 0; t < i; t++)
        {
            uint8_t* b = &dst[t];
            carry += ((uint64_t)*b) << 32;
            *b = (uint8_t)(carry % 62);
            carry /= 62;
        }

        while (carry > 0)
        {
            dst[i] = (uint8_t)(carry % 62);
            carry /= 62;
            i++;
        }
    }

    for (int x = 0; x < SPOTIFY_ID_B62_LENGTH; x++)
    {
        dst[x] = BASE62_DIGITS[dst[x]];
        out[SPOTIFY_ID_B62_LENGTH - x - 1] = (char)dst[x];
    }
    out[SPOTIFY_ID_B62_LENGTH] = 0;

}
#pragma endregion spotify_id

#pragma region spotify_file_id
void spotify_file_id_to_b16(char* out, spotify_file_id id)
{
    to_base16(out, id.id, SPOTIFY_FILE_ID_RAW_LENGTH);
}
#pragma endregion spotify_file_id
