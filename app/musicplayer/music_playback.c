/****************************************************************************
 * music_playback.c - Audio playback via nxplayer API.
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <nuttx/config.h>
#include <nuttx/audio/audio.h>
#include <system/nxplayer.h>
#include "config.h"

static struct nxplayer_s *g_player;
static bool g_playing;
static char g_current[256];
static time_t g_play_start;       /* When current playback started */
static uint32_t g_total_sec;      /* Total duration in seconds (0=unknown) */

/* Audio extensions: full + 8.3 short forms */
static const char *g_exts[] = {
    "mp3","mp2","mp1",
    "wav","wave",
    "flac","fla",
    "aac","m4a","mp4",
    "ogg","oga",
    "opus","opu",
    "wma","asf",
    "aif","aiff","aifc",
    "mid","midi",
    "ape","wv",
    NULL
};

bool music_is_audio_file(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return false;
    ext++;
    for (int i = 0; g_exts[i]; i++) {
        if (strcasecmp(ext, g_exts[i]) == 0) {
            printf("[audio] match: %s -> %s\n", name, g_exts[i]);
            return true;
        }
    }
    return false;
}

bool music_sdcard_available(void)
{
    struct stat st;
    int ret = stat(SDCARD_PATH, &st);
    printf("[audio] sdcard check: %s -> %d (isdir=%d)\n", SDCARD_PATH, ret,
           (ret==0)?S_ISDIR(st.st_mode):0);
    return (ret == 0 && S_ISDIR(st.st_mode));
}

/* Check if a string is pure ASCII (no CJK/multibyte) */
/* Estimate WAV duration from file size.
 * Standard WAV: 44-byte header + PCM data. */
static uint32_t wav_duration(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (st.st_size <= 44) return 0;

    /* Try to read the format chunk for accurate byte_rate */
    FILE *f = fopen(path, "rb");
    if (f) {
        uint8_t hdr[44];
        if (fread(hdr, 1, 44, f) >= 44 &&
            memcmp(hdr, "RIFF", 4) == 0 &&
            memcmp(hdr + 8, "WAVE", 4) == 0) {
            uint32_t byte_rate = *(uint32_t *)(hdr + 28);
            if (byte_rate > 0) {
                fclose(f);
                return (st.st_size - 44) / byte_rate;
            }
        }
        fclose(f);
    }

    /* Fallback: assume 48000 Hz stereo 16-bit = 192000 bytes/sec */
    return (st.st_size - 44) / 192000;
}

static bool is_ascii(const char *s)
{
    while (*s) { if ((unsigned char)*s > 127) return false; s++; }
    return true;
}

bool music_playback_play(const char *path)
{
    printf("[audio] play: %s (ascii=%d)\n", path, is_ascii(path));

    /* CRITICAL: Do NOT call nxplayer_stop() on the old player.
     * If the old playback thread is stuck in a blocking DMA write
     * (due to audio hardware DMA timeout), nxplayer_stop() will
     * block trying to join the stuck thread, freezing the UI.
     *
     * Instead, we abandon the old player (its thread will eventually
     * exit when DMA errors accumulate) and create a fresh one.
     */
    if (g_player) {
        /* Just release reference, don't join thread */
        printf("[audio] abandoning old player %p\n", (void*)g_player);
        g_player = NULL;
    }
    g_playing = false;
    g_total_sec = 0;

    g_player = nxplayer_create();
    if (!g_player) { printf("[audio] create FAILED\n"); return false; }
    nxplayer_setdevice(g_player, AUDIO_DEVICE);

    int ret;

    if (is_ascii(path)) {
        /* ASCII path: nxplayer can open directly */
        ret = nxplayer_playfile(g_player, path, AUDIO_FMT_UNDEF, AUDIO_FMT_UNDEF);
        printf("[audio] ascii direct: %d (errno=%d)\n", ret, errno);
    } else {
        /* CJK path: nxplayer can't open CJK filenames directly via open().
         * Workaround: set mediadir to the parent directory, then call
         * nxplayer_playfile() with just the basename. nxplayer will fail
         * to open the bare basename, then retry as mediadir/basename,
         * which succeeds because the directory component is ASCII-safe
         * when constructed internally.
         */
        char dir[256], fname[256];
        strncpy(dir, path, 255); dir[255] = '\0';
        char *slash = strrchr(dir, '/');
        if (slash) {
            strncpy(fname, slash + 1, 255); fname[255] = '\0';
            *slash = '\0';
        } else {
            strncpy(fname, path, 255); fname[255] = '\0';
            strcpy(dir, ".");
        }

        /* Save and override mediadir to parent directory */
        char saved_mediadir[256];
        strncpy(saved_mediadir,
                g_player->mediadir[0] ? g_player->mediadir : "/music",
                255);
        saved_mediadir[255] = '\0';
        nxplayer_setmediadir(g_player, dir);

        ret = nxplayer_playfile(g_player, fname, AUDIO_FMT_UNDEF, AUDIO_FMT_UNDEF);
        printf("[audio] cjk via mediadir '%s': %d (errno=%d)\n", dir, ret, errno);

        /* Restore original mediadir */
        nxplayer_setmediadir(g_player, saved_mediadir);
    }

    if (ret != 0) { printf("[audio] FAILED: %d\n", ret); return false; }
    g_playing = true;
    g_play_start = time(NULL);
    strncpy(g_current, path, 255);

    /* Try to determine total duration */
    g_total_sec = 0;
    const char *ext = strrchr(path, '.');
    if (ext && strcasecmp(ext, ".wav") == 0) {
        g_total_sec = wav_duration(path);
    }
    printf("[audio] playing: %s (total=%us)\n", path, g_total_sec);
    return true;
}

