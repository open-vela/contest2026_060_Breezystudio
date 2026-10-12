/****************************************************************************
 * music_bluetooth.c - Bluetooth A2DP Sink
 *   Just checks if bluetoothd is running. No bttool calls from UI thread.
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "config.h"

static bool g_bt_on;
static char g_bt_status[64] = "Off";

bool music_bt_init(void)
{
    if (g_bt_on) return true;

    printf("[bt] Bluetooth A2DP Sink init\n");
    g_bt_on = true;

    /* bluetoothd should be started by rcS.nsh at boot.
     * Don't try to start it here - it crashes on this platform.
     * Just check if it's running and set scanmode. */
    if (system("pidof bluetoothd > /dev/null 2>&1") == 0) {
        printf("[bt] bluetoothd running, setting discoverable\n");
        snprintf(g_bt_status, sizeof(g_bt_status), "Discoverable");
        system("bt_lightCtrl scanmode 2 &");
    } else {
        printf("[bt] bluetoothd NOT running - BT unavailable\n");
        snprintf(g_bt_status, sizeof(g_bt_status), "Unavailable");
    }

    return true;
}

void music_bt_deinit(void)
{
    g_bt_on = false;
    snprintf(g_bt_status, sizeof(g_bt_status), "Off");
}

const char *music_bt_get_status(void)
{
    if (!g_bt_on) {
        snprintf(g_bt_status, sizeof(g_bt_status), "Off");
        return g_bt_status;
    }

    /* Check bluetoothd status without calling bttool (blocks UI) */
    if (system("pidof bluetoothd > /dev/null 2>&1") == 0) {
        /* bluetoothd is running - it handles discoverability automatically */
        snprintf(g_bt_status, sizeof(g_bt_status), "Discoverable");
    } else {
        snprintf(g_bt_status, sizeof(g_bt_status), "Waiting...");
    }
    return g_bt_status;
}

bool music_bt_is_on(void) { return g_bt_on; }
