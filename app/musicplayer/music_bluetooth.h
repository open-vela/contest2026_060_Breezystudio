/****************************************************************************
 * music_bluetooth.h - Bluetooth A2DP Sink integration
 ****************************************************************************/
#ifndef __MUSIC_BLUETOOTH_H
#define __MUSIC_BLUETOOTH_H
#include <stdbool.h>

bool music_bt_init(void);
void music_bt_deinit(void);
const char *music_bt_get_status(void);
void music_bt_poll(void);
bool music_bt_is_on(void);

#endif