void music_playback_stop(void)
{
    /* Abandon the player without joining (join may block on stuck DMA).
     * The old playback thread will eventually exit on its own. */
    if (g_player) {
        printf("[audio] abandon on stop: %p\n", (void*)g_player);
        g_player = NULL;
    }
    g_playing = false;
    g_current[0] = 0;
    g_total_sec = 0;
    g_play_start = 0;
}

void music_playback_pause(void)
{
    if (!g_player) return;
    if (g_playing) {
        nxplayer_pause(g_player);
        g_playing = false;
    } else {
        nxplayer_resume(g_player);
        g_playing = true;
        if (g_play_start == 0) g_play_start = time(NULL);
    }
}

bool music_playback_is_playing(void)
{
    if (!g_player || !g_playing) return false;
    /* Also check actual nxplayer state: if thread finished, state goes to IDLE */
    if (g_player->state == 0) { /* NXPLAYER_STATE_IDLE */
        g_playing = false;
        g_current[0] = 0;
        return false;
    }
    return true;
}

/* Must be called periodically to detect playback completion */
void music_playback_update(void)
{
    if (!g_player || !g_playing) return;
    if (g_player->state == 0) { /* NXPLAYER_STATE_IDLE */
        printf("[audio] playback ended naturally\n");
        g_playing = false;
        g_current[0] = 0;
        g_total_sec = 0;
        g_play_start = 0;
    }
}
const char *music_playback_current(void) { return g_current[0] ? g_current : NULL; }

void music_playback_deinit(void)
{
    music_playback_stop();
    /* Note: we leak the last nxplayer instance. Acceptable for now
     * since the DMA timeout prevents clean shutdown anyway. */
}

uint32_t music_playback_elapsed(void)
{
    if (!g_playing || g_play_start == 0) return 0;
    time_t now = time(NULL);
    if (now < g_play_start) return 0;
    return (uint32_t)(now - g_play_start);
}

uint32_t music_playback_total(void)
{
    return g_total_sec;
}

/* Generate a test tone at given frequency for given duration (seconds).
 * This bypasses SD card entirely - useful for testing audio hardware. */
bool music_playback_test_tone(uint32_t freq, uint32_t duration_sec)
{
    printf("[audio] test tone: %luHz for %lus\n", freq, duration_sec);

    /* Abandon old player to avoid blocking */
    if (g_player) {
        printf("[audio] abandoning old player for tone\n");
        g_player = NULL;
    }
    g_playing = false;
    g_total_sec = 0;

    g_player = nxplayer_create();
    if (!g_player) { printf("[audio] create FAILED\n"); return false; }
    nxplayer_setdevice(g_player, AUDIO_DEVICE);

    int ret = nxplayer_playtone(g_player, 48000, freq, duration_sec);
    printf("[audio] tone result: %d (errno=%d)\n", ret, errno);

    if (ret != 0) return false;
    g_playing = true;
    g_play_start = time(NULL);
    g_total_sec = duration_sec;
    g_current[0] = 0;
    return true;
}
