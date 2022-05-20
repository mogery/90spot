#include "audioplay.h"

#include <string.h>
#include <malloc.h>
#include <switch/arm/cache.h>
#include "log.h"

static const AudioRendererConfig ar_config =
{
    .output_rate     = AudioRendererOutputRate_48kHz,
    .num_voices      = 24, // TODO: overkill
    .num_effects     = 0,
    .num_sinks       = 1,
    .num_mix_objs    = 1,
    .num_mix_buffers = 2,
};

/*
 * For some odd reason, if we initialize audren
 * from within audioplay_init (which doesn't
 * directly run in main()), it simply doesn't
 * work. So, this is an ugly bodge. Oh well!
 */
bool audioplay_svc_init()
{
    Result res;
    if (R_FAILED((res = audrenInitialize(&ar_config))))
    {
        log_error("[AUDIOPLAY] failed to initialize audren: %04X\n", res);
        return false;
    }

    return true;
}

audioplay_ctx* audioplay_init(audiofetch_ctx* audiofetch)
{
    audioplay_ctx* ctx = malloc(sizeof(audioplay_ctx));
    ctx->audiofetch = audiofetch;
    ctx->tracks = NULL;

    ctx->mempool_size = (AUDIOPLAY_WAVEBUF_DEF_RATE * AUDIOPLAY_WAVEBUF_DEF_LEN * AUDIOPLAY_WAVEBUF_COUNT * AUDIOPLAY_WAVEBUF_DEF_CHANNELS * sizeof(uint16_t) + 0xFFF) &~ 0xFFF;
    ctx->mempool_ptr = memalign(0x1000, ctx->mempool_size);
    memset(ctx->mempool_ptr, 0, ctx->mempool_size);

    // audren is already initialized by audioplay_svc_init
    Result res;

    if (R_FAILED((res = audrvCreate(&ctx->audrv, &ar_config, 2))))
    {
        log_error("[AUDIOPLAY] failed to create AudioDriver: %04X\n", res);
        return NULL;
    }

    ctx->mempool_id = audrvMemPoolAdd(&ctx->audrv, ctx->mempool_ptr, ctx->mempool_size);
    if (ctx->mempool_id < 0)
    {
        log_error("[AUDIOPLAY] failed to add mempool to AudioDriver\n");
        audrvClose(&ctx->audrv);
        audrenExit();
        return NULL;
    }

    if (!audrvMemPoolAttach(&ctx->audrv, ctx->mempool_id))
    {
        log_error("[AUDIOPLAY] failed to attach mempool to AudioDriver\n");
        audrvMemPoolRemove(&ctx->audrv, ctx->mempool_id);
        audrvClose(&ctx->audrv);
        audrenExit();
        return NULL;
    }

    static const uint8_t sink_channels[] = { 0, 1 };
    int sink = audrvDeviceSinkAdd(&ctx->audrv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);
    if (sink < 0)
    {
        log_error("[AUDIOPLAY] failed to add sink to AudioDriver\n");
        audrvMemPoolDetach(&ctx->audrv, ctx->mempool_id);
        audrvMemPoolRemove(&ctx->audrv, ctx->mempool_id);
        audrvClose(&ctx->audrv);
        audrenExit();
        return NULL;
    }

    if (R_FAILED((res = audrvUpdate(&ctx->audrv))))
    {
        log_error("[AUDIOPLAY] failed to update AudioDriver: %04X\n", res);
        audrvMemPoolDetach(&ctx->audrv, ctx->mempool_id);
        audrvMemPoolRemove(&ctx->audrv, ctx->mempool_id);
        audrvClose(&ctx->audrv);
        audrenExit();
        return NULL;
    }

    if (R_FAILED((res = audrenStartAudioRenderer())))
    {
        log_error("[AUDIOPLAY] failed to start AudioRenderer: %04X\n", res);
        audrvMemPoolDetach(&ctx->audrv, ctx->mempool_id);
        audrvMemPoolRemove(&ctx->audrv, ctx->mempool_id);
        audrvClose(&ctx->audrv);
        audrenExit();
        return NULL;
    }

    AudioDriverWaveBuf wavebuf = {0};
    for (int i = 0; i < AUDIOPLAY_WAVEBUF_COUNT; i++) {
        ctx->wavebufs[i] = wavebuf;
        ctx->wavebufs[i].data_raw = ctx->mempool_ptr;
        ctx->wavebufs[i].size = AUDIOPLAY_WAVEBUF_COUNT * AUDIOPLAY_WAVEBUF_DEF_LEN * AUDIOPLAY_WAVEBUF_DEF_RATE * AUDIOPLAY_WAVEBUF_DEF_CHANNELS * sizeof(uint16_t);
        ctx->wavebufs[i].start_sample_offset = i * (AUDIOPLAY_WAVEBUF_DEF_LEN * AUDIOPLAY_WAVEBUF_DEF_RATE);
        ctx->wavebufs[i].end_sample_offset = ctx->wavebufs[i].start_sample_offset;
    }

    return ctx;
}

