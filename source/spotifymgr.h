#pragma once
#include <stdlib.h>

#include "protobuf-c.h"
#include "dh.h"
#include "spotify/apresolve.h"
#include "spotify/handshake.h"
#include "spotify/session.h"
#include "spotify/mercury.h"
#include "spotify/idtool.h"
#include "spotify/audiokey.h"
#include "spotify/channelmgr.h"
#include "spotify/fetch.h"
#include "spotify/audiofetch.h"
#include "audioplay.h"

#include "spotify/proto/metadata.pb-c.h"

int spotifymgr_init();
int spotifymgr_update();
int spotifymgr_authenticate(char* username, char* password, int (*callback)(void*), void* state);
int spotifymgr_demo();
void spotifymgr_destroy();