/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_player.c
 *
 * Audio player using nxplayer API.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <nuttx/config.h>
#include <nuttx/audio/audio.h>
#include <system/nxplayer.h>

#include "launcher_player.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxplayer_s *g_player = NULL;
static bool g_playing = false;
static int g_volume = 80;  /* Default 80% */
static char g_current_file[256] = "";
static bool g_sdcard_checked = false;
static bool g_sdcard_present = false;

/* Supported audio extensions */
static const char *g_audio_exts[] = {
    "mp3", "flac", "wav", "aac", "ogg",
    "opus", "wma", "m4a", "ape", "wv",
    "aiff", "aif", "alac", "dsf", "dff",
    NULL
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool launcher_player_is_audio_file(const char *filename)
{
    const char *ext;
    int i;

    if (!filename) return false;

    ext = strrchr(filename, '.');
    if (!ext) return false;
    ext++; /* Skip the dot */

    for (i = 0; g_audio_exts[i] != NULL; i++) {
        if (strcasecmp(ext, g_audio_exts[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool launcher_sdcard_available(void)
{
    struct stat st;

    if (!g_sdcard_checked) {
        if (stat("/sdcard", &st) == 0 && S_ISDIR(st.st_mode)) {
            g_sdcard_present = true;
        } else {
            g_sdcard_present = false;
        }
        g_sdcard_checked = true;
    }
    return g_sdcard_present;
}

void launcher_sdcard_refresh(void)
{
    g_sdcard_checked = false;
    launcher_sdcard_available();
}

int launcher_player_init(void)
{
    if (g_player) {
        return 0; /* Already initialized */
    }

    g_player = nxplayer_create();
    if (!g_player) {
        printf("Player: failed to create nxplayer\n");
        return -1;
    }

#ifdef CONFIG_AUDIOUTILS_NXAUDIO_DEVPATH
    nxplayer_setdevice(g_player, CONFIG_AUDIOUTILS_NXAUDIO_DEVPATH);
#else
    nxplayer_setdevice(g_player, "/dev/audio/pcm0p");
#endif

#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
    nxplayer_setvolume(g_player, g_volume * 10); /* nxplayer uses 0-1000 */
#endif
    printf("Player: initialized (device=%s, volume=%d%%)\n",
           CONFIG_AUDIOUTILS_NXAUDIO_DEVPATH, g_volume);
    return 0;
}

int launcher_player_play(const char *filepath)
{
    int ret;

    if (!g_player) {
        if (launcher_player_init() != 0) return -1;
    }

    /* Stop any current playback */
    if (g_playing) {
        nxplayer_stop(g_player);
        g_playing = false;
    }

    if (!filepath) return -1;

    printf("Player: playing '%s'\n", filepath);

    /* Play the file - nxplayer auto-detects format */
    ret = nxplayer_playfile(g_player, filepath, AUDIO_FMT_UNDEF, AUDIO_FMT_UNDEF);
    if (ret != 0) {
        printf("Player: playfile failed: %d (errno=%d)\n", ret, errno);
        return -1;
    }

    g_playing = true;
    strncpy(g_current_file, filepath, sizeof(g_current_file) - 1);
    return 0;
}

void launcher_player_stop(void)
{
    if (g_player && g_playing) {
        nxplayer_stop(g_player);
        g_playing = false;
        g_current_file[0] = '\0';
        printf("Player: stopped\n");
    }
}

void launcher_player_pause(void)
{
    if (!g_player) return;

    if (g_playing) {
        nxplayer_pause(g_player);
        g_playing = false;
        printf("Player: paused\n");
    } else {
        nxplayer_resume(g_player);
        g_playing = true;
        printf("Player: resumed\n");
    }
}

void launcher_player_set_volume(int vol)
{
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    g_volume = vol;

    if (g_player) {
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
        nxplayer_setvolume(g_player, vol * 10);
#endif
        printf("Player: volume = %d%%\n", vol);
    }
}

int launcher_player_get_volume(void)
{
    return g_volume;
}

bool launcher_player_is_playing(void)
{
    return g_playing;
}

const char *launcher_player_get_current_file(void)
{
    return g_current_file[0] ? g_current_file : NULL;
}

void launcher_player_deinit(void)
{
    if (g_player) {
        launcher_player_stop();
        nxplayer_release(g_player);
        g_player = NULL;
        printf("Player: deinitialized\n");
    }
}