int audioplay_audiofetch_header_handler(audiofetch_ctx* audiofetch, audiofetch_request* req, vorbis_info info, void* _track)
{
    audioplay_track* track = _track;
    track->has_info = true;
    track->info = info;

    log_info("[AUDIOPLAY] Received headers! ch: %d, sr: %ld\n", info.channels, info.rate);   
    return 0;
}

int audioplay_audiofetch_frame_handler(audiofetch_ctx* audiofetch, audiofetch_request* req, float** samples, int count, int channels, bool is_eos, void* _track)
{
    if (count == 0) // if PCM frame is empty, don't bother
        return count;

    audioplay_track* track = _track;

    audioplay_sampile* pile = malloc(sizeof(audioplay_sampile));
    if (pile == NULL)
    {
        log_error("[AUDIOPLAY] Failed to allocate sampile struct!\n");
        return -1;
    }

    pile->buf = malloc(count * channels * sizeof(uint16_t));
    if (pile->buf == NULL)
    {
        log_error("[AUDIOPLAY] Failed to allocate sampile buffer!\n");
        return -1;
    }

    pile->samples = count * channels;
    pile->next = NULL;

    for (int i = 0; i < count; i++)
    {
        for (int c = 0; c < channels; c++)
        {
            float f = samples[c][i];
            f = f * 32768;
            if(f > 32767) f = 32767;
            if(f < -32768) f = -32768;
            pile->buf[i * channels + c] = (int16_t)f;
        }
    }

    if (track->sampiles == NULL)
        track->sampiles = pile;
    else
    {
        audioplay_sampile* last_pile = track->sampiles;
        while (last_pile->next != NULL)
            last_pile = last_pile->next;
        
        last_pile->next = pile;
    }

    if (is_eos)
        track->is_ending = true;

    return count;
}

int audioplay_audiofetch_end_handler(audiofetch_ctx* audiofetch, audiofetch_request* req, void* _track)
{
    audioplay_track* track = _track;

    log_info("[AUDIOPLAY] Request ended.\n");
    track->is_busy = false;

    return 0;
}

int audioplay_track_create(audioplay_ctx* ctx, spotify_id track_id, spotify_file_id file_id)
{
    audioplay_track* track = malloc(sizeof(audioplay_track));
    track->ctx = ctx;
    track->track_id = track_id;
    track->file_id = file_id;
    track->has_info = false;
    track->is_busy = true;
    track->is_ending = false;
    track->sampiles = NULL;

    audiofetch_request* req = audiofetch_create(ctx->audiofetch, track_id, file_id, audioplay_audiofetch_end_handler, audioplay_audiofetch_header_handler, audioplay_audiofetch_frame_handler, track);
    if (req == NULL)
    {
        log_error("[AUDIOPLAY] failed to create audiofetch request :(\n");
        free(track);
        return -1;
    }
    track->req = req;

    if (ctx->tracks == NULL)
        ctx->tracks = track;
    else
    {
        audioplay_track* last_track = ctx->tracks;
        while (last_track->next != NULL)
            last_track = last_track->next;
        
        last_track->next = track;
    }

    return 0;
}

