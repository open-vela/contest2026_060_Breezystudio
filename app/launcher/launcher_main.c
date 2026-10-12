/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_main.c
 *
 * Vela Launcher - Main Entry Point.
 * A beautiful LVGL-based application launcher for Gemini-S1.
 *
 * Features:
 *   - Grid-based app icon launcher
 *   - Builtin app discovery (NuttX tasks)
 *   - SD card app support (manifest.json based)
 *   - Multi-page navigation
 *   - Status bar with clock
 *   - Touch and button navigation
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/boardctl.h>

#include <lvgl/lvgl.h>
#include "launcher_app.h"
#include "launcher_ui.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NEED_BOARDINIT

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define NEED_BOARDINIT 1
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static launcher_app_t g_launcher_apps[MAX_APPS];
static int g_launcher_app_count = 0;
static lv_timer_t *g_clock_timer = NULL;

/****************************************************************************
 * Timer Callbacks
 ****************************************************************************/

static void clock_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    launcher_ui_refresh();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;
    lv_obj_t *screen;

    (void)argc;
    (void)argv;

    /* Check if LVGL already initialized */
    if (lv_is_initialized()) {
        printf("Launcher: LVGL already initialized, aborting.\n");
        return -1;
    }

#ifdef NEED_BOARDINIT
    /* Perform board-specific driver initialization */
    boardctl(BOARDIOC_INIT, 0);
#endif

    /* Initialize LVGL */
    lv_init();

    /* Configure NuttX LVGL display driver */
    lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
    info.fb_path = "/dev/lcd0";
#endif

#ifdef CONFIG_INPUT_TOUCHSCREEN
    info.input_path = CONFIG_CONTEST_LAUNCHER_INPUT_DEVPATH;
#endif

    lv_nuttx_init(&info, &result);

    if (result.disp == NULL) {
        printf("Launcher: display initialization failure!\n");
        return 1;
    }

    printf("Launcher: LVGL initialized successfully.\n");

    /* Discover apps */
    g_launcher_app_count = launcher_app_discover(g_launcher_apps, MAX_APPS);
    printf("Launcher: discovered %d applications.\n", g_launcher_app_count);

    /* Create UI */
    screen = launcher_ui_create(g_launcher_apps, g_launcher_app_count);
    if (!screen) {
        printf("Launcher: UI creation failed!\n");
        lv_nuttx_deinit(&result);
        lv_deinit();
        return 1;
    }

    /* Start clock update timer */
    g_clock_timer = lv_timer_create(clock_timer_cb, CLOCK_UPDATE_MS, NULL);

    /* Main event loop */
    printf("Launcher: entering main loop.\n");

    while (1) {
        uint32_t idle;

        /* Handle LVGL tasks */
        idle = lv_timer_handler();

        /* Minimum sleep of 1ms to prevent busy-waiting */
        idle = idle ? idle : 1;
        usleep(idle * 1000);
    }

    /* Cleanup (unreachable) */
    if (g_clock_timer) {
        lv_timer_delete(g_clock_timer);
    }
    lv_nuttx_deinit(&result);
    lv_deinit();

    return 0;
}
