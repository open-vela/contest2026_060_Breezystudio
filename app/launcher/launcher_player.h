/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_player.h
 *
 * Audio player module using nxplayer API.
 * Supports MP3, FLAC, WAV, AAC, OGG, OPUS via FFmpeg.
 ****************************************************************************/

#ifndef __LAUNCHER_PLAYER_H
#define __LAUNCHER_PLAYER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the audio player.
 * @return 0 on success, -1 on error.
 */
int launcher_player_init(void);

/**
 * @brief Play an audio file.
 * @param filepath  Full path to audio file.
 * @return 0 on success, -1 on error.
 */
int launcher_player_play(const char *filepath);

/**
 * @brief Stop current playback.
 */
void launcher_player_stop(void);

/**
 * @brief Pause/resume playback.
 */
void launcher_player_pause(void);

/**
 * @brief Set volume (0-100).
 * @param vol  Volume percentage.
 */
void launcher_player_set_volume(int vol);

/**
 * @brief Get current volume (0-100).
 */
int launcher_player_get_volume(void);

/**
 * @brief Check if currently playing.
 */
bool launcher_player_is_playing(void);

/**
 * @brief Cleanup player resources.
 */
void launcher_player_deinit(void);

/**
 * @brief Get the currently playing filename (or NULL).
 */
const char *launcher_player_get_current_file(void);

/**
 * @brief Check if SD card is inserted and mounted.
 */
bool launcher_sdcard_available(void);

/**
 * @brief Refresh SD card status (re-check).
 */
void launcher_sdcard_refresh(void);

/**
 * @brief Check if a filename has a supported audio extension.
 */
bool launcher_player_is_audio_file(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* __LAUNCHER_PLAYER_H */