int audioplay_update(audioplay_ctx* ctx)
{
    audioplay_track* track = ctx->tracks;
    if (track == NULL)
        return 0;

    if (!track->is_playing)
    {
        if (!track->has_info)
            return 0;

        if (!audrvVoiceInit(&ctx->audrv, 0, track->info.channels, PcmFormat_Int16, track->info.rate))
        {
            log_error("[AUDIOPLAY] failed to initialize voice\n");
            return -1;
        }

        audrvVoiceSetDestinationMix(&ctx->audrv, 0, AUDREN_FINAL_MIX_ID);

        if (track->info.channels == 1)
        {
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 1.0f, 0, 0);
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 1.0f, 0, 1);
        }
        else
        {
            if (track->info.channels > 2)
                log_warn("[AUDIOPLAY] Track has more than 2 channels, ignoring the rest.\n");
            
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 1.0f, 0, 0);
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 0.0f, 0, 1);
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 0.0f, 1, 0);
            audrvVoiceSetMixFactor(&ctx->audrv, 0, 1.0f, 1, 1);
        }

        audrvVoiceStart(&ctx->audrv, 0);

        track->is_playing = true;        
    }
    else
    {
        int curwb = -1;
        bool all_wavebufs_free = true;

        for (int i = 0; i < AUDIOPLAY_WAVEBUF_COUNT; i++)
        {
            if (ctx->wavebufs[i].state == AudioDriverWaveBufState_Free || ctx->wavebufs[i].state == AudioDriverWaveBufState_Done)
            {
                if (curwb == -1)
                    curwb = i;
            }
            else
            {
                all_wavebufs_free = false;
            }
        }

        if (track->is_ending && all_wavebufs_free)
        {
            audrvVoiceStop(&ctx->audrv, 0);
            goto skip;
        }
        
        if (curwb == -1)
        {
            if (!audrvVoiceIsPlaying(&ctx->audrv, 0))
            {
                audrvVoiceStart(&ctx->audrv, 0);
                log_warn("[AUDIOPLAY] Voice wasn't playing! Restarted.\n");
            }
        }
        else if (!track->is_busy)
        {
            uint16_t* curbuf = (ctx->mempool_ptr + ctx->wavebufs[curwb].start_sample_offset * sizeof(uint16_t));
            int max_sample_count = AUDIOPLAY_WAVEBUF_DEF_LEN * AUDIOPLAY_WAVEBUF_DEF_RATE * AUDIOPLAY_WAVEBUF_DEF_CHANNELS;
            size_t curbuf_max_len = max_sample_count * sizeof(uint16_t);
            
            int sample_cnt = 0;
            audioplay_sampile* pile = track->sampiles;
            int piles_consumed = 0;

            if (pile == NULL)
                goto skip_but_check;
            
            memset(curbuf, 0, curbuf_max_len);
            
            while (pile != NULL && sample_cnt < max_sample_count)
            {
                audioplay_sampile* next = pile->next;

                if (pile->buf == NULL)
                {
                    pile = next;
                    continue;
                }

                int remaining_samples = max_sample_count - sample_cnt;
                int committing_samples = pile->samples;
                if (pile->samples > remaining_samples)
                    committing_samples = remaining_samples;
                
                for (int i = 0; i < committing_samples; i++)
                    curbuf[sample_cnt + i] = pile->buf[i];

                if (committing_samples < pile->samples) // if we weren't able to utilize the entirity of the pile
                {
                    // TODO: figure out why this is kinda busted (crunchy audio)
                    // copy remaining samples to the beginning of the pile buffer
                    memcpy(pile->buf, (pile->buf + (committing_samples * sizeof(uint16_t))), (pile->samples - committing_samples) * sizeof(uint16_t));
                    // remove used sample count
                    pile->samples -= committing_samples;
                }
                else
                {
                    free(pile->buf);
                    free(pile);
                    track->sampiles = next;
                }

                sample_cnt += committing_samples;
                pile = next;
                piles_consumed++;
            }

            log_info("[AUDIOPLAY] Consumed %d piles!\n", piles_consumed);

            ctx->wavebufs[curwb].end_sample_offset = ctx->wavebufs[curwb].start_sample_offset + (sample_cnt / AUDIOPLAY_WAVEBUF_DEF_CHANNELS);
            armDCacheFlush(curbuf, sample_cnt * sizeof(uint16_t));

            audrvVoiceAddWaveBuf(&ctx->audrv, 0, &ctx->wavebufs[curwb]);
        }

skip_but_check:
        if (!track->is_busy && !track->is_ending)
        {
            int total_samples = 0;
            audioplay_sampile* pile = track->sampiles;
            while (pile != NULL)
            {
                total_samples += pile->samples;
                pile = pile->next;
            }

            if (total_samples < AUDIOPLAY_LOW_SAMPLE_THRESHOLD)
            {
                log_warn("[AUDIOPLAY] Sampile too small, requesting more... (%d)\n", total_samples);
                track->is_busy = true;
                audiofetch_fetch(track->req, 1024 * 1024 / FETCH_BLOCK_SIZE);
            }
        }
    }

skip:
    Result res;
    if (R_FAILED((res = audrvUpdate(&ctx->audrv))))
    {
        log_error("[AUDIOPLAY] failed to update AudioDriver: %04X\n", res);
        return -1;
    }

    return 0;
}

void audioplay_destroy(audioplay_ctx* ctx)
{
    audrvMemPoolDetach(&ctx->audrv, ctx->mempool_id);
    audrvMemPoolRemove(&ctx->audrv, ctx->mempool_id);
    audrvClose(&ctx->audrv);
    free(ctx);
}

void audioplay_svc_cleanup()
{
    audrenExit();
}