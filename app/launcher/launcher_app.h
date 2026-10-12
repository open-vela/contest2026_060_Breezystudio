/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_app.h
 *
 * App discovery, management, and launching.
 ****************************************************************************/

#ifndef __LAUNCHER_APP_H
#define __LAUNCHER_APP_H

#include "launcher_config.h"

/****************************************************************************
 * Types
 ****************************************************************************/

typedef struct {
    char name[MAX_APP_NAME_LEN];
    char exec_path[MAX_EXEC_PATH_LEN];
    launcher_app_type_t type;
    char icon_path[MAX_EXEC_PATH_LEN];
    lv_color_t color;       /* Icon background color */
    uint8_t page;           /* Which page this app is on */
    uint8_t slot;           /* Position within page (0..APPS_PER_PAGE-1) */
} launcher_app_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Discover all available apps (builtin + SD card).
 *
 * @param apps       Output array of discovered apps
 * @param max_apps   Maximum number of apps to discover
 * @return Number of apps found
 */
int launcher_app_discover(launcher_app_t *apps, int max_apps);

/**
 * @brief Launch an application.
 *
 * For builtin apps: uses task_create or the NuttX exec_builtin mechanism.
 * For ELF apps: uses posix_spawn to execute the ELF from filesystem.
 *
 * @param app  The app to launch
 * @return 0 on success, -1 on error
 */
int launcher_app_launch(const launcher_app_t *app);

/**
 * @brief Get the total number of pages needed.
 *
 * @param app_count  Total number of apps
 * @return Number of pages
 */
int launcher_app_page_count(int app_count);

#ifdef __cplusplus
}
#endif

#endif /* __LAUNCHER_APP_H */
