#include <stdio.h>

#ifndef _idtool_H
#define _idtool_H

#define BASE62_DIGITS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define BASE16_DIGITS "0123456789abcdef"

typedef enum {
    Track,
    Podcast,
    NonPlayable
} spotify_audio_type;

#define SPOTIFY_ID_RAW_LENGTH 16 // 128 / 8
#define SPOTIFY_ID_B62_LENGTH 22
#define SPOTIFY_ID_B16_LENGTH 32 // RAW_LENGTH * 2

typedef struct {
    __uint128_t id;
    spotify_audio_type type;
} spotify_id;

spotify_id spotify_id_from_raw(uint8_t* src, spotify_audio_type type);
spotify_id spotify_id_from_b16(char* src, spotify_audio_type type);
spotify_id spotify_id_from_b62(char* src, spotify_audio_type type);
void spotify_id_to_raw(uint8_t* buf, spotify_id id);
void spotify_id_to_b16(char* out, spotify_id id);
void spotify_id_to_b62(char* out, spotify_id id);

#define SPOTIFY_FILE_ID_RAW_LENGTH 20
#define SPOTIFY_FILE_ID_B16_LENGTH 40 // RAW_LENGTH * 2

typedef struct {
    uint8_t id[SPOTIFY_FILE_ID_RAW_LENGTH];
} spotify_file_id;

void spotify_file_id_to_b16(char* out, spotify_file_id id);

#endif