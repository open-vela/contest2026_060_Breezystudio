/****************************************************************************
 * contest2026_060_Breezystudio/app/musicplayer/config.h
 *
 * Music Player configuration for Gemini-S1 (320x240).
 ****************************************************************************/

#ifndef __MUSIC_CONFIG_H
#define __MUSIC_CONFIG_H

#include <lvgl/lvgl.h>

/* Display */
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define STATUSBAR_H     28
#define PLAYER_BAR_H    60  /* Bottom player bar height */

/* Audio */
#define SDCARD_PATH     "/sdcard"
#define DATA_PATH       "/data"          /* NAND flash storage */
#define AUDIO_DEVICE    "/dev/audio/pcm0p"
#define DEFAULT_VOLUME  80

/* Colors */
#define COLOR_BG        0x0D0D1A
#define COLOR_TITLE_BG  0x1A1A2E
#define COLOR_ACCENT    0xE91E63
#define COLOR_BTN1      0x3498DB
#define COLOR_BTN2      0x2ECC71
#define COLOR_BTN3      0xF39C12
#define COLOR_TEXT       0xE0E0E0
#define COLOR_DIR        0x3498DB
#define COLOR_FILE       0x2ECC71
#define COLOR_DIR        0x3498DB
#define COLOR_FILE       0x2ECC71

#endif
