/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_ui.h
 *
 * LVGL-based Launcher UI.
 ****************************************************************************/

#ifndef __LAUNCHER_UI_H
#define __LAUNCHER_UI_H

#include <lvgl/lvgl.h>
#include "launcher_app.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and show the launcher UI.
 *
 * @param apps       Array of discovered apps
 * @param app_count  Number of apps
 * @return LVGL screen object
 */
lv_obj_t *launcher_ui_create(launcher_app_t *apps, int app_count);

/**
 * @brief Navigate to a specific page.
 *
 * @param page  Page index (0-based)
 */
void launcher_ui_goto_page(int page);

/**
 * @brief Get current page index.
 */
int launcher_ui_get_current_page(void);

/**
 * @brief Refresh the UI (e.g., after time update).
 */
void launcher_ui_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* __LAUNCHER_UI_H */
