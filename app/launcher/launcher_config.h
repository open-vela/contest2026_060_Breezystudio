/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_config.h
 *
 * Launcher configuration - screen, layout, and app settings.
 * Gemini-S1, 2.8-inch display (240x320).
 ****************************************************************************/

#ifndef __LAUNCHER_CONFIG_H
#define __LAUNCHER_CONFIG_H

#include <lvgl/lvgl.h>

/****************************************************************************
 * Display Configuration
 ****************************************************************************/

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

/****************************************************************************
 * Grid Layout
 ****************************************************************************/

#define GRID_COLS           3
#define GRID_ROWS           3
#define APPS_PER_PAGE       (GRID_COLS * GRID_ROWS)

#define ICON_SIZE           56
#define ICON_RADIUS         14
#define ICON_SPACING_X      12
#define ICON_SPACING_Y      12
#define ICON_LABEL_HEIGHT   28

/* Grid origin (centered horizontally, starts below status bar) */
#define STATUS_BAR_HEIGHT   32
#define GRID_TOP_MARGIN     (STATUS_BAR_HEIGHT + 12)
#define GRID_LEFT_MARGIN    ((DISPLAY_WIDTH - (GRID_COLS * (ICON_SIZE + ICON_SPACING_X) - ICON_SPACING_X)) / 2)

/****************************************************************************
 * Page Indicator
 ****************************************************************************/

#define PAGE_DOT_RADIUS     4
#define PAGE_DOT_SPACING    10
#define PAGE_DOT_Y          (DISPLAY_HEIGHT - 20)

/****************************************************************************
 * Status Bar
 ****************************************************************************/

#define STATUSBAR_BG_COLOR  0x1A1A2E
#define STATUSBAR_TEXT_COLOR 0xE0E0E0

/****************************************************************************
 * Color Palette
 ****************************************************************************/

/* App icon background colors - cycling palette */
#define COLOR_PALETTE_SIZE  12

/* Declared in launcher_ui.c */
extern const lv_color_t g_icon_colors[COLOR_PALETTE_SIZE];

/****************************************************************************
 * App Launcher Settings
 ****************************************************************************/

/* Directories to scan for apps */
#define SDCARD_APPS_DIR         "/sdcard/apps"
#define SDCARD_MANIFEST_FILE    "manifest.json"

/* Maximum number of apps */
#define MAX_APPS                32
#define MAX_MANIFEST_PATH       256
#define MAX_APP_NAME_LEN        32
#define MAX_EXEC_PATH_LEN       128

/* Time update interval (ms) */
#define CLOCK_UPDATE_MS         1000

/****************************************************************************
 * App Types
 ****************************************************************************/

typedef enum {
    APP_TYPE_BUILTIN = 0,   /* NuttX builtin task */
    APP_TYPE_ELF,           /* ELF executable from filesystem */
    APP_TYPE_QUICKAPP,      /* Vela QuickApp (future) */
    APP_TYPE_COUNT
} launcher_app_type_t;

#endif /* __LAUNCHER_CONFIG_H */
