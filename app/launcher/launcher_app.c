/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_app.c
 *
 * App discovery and launching for the Launcher.
 * Scans builtin tasks and SD card app manifests.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <nuttx/config.h>
#include <nuttx/lib/builtin.h>
#include <spawn.h>
#include <sched.h>

#include "launcher_app.h"
#include "launcher_manifest.h"

/****************************************************************************
 * Private Helpers
 ****************************************************************************/

static launcher_app_t *g_app_list = NULL;
static int g_app_count = 0;

#ifdef CONFIG_BUILTIN
static int discover_builtin(launcher_app_t *apps, int max_apps, int offset)
{
    int i;
    int count = 0;

    for (i = 0; i < g_builtin_count && count < max_apps; i++) {
        /* Skip system tasks (optional filter) */
        const char *name = g_builtins[i].name;
        if (!name || name[0] == '\0') continue;

        /* Skip nsh if we're replacing it */
        /* if (strcmp(name, "nsh") == 0) continue; */

        launcher_app_t *app = &apps[offset + count];
        memset(app, 0, sizeof(*app));
        strncpy(app->name, name, MAX_APP_NAME_LEN - 1);
        strncpy(app->exec_path, name, MAX_EXEC_PATH_LEN - 1);
        app->type = APP_TYPE_BUILTIN;
        app->color = g_icon_colors[(offset + count) % COLOR_PALETTE_SIZE];

        count++;
    }

    return count;
}
#endif

static int discover_sdcard(launcher_app_t *apps, int max_apps, int offset)
{
    launcher_manifest_t manifests[MAX_APPS];
    int n, count = 0;

    n = launcher_manifest_scan(SDCARD_APPS_DIR, manifests, MAX_APPS);
    for (int i = 0; i < n && count < max_apps; i++) {
        launcher_app_t *app = &apps[offset + count];
        memset(app, 0, sizeof(*app));
        strncpy(app->name, manifests[i].name, MAX_APP_NAME_LEN - 1);
        strncpy(app->exec_path, manifests[i].exec_path, MAX_EXEC_PATH_LEN - 1);
        app->type = manifests[i].type;
        app->color = g_icon_colors[(offset + count) % COLOR_PALETTE_SIZE];
        if (manifests[i].icon_path[0] != '\0') {
            strncpy(app->icon_path, manifests[i].icon_path, MAX_EXEC_PATH_LEN - 1);
        }
        count++;
    }
    return count;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int launcher_app_discover(launcher_app_t *apps, int max_apps)
{
    int total = 0;

    if (!apps || max_apps <= 0) return 0;

    memset(apps, 0, max_apps * sizeof(launcher_app_t));

#ifdef CONFIG_BUILTIN
    total += discover_builtin(apps, max_apps - total, total);
#endif

    total += discover_sdcard(apps, max_apps - total, total);

    /* Assign pages and slots */
    for (int i = 0; i < total; i++) {
        apps[i].page = i / APPS_PER_PAGE;
        apps[i].slot = i % APPS_PER_PAGE;
    }

    g_app_list = apps;
    g_app_count = total;

    printf("Launcher: discovered %d apps total\n", total);
    return total;
}

int launcher_app_launch(const launcher_app_t *app)
{
    int ret;
    pid_t pid;
    char *argv[2];

    if (!app || app->exec_path[0] == '\0') return -1;

    printf("Launcher: launching '%s' (%s) type=%d\n",
           app->name, app->exec_path, app->type);

    argv[0] = (char *)app->exec_path;
    argv[1] = NULL;

    switch (app->type) {
    case APP_TYPE_BUILTIN:
    case APP_TYPE_ELF:
        /* Use posix_spawn - NuttX binfmt handles both builtin and ELF */
        ret = posix_spawn(&pid, app->exec_path, NULL, NULL, argv, NULL);
        if (ret != 0) {
            printf("Launcher: posix_spawn failed: %d (errno=%d)\n", ret, errno);
            return -1;
        }
        printf("Launcher: spawned pid=%d\n", (int)pid);
        return 0;

    case APP_TYPE_QUICKAPP:
        /* Future: use quickapp launch API */
        printf("Launcher: quickapp not supported yet\n");
        return -1;

    default:
        return -1;
    }
}

int launcher_app_page_count(int app_count)
{
    if (app_count <= 0) return 1;
    return (app_count + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
}
