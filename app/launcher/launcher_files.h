/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_files.h
 *
 * LVGL file browser for SD card audio files.
 ****************************************************************************/

#ifndef __LAUNCHER_FILES_H
#define __LAUNCHER_FILES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and show the file browser screen.
 *        If SD card is not available, shows "TF未插入" message.
 * @param parent  Parent screen to return to on exit.
 */
void launcher_files_show(lv_obj_t *parent);

/**
 * @brief Hide/cleanup the file browser.
 */
void launcher_files_hide(void);

#ifdef __cplusplus
}
#endif

#endif /* __LAUNCHER_FILES_H */
