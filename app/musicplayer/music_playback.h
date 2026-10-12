/****************************************************************************
 * music_playback.h - Audio playback via nxplayer
 ****************************************************************************/
#ifndef __MUSIC_PLAYBACK_H
#define __MUSIC_PLAYBACK_H
#include <stdbool.h>

bool music_playback_play(const char *filepath);
void music_playback_stop(void);
void music_playback_pause(void);
bool music_playback_is_playing(void);
void music_playback_update(void);
const char *music_playback_current(void);
void music_playback_deinit(void);
bool music_sdcard_available(void);
bool music_is_audio_file(const char *name);
uint32_t music_playback_elapsed(void);
uint32_t music_playback_total(void);
bool music_playback_test_tone(uint32_t freq, uint32_t duration_sec);
#endif
