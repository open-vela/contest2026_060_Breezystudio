/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_manifest.h
 *
 * JSON manifest parser for SD card app packages.
 * Format: { "name": "App", "exec": "path_or_task", "type": "builtin|elf", "icon": "optional_icon_path" }
 ****************************************************************************/

#ifndef __LAUNCHER_MANIFEST_H
#define __LAUNCHER_MANIFEST_H

#include "launcher_config.h"

/****************************************************************************
 * Types
 ****************************************************************************/

typedef struct {
    char name[MAX_APP_NAME_LEN];
    char exec_path[MAX_EXEC_PATH_LEN];
    launcher_app_type_t type;
    char icon_path[MAX_EXEC_PATH_LEN];  /* Empty = use default */
} launcher_manifest_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a manifest.json file into a manifest struct.
 *
 * @param filepath  Full path to manifest.json
 * @param base_dir  Base directory (for resolving relative paths)
 * @param manifest  Output manifest struct
 * @return 0 on success, -1 on error
 */
int launcher_manifest_parse(const char *filepath, const char *base_dir,
                            launcher_manifest_t *manifest);

/**
 * @brief Scan a directory for app subdirectories containing manifest.json.
 *
 * @param dir_path      Path to scan (e.g., /sdcard/apps/)
 * @param manifests     Output array of manifest structs
 * @param max_manifests Maximum number to scan
 * @return Number of apps found
 */
int launcher_manifest_scan(const char *dir_path,
                           launcher_manifest_t *manifests,
                           int max_manifests);

#ifdef __cplusplus
}
#endif

#endif /* __LAUNCHER_MANIFEST_H */
