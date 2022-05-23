#include "spotifymgr.h"
#include "log.h"
#include "glob.h"
#include <string.h>

session_ctx* session = NULL;
mercury_ctx* mercury = NULL;
audiokey_ctx* audiokey = NULL;
channelmgr_ctx* channelmgr = NULL;
fetch_ctx* fetch = NULL;
audiofetch_ctx* audiofetch = NULL;
audioplay_ctx* audioplay = NULL;

#pragma region Init/Destroy
int spotifymgr_init()
{
    log_debug("[PROTOBUF] Version: %s\n", protobuf_c_version());

    if (!audioplay_svc_init())
        return -1;
    
    dh_init();
    dh_keys keys = dh_keygen();

    // Resolve a Spotify Access Point
    struct sockaddr_in* ap = apresolve();
    if (ap == NULL)
        return -1;

    // Handshake with AP
    hs_res* handshake = spotify_handshake(ap, keys);
    if (handshake == NULL)
        return -1;

    // Initialize session from handshake data & socket
    session = session_init(handshake);
    if (session == NULL)
        return -1;
    
    return 0;
}

void spotifymgr_destroy()
{
    if (audioplay != NULL)
        audioplay_destroy(audioplay);
    
    if (audiofetch != NULL)
        audiofetch_destroy(audiofetch);

    if (fetch != NULL)
        fetch_destroy(fetch);

    if (channelmgr != NULL)
        channelmgr_destroy(channelmgr);
    
    if (audiokey != NULL)
        audiokey_destroy(audiokey);

    if (mercury != NULL)
        mercury_destroy(mercury);

    if (session != NULL)
        session_destroy(session);

    audioplay_svc_cleanup();
}
#pragma endregion Init/Destroy

int spotifymgr_update()
{
    if (session_update(session) < 0)
        return -1;

    if (audioplay != NULL && audioplay_update(audioplay) < 0)
        return -1;
    
    return 0;
}

#pragma region Authentication

int (*authentication_callback)(void*);
void* authentication_callback_state = NULL;

void authentication_handler(session_ctx* session, bool success)
{
    if (!success)
    {
        panic();
    }

    // Initialize Mercury
    if ((mercury = mercury_init(session)) == NULL)
        panic();

    // Initialize AudioKey
    if ((audiokey = audiokey_init(session)) == NULL)
        panic();

    // Initialize ChannelMgr
    if ((channelmgr = channelmgr_init(session)) == NULL)
        panic();
    
    // Initialize Fetch
    if ((fetch = fetch_init(channelmgr)) == NULL)
        panic();
    
    // Initialize AudioFetch
    if ((audiofetch = audiofetch_init(fetch, audiokey)) == NULL)
        panic();
    
    // Initialize AudioPlay
    if ((audioplay = audioplay_init(audiofetch)) == NULL)
        panic();
    
    if (authentication_callback != NULL && authentication_callback(authentication_callback_state) < 0)
        panic();
}

int spotifymgr_authenticate(char* username, char* password, int (*callback)(void*), void* state)
{
    authentication_callback = callback;
    authentication_callback_state = state;
    
    // Authenticate session
    if (session_authenticate(session, username, password, authentication_handler) < 0)
        return -1;

    return 0;
}
#pragma endregion Authentication

#pragma region Demo
int demo_request_handler(mercury_ctx* mercury, Header* header, mercury_message_part* parts, void* _)
{
    log_info("Request handler called!\n");

    if (header->status_code >= 300)
    {
        log_error("Bad status code! Halting.");
        return -1;
    }

    if (parts == NULL)
    {
        log_error("Received no parts! Halting.");
        return -1;
    }

    log_info("1st part: #%p, len %d\n", parts->buf, parts->len);

    Track* track = track__unpack(NULL, parts->len, parts->buf);
    if (track == NULL)
    {
        log_error("Failed to unpack Track!\n");
        return -1;
    }

    log_info("Track unpacked!\n");

    spotify_id trackid = spotify_id_from_raw(track->gid.data, SAT_Track);
    log_info("%s - %s\n", track->artist[0]->name, track->name);

    AudioFile* bestSupported = NULL;

    for (int i = 0; i < track->n_file; i++)
    {
        AudioFile* file = track->file[i];

        if (file->format < 3)
        {
            if (bestSupported == NULL)
                bestSupported = file;
            else if (bestSupported->format < file->format)
                bestSupported = file;
        }
    }

    char file_id[SPOTIFY_FILE_ID_B16_LENGTH] = "<no id>";
    spotify_file_id sfid = spotify_file_id_from_raw(bestSupported->file_id.data);
    spotify_file_id_to_b16(file_id, sfid);
    log_info("[Best] File %s: format = %d\n", file_id, bestSupported->format);

    if (audioplay_track_create(audioplay, trackid, sfid) != 0)
        return -1;

    return 0;
}

int spotifymgr_demo()
{
    log_info("Sending mercury GET\n");

    spotify_id track_id = spotify_id_from_b62("0GNNFgqWRqhQ8Cng9nEXpY", SAT_Track);
    char track_id_b16[SPOTIFY_ID_B16_LENGTH + 1];
    spotify_id_to_b16(track_id_b16, track_id);

    char* mercury_url = malloc(strlen("hm://metadata/3/track/") + SPOTIFY_ID_B16_LENGTH + 1);
    sprintf(mercury_url, "hm://metadata/3/track/%s", track_id_b16);

    if (mercury_get_request(mercury, mercury_url, demo_request_handler, NULL) < 0)
        return -1;

    return 0;
}
#pragma endregion Demo