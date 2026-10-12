/****************************************************************************
 * music_player.c - Music Player for Gemini-S1 (320x240 LVGL)
 *
 * Features:
 *   - SD file browser (CJK names via d_type, not stat())
 *   - Now-Playing detail (player bar hidden, controls at bottom)
 *   - Bluetooth A2DP Sink status screen
 *   - Settings screen with volume slider
 *   - Persistent player bar at bottom
 *   - Selection highlight on currently playing file
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/boardctl.h>
#include <lvgl/lvgl.h>
#if LV_USE_FREETYPE
extern lv_result_t lv_freetype_init(uint32_t max_glyph_cnt);
extern lv_font_t *lv_freetype_font_create(const char *pathname,
    lv_freetype_font_render_mode_t render_mode, uint32_t size,
    lv_freetype_font_style_t style);
#endif
#include "config.h"
#include "music_playback.h"
#include "music_bluetooth.h"

/* ======================== CJK Font ======================== */
static lv_font_t *g_cjk_font = NULL;
extern const lv_font_t lv_font_simsun_16_cjk;

static void init_cjk_font(void)
{
#if LV_USE_FREETYPE
    lv_freetype_init(512);
    const char *fonts[] = {
        "/resource/fonts/MiSans-Normal.ttf",
        "/resource/fonts/Mplus1p-Regular.ttf",
        SDCARD_PATH "/font.ttf",
        NULL
    };
    for (int i = 0; fonts[i]; i++) {
        g_cjk_font = lv_freetype_font_create(fonts[i],
                        LV_FREETYPE_FONT_RENDER_MODE_BITMAP, 16,
                        LV_FREETYPE_FONT_STYLE_NORMAL);
        if (g_cjk_font) { printf("[font] loaded: %s\n", fonts[i]); return; }
        printf("[font] not found: %s\n", fonts[i]);
    }
    printf("[font] using simsun fallback\n");
#endif
    g_cjk_font = (lv_font_t *)&lv_font_simsun_16_cjk;
}

/* ======================== Globals ======================== */
static lv_obj_t *g_scr, *g_title, *g_title_label, *g_back_btn;
static lv_obj_t *g_list, *g_msg, *g_main_cont;
static lv_obj_t *g_player_bar, *g_bar_song_label, *g_bar_play_btn, *g_bar_time_label;
static lv_obj_t *g_now_playing_cont, *g_detail_song_label;
static lv_obj_t *g_detail_play_btn, *g_detail_stop_btn;
static lv_obj_t *g_bt_cont, *g_bt_status_label, *g_bt_name_label;
static lv_obj_t *g_settings_cont, *g_vol_slider, *g_vol_label;
static lv_obj_t *g_pad_cont;
static lv_timer_t *g_bt_timer;
static lv_timer_t *g_time_timer;

static char   g_path[256]        = "/sdcard";
static char   g_playing_path[256]= "";
static char   g_pending_path[256]= "";   /* Deferred play request */
static int    g_selected_idx     = -1;
static bool   g_browsing, g_showing_now_playing;
static bool   g_showing_bt, g_showing_settings, g_showing_pad;

#define CONTENT_H  (DISPLAY_HEIGHT - STATUSBAR_H - PLAYER_BAR_H)

/* ======================== Styles ======================== */
static lv_style_t sty_title, sty_btn, sty_dir, sty_file, sty_msg;
static lv_style_t sty_selected, sty_bar, sty_nowplaying, sty_bt_status, sty_slider;

static void init_styles(void)
{
    lv_style_init(&sty_title);
    lv_style_set_bg_color(&sty_title, lv_color_hex(COLOR_TITLE_BG));
    lv_style_set_bg_opa(&sty_title, LV_OPA_COVER);
    lv_style_set_pad_all(&sty_title, 4);
    lv_style_set_border_width(&sty_title, 0);

    lv_style_init(&sty_btn);
    lv_style_set_radius(&sty_btn, 12);
    lv_style_set_shadow_width(&sty_btn, 4);
    lv_style_set_shadow_opa(&sty_btn, LV_OPA_30);

    lv_style_init(&sty_dir);
    lv_style_set_text_color(&sty_dir, lv_color_hex(COLOR_DIR));
    lv_style_set_text_font(&sty_dir, g_cjk_font);

    lv_style_init(&sty_file);
    lv_style_set_text_color(&sty_file, lv_color_hex(COLOR_FILE));
    lv_style_set_text_font(&sty_file, g_cjk_font);

    lv_style_init(&sty_msg);
    lv_style_set_text_color(&sty_msg, lv_color_hex(0x777777));
    lv_style_set_text_font(&sty_msg, &lv_font_montserrat_18);

    lv_style_init(&sty_selected);
    lv_style_set_bg_color(&sty_selected, lv_color_hex(0x333366));
    lv_style_set_bg_opa(&sty_selected, LV_OPA_50);
    lv_style_set_border_width(&sty_selected, 0);

    lv_style_init(&sty_bar);
    lv_style_set_bg_color(&sty_bar, lv_color_hex(0x1A1A2E));
    lv_style_set_bg_opa(&sty_bar, LV_OPA_COVER);
    lv_style_set_pad_all(&sty_bar, 4);
    lv_style_set_border_width(&sty_bar, 1);
    lv_style_set_border_color(&sty_bar, lv_color_hex(0x333355));

    lv_style_init(&sty_nowplaying);
    lv_style_set_bg_color(&sty_nowplaying, lv_color_hex(COLOR_BG));
    lv_style_set_bg_opa(&sty_nowplaying, LV_OPA_COVER);
    lv_style_set_pad_all(&sty_nowplaying, 8);
    lv_style_set_border_width(&sty_nowplaying, 0);

    lv_style_init(&sty_bt_status);
    lv_style_set_text_color(&sty_bt_status, lv_color_hex(0xAAAAAA));
    lv_style_set_text_font(&sty_bt_status, g_cjk_font);

    lv_style_init(&sty_slider);
    lv_style_set_bg_color(&sty_slider, lv_color_hex(COLOR_ACCENT));
    lv_style_set_bg_opa(&sty_slider, LV_OPA_COVER);
}

/* ======================== Forward Declarations ======================== */
static void show_main(void);
static void show_browser(void);
static void show_now_playing(void);
static void show_music_pad(void);
static void show_bluetooth(void);
static void show_settings(void);
static void refresh_list(void);
static void update_player_bar(void);
static void update_detail_controls(void);
static void do_back(void);
static void on_back(lv_event_t *e)        { (void)e; do_back(); }
static void on_now_playing(lv_event_t *e)
{
    (void)e;
    show_music_pad();
}
static void on_play_nand(lv_event_t *e)
{
    (void)e;
    strcpy(g_path, DATA_PATH);
    show_browser();
}

static void on_play_sd(lv_event_t *e)     { (void)e; show_browser(); }
/* BT disabled until bluetoothd crash is fixed
static void on_play_bt(lv_event_t *e)     { (void)e; show_bluetooth(); }
*/
static void on_settings_btn(lv_event_t *e){ (void)e; show_settings(); }
static void on_item(lv_event_t *e);
static void on_bar_click(lv_event_t *e);
static void on_bar_play_pause(lv_event_t *e);
static void on_detail_play_pause(lv_event_t *e);
static void on_detail_stop(lv_event_t *e);
static void on_vol_changed(lv_event_t *e);

/* Free malloc'd user_data from g_list children before deleting */
static void free_list_userdata(void)
{
    if (!g_list) return;
    uint32_t cnt = lv_obj_get_child_cnt(g_list);
    for (uint32_t i = 0; i < cnt; i++) {
        lv_obj_t *c = lv_obj_get_child(g_list, i);
        uintptr_t d = (uintptr_t)lv_obj_get_user_data(c);
        if (d) free((void *)(d & ~1));
    }
}

/* ======================== Title Bar ======================== */
static void create_title(void)
{
    g_title = lv_obj_create(g_scr);
    lv_obj_add_style(g_title, &sty_title, 0);
    lv_obj_set_size(g_title, DISPLAY_WIDTH, STATUSBAR_H);
    lv_obj_set_pos(g_title, 0, 0);
    lv_obj_remove_flag(g_title, LV_OBJ_FLAG_SCROLLABLE);

    g_back_btn = lv_button_create(g_title);
    lv_obj_set_size(g_back_btn, 56, 22);
    lv_obj_align(g_back_btn, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_add_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(g_back_btn, on_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(g_back_btn);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_12, 0);
    lv_obj_center(bl);

    g_title_label = lv_label_create(g_title);
    lv_obj_set_style_text_color(g_title_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(g_title_label, &lv_font_montserrat_14, 0);
    lv_obj_align(g_title_label, LV_ALIGN_CENTER, 0, 0);
}

/* ======================== Player Bar ======================== */
static void create_player_bar(void)
{
    g_player_bar = lv_obj_create(g_scr);
    lv_obj_add_style(g_player_bar, &sty_bar, 0);
    lv_obj_set_size(g_player_bar, DISPLAY_WIDTH, PLAYER_BAR_H);
    lv_obj_set_pos(g_player_bar, 0, DISPLAY_HEIGHT - PLAYER_BAR_H);
    lv_obj_remove_flag(g_player_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_player_bar, on_bar_click, LV_EVENT_CLICKED, NULL);

    g_bar_song_label = lv_label_create(g_player_bar);
    lv_label_set_text(g_bar_song_label, "No song playing");
    lv_obj_set_style_text_color(g_bar_song_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(g_bar_song_label, g_cjk_font, 0);
    lv_obj_align(g_bar_song_label, LV_ALIGN_LEFT_MID, 8, -8);
    lv_label_set_long_mode(g_bar_song_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(g_bar_song_label, DISPLAY_WIDTH - 64);

    g_bar_play_btn = lv_button_create(g_player_bar);
    lv_obj_set_size(g_bar_play_btn, 30, 30);
    lv_obj_align(g_bar_play_btn, LV_ALIGN_RIGHT_MID, -8, -2);
    lv_obj_add_event_cb(g_bar_play_btn, on_bar_play_pause, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bicon = lv_label_create(g_bar_play_btn);
    lv_label_set_text(bicon, LV_SYMBOL_PLAY);
    lv_obj_center(bicon);

    /* Time label: "MM:SS / MM:SS" or "MM:SS" */
    g_bar_time_label = lv_label_create(g_player_bar);
    lv_label_set_text(g_bar_time_label, "");
    lv_obj_set_style_text_color(g_bar_time_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(g_bar_time_label, &lv_font_montserrat_12, 0);
    lv_obj_align(g_bar_time_label, LV_ALIGN_LEFT_MID, 8, 10);
}

/* ======================== Main Screen ======================== */
static void show_main(void)
{
    printf("[music] show_main\n");
    g_browsing = false;
    g_showing_now_playing = false;
    g_showing_bt = false;
    g_showing_settings = false;
    g_showing_pad = false;

    if (g_list)             { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
    if (g_bt_cont)          { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }
    if (g_bt_timer)         { lv_timer_del(g_bt_timer); g_bt_timer = NULL; }
    if (g_settings_cont)    { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }
    if (g_pad_cont)         { lv_obj_delete(g_pad_cont); g_pad_cont = NULL; }

    lv_obj_add_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Music Player");

    if (!g_main_cont) {
        g_main_cont = lv_obj_create(g_scr);
        lv_obj_set_size(g_main_cont, DISPLAY_WIDTH, CONTENT_H);
        lv_obj_set_pos(g_main_cont, 0, STATUSBAR_H);
        lv_obj_set_style_bg_opa(g_main_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_main_cont, 0, 0);
        lv_obj_remove_flag(g_main_cont, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_remove_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(g_main_cont);

    int bw = 140, bh = 60, gap = 16;
    int sx = (DISPLAY_WIDTH - bw * 2 - gap) / 2;
    int sy = 10;

    struct { const char *text; int color; lv_event_cb_t cb; } btns[] = {
        {"Play from NAND",    COLOR_ACCENT, on_play_nand},
        {"Play from SD",      COLOR_BTN1,   on_play_sd},
        {"Music Pad",         COLOR_BTN2,   on_now_playing},
        {"Settings",          COLOR_BTN3,   on_settings_btn},
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(g_main_cont);
        lv_obj_add_style(btn, &sty_btn, 0);
        lv_obj_set_size(btn, bw, bh);
        lv_obj_set_style_bg_color(btn, lv_color_hex(btns[i].color), 0);
        int col = i % 2, row = i / 2;
        lv_obj_set_pos(btn, sx + col * (bw + gap), sy + row * (bh + gap));
        if (btns[i].cb) lv_obj_add_event_cb(btn, btns[i].cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btns[i].text);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);
    }
    update_player_bar();
}

/* ======================== SD File Browser ======================== */
static bool is_system_dir(const char *name)
{
    const char *skip[] = {
        "System Volume Information","SYSTEM~1","$RECYCLE.BIN",
        "RECYCLED","LOST.DIR",".Trashes",".fseventsd",
        ".Spotlight-V100",".TemporaryItems",NULL
    };
    for (int i = 0; skip[i]; i++)
        if (strcasecmp(name, skip[i]) == 0) return true;
    return false;
}

static void show_browser(void)
{
    printf("[music] show_browser path=%s\n", g_path);

    /* Check if path is available */
    struct stat st;
    if (stat(g_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        g_browsing = true;
        if (g_list) { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
        lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(g_title_label, "Not Found");
        g_msg = lv_label_create(g_scr);
        lv_obj_add_style(g_msg, &sty_msg, 0);
        lv_label_set_text(g_msg, "Path not available\n\nPress Back to return");
        lv_obj_set_style_text_align(g_msg, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(g_msg);
        return;
    }

    g_browsing = true;
    g_showing_now_playing = false;
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
    lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Browse");

    if (!g_list) {
        g_list = lv_list_create(g_scr);
        lv_obj_set_size(g_list, DISPLAY_WIDTH, CONTENT_H);
        lv_obj_set_pos(g_list, 0, STATUSBAR_H);
        lv_obj_set_style_bg_opa(g_list, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_list, 0, 0);
        lv_obj_set_style_pad_all(g_list, 4, 0);
    }
    strcpy(g_path, SDCARD_PATH);
    refresh_list();
}

static void refresh_list(void)
{
    if (!g_list) return;
    free_list_userdata();
    lv_obj_clean(g_list);

    const char *display = g_path;
    if (strncmp(g_path, SDCARD_PATH, strlen(SDCARD_PATH)) == 0) {
        display = g_path + strlen(SDCARD_PATH);
        if (display[0] == '\0') display = "/";
    }
    lv_label_set_text(g_title_label, display);
    printf("[music] browsing: %s\n", g_path);

    DIR *dir = opendir(g_path);
    if (!dir) {
        lv_obj_t *l = lv_label_create(g_list);
        lv_label_set_text(l, "Cannot open directory");
        return;
    }

    if (strcmp(g_path, SDCARD_PATH) != 0) {
        lv_obj_t *btn = lv_button_create(g_list);
        lv_obj_set_size(btn, DISPLAY_WIDTH - 16, 36);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, on_back, LV_EVENT_CLICKED, NULL);
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, LV_SYMBOL_DIRECTORY);
        lv_obj_add_style(icon, &sty_dir, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 6, 0);
        lv_obj_t *nm = lv_label_create(btn);
        lv_label_set_text(nm, ".. (parent)");
        lv_obj_add_style(nm, &sty_dir, 0);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 34, 0);
    }

    char names[256][256]; bool isdir[256]; int n = 0;
    struct dirent *e;

    rewinddir(dir);
    while ((e = readdir(dir)) && n < 256) {
        if (e->d_name[0] == '.') continue;
        if (is_system_dir(e->d_name)) continue;
        if (e->d_type != DT_DIR) continue;
        strncpy(names[n], e->d_name, 255); isdir[n] = true; n++;
    }
    rewinddir(dir);
    while ((e = readdir(dir)) && n < 256) {
        if (e->d_name[0] == '.') continue;
        if (e->d_type != DT_REG) continue;
        strncpy(names[n], e->d_name, 255); isdir[n] = false; n++;
    }
    closedir(dir);

    if (n == 0) {
        lv_obj_t *l = lv_label_create(g_list);
        lv_label_set_text(l, "No files found");
        return;
    }

    g_selected_idx = -1;
    for (int i = 0; i < n; i++) {
        lv_obj_t *btn = lv_button_create(g_list);
        lv_obj_set_size(btn, DISPLAY_WIDTH - 16, 36);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        char *fp = malloc(512);
        snprintf(fp, 512, "%s/%s", g_path, names[i]);
        lv_obj_set_user_data(btn, (void*)((uintptr_t)fp | (isdir[i] ? 1 : 0)));
        lv_obj_add_event_cb(btn, on_item, LV_EVENT_CLICKED, NULL);

        /* Highlight currently playing file */
        if (!isdir[i] && strcmp(fp, g_playing_path) == 0) {
            lv_obj_add_style(btn, &sty_selected, 0);
            g_selected_idx = i;
        }

        lv_obj_t *icon = lv_label_create(btn);
        const char *itext;
        if (isdir[i]) itext = LV_SYMBOL_DIRECTORY;
        else if (music_is_audio_file(names[i])) itext = LV_SYMBOL_AUDIO;
        else itext = LV_SYMBOL_FILE;
        lv_label_set_text(icon, itext);
        lv_obj_add_style(icon, isdir[i] ? &sty_dir : &sty_file, 0);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 6, 0);

        lv_obj_t *nm = lv_label_create(btn);
        lv_label_set_text(nm, names[i]);
        lv_obj_add_style(nm, isdir[i] ? &sty_dir : &sty_file, 0);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 34, 0);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(nm, DISPLAY_WIDTH - 70);
    }
    printf("[music] listed %d entries\n", n);
}

/* ======================== Now-Playing Screen ======================== */
static void show_now_playing(void)
{
    printf("[music] show_now_playing\n");
    g_showing_now_playing = true;
    g_browsing = false;

    if (g_list)             { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_bt_cont)          { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }
    if (g_settings_cont)    { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }
    lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);

    /* HIDE player bar - controls are now inside this screen */
    lv_obj_add_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Now Playing");

    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }

    g_now_playing_cont = lv_obj_create(g_scr);
    lv_obj_add_style(g_now_playing_cont, &sty_nowplaying, 0);
    lv_obj_set_size(g_now_playing_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT - STATUSBAR_H);
    lv_obj_set_pos(g_now_playing_cont, 0, STATUSBAR_H);
    lv_obj_remove_flag(g_now_playing_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Cover placeholder */
    lv_obj_t *cover = lv_obj_create(g_now_playing_cont);
    int cs = 100;
    lv_obj_set_size(cover, cs, cs);
    lv_obj_align(cover, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_bg_color(cover, lv_color_hex(0x2A2A4E), 0);
    lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cover, 12, 0);
    lv_obj_set_style_border_width(cover, 0, 0);
    lv_obj_remove_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *cv = lv_label_create(cover);
    lv_label_set_text(cv, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(cv, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_text_font(cv, &lv_font_montserrat_48, 0);
    lv_obj_center(cv);

    /* Song name */
    g_detail_song_label = lv_label_create(g_now_playing_cont);
    lv_obj_set_style_text_color(g_detail_song_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(g_detail_song_label, g_cjk_font, 0);
    lv_label_set_long_mode(g_detail_song_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(g_detail_song_label, DISPLAY_WIDTH - 32);
    lv_obj_align(g_detail_song_label, LV_ALIGN_TOP_MID, 0, cs + 20);

    /* Controls at bottom */
    int btn_y = DISPLAY_HEIGHT - STATUSBAR_H - 52;

    lv_obj_t *prev = lv_button_create(g_now_playing_cont);
    lv_obj_set_size(prev, 50, 40);
    lv_obj_set_pos(prev, 20, btn_y);
    lv_obj_add_style(prev, &sty_btn, 0);
    lv_obj_set_style_bg_color(prev, lv_color_hex(COLOR_BTN1), 0);
    lv_obj_t *pl = lv_label_create(prev);
    lv_label_set_text(pl, LV_SYMBOL_PREV);
    lv_obj_center(pl);

    g_detail_play_btn = lv_button_create(g_now_playing_cont);
    lv_obj_set_size(g_detail_play_btn, 80, 40);
    lv_obj_align(g_detail_play_btn, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_add_style(g_detail_play_btn, &sty_btn, 0);
    lv_obj_set_style_bg_color(g_detail_play_btn, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_add_event_cb(g_detail_play_btn, on_detail_play_pause, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pd = lv_label_create(g_detail_play_btn);
    lv_label_set_text(pd, LV_SYMBOL_PLAY);
    lv_obj_center(pd);

    g_detail_stop_btn = lv_button_create(g_now_playing_cont);
    lv_obj_set_size(g_detail_stop_btn, 50, 40);
    lv_obj_set_pos(g_detail_stop_btn, DISPLAY_WIDTH - 70, btn_y);
    lv_obj_add_style(g_detail_stop_btn, &sty_btn, 0);
    lv_obj_set_style_bg_color(g_detail_stop_btn, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_event_cb(g_detail_stop_btn, on_detail_stop, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(g_detail_stop_btn);
    lv_label_set_text(sl, LV_SYMBOL_STOP);
    lv_obj_center(sl);

    update_detail_controls();
}

/* ======================== Music Pad (4x4 tone grid) ======================== */
/* Standard musical note frequencies (Hz) - 4x4 grid starting from C4 */
static const uint32_t g_note_freqs[4][4] = {
    { 262, 294, 330, 349 },  /* C4  D4  E4  F4  */
    { 392, 440, 494, 523 },  /* G4  A4  B4  C5  */
    { 587, 659, 698, 784 },  /* D5  E5  F5  G5  */
    { 880, 988, 1047, 1175 }, /* A5  B5  C6  D6  */
};
static const char *g_note_names[4][4] = {
    { "C4", "D4", "E4", "F4" },
    { "G4", "A4", "B4", "C5" },
    { "D5", "E5", "F5", "G5" },
    { "A5", "B5", "C6", "D6" },
};

static void on_pad_press(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    uintptr_t freq = (uintptr_t)lv_obj_get_user_data(btn);
    if (freq == 0) return;
    printf("[pad] note: %lu Hz\n", (unsigned long)freq);
    /* Quick 300ms tone - feels like a real instrument pad */
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "TONE:%lu:1", (unsigned long)freq);
    strncpy(g_pending_path, cmd, 255);
    g_pending_path[255] = '\0';
}

static void show_music_pad(void)
{
    printf("[music] show_music_pad\n");
    g_showing_pad = true;
    g_browsing = false;

    if (g_list)             { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
    if (g_bt_cont)          { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }
    if (g_settings_cont)    { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }
    lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Music Pad");

    if (g_pad_cont) { lv_obj_delete(g_pad_cont); g_pad_cont = NULL; }

    g_pad_cont = lv_obj_create(g_scr);
    lv_obj_set_size(g_pad_cont, DISPLAY_WIDTH, DISPLAY_HEIGHT - STATUSBAR_H);
    lv_obj_set_pos(g_pad_cont, 0, STATUSBAR_H);
    lv_obj_set_style_bg_opa(g_pad_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_pad_cont, 0, 0);
    lv_obj_remove_flag(g_pad_cont, LV_OBJ_FLAG_SCROLLABLE);

    int cols = 4, rows = 4;
    int pad_w = (DISPLAY_WIDTH - 24) / cols;
    int pad_h = (DISPLAY_HEIGHT - STATUSBAR_H - 24) / rows;
    int gap = 4;

    /* Colors for each row */
    uint32_t row_colors[] = {
        0xE91E63,  /* Pink   */
        0x3498DB,  /* Blue   */
        0x2ECC71,  /* Green  */
        0xF39C12,  /* Orange */
    };

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            lv_obj_t *btn = lv_button_create(g_pad_cont);
            lv_obj_set_size(btn, pad_w, pad_h);
            int x = 8 + c * (pad_w + gap);
            int y = 8 + r * (pad_h + gap);
            lv_obj_set_pos(btn, x, y);
            lv_obj_set_style_radius(btn, 8, 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(row_colors[r]), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

            /* Store frequency in user_data */
            lv_obj_set_user_data(btn, (void *)(uintptr_t)g_note_freqs[r][c]);
            lv_obj_add_event_cb(btn, on_pad_press, LV_EVENT_PRESSED, NULL);

            /* Note name label */
            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, g_note_names[r][c]);
            lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
            lv_obj_center(lbl);
        }
    }
}

/* ======================== Bluetooth Screen ======================== */
static void bt_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!g_showing_bt || !g_bt_status_label) return;

    const char *status = music_bt_get_status();
    lv_label_set_text(g_bt_status_label, status);

    /* Also try to init if not yet done */
    if (!music_bt_is_on()) {
        music_bt_init();
    }
}

static void show_bluetooth(void)
{
    printf("[music] show_bluetooth\n");
    g_showing_bt = true;
    g_browsing = false;

    if (g_list)             { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
    if (g_settings_cont)    { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }
    lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Bluetooth");

    if (g_bt_cont) { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }

    g_bt_cont = lv_obj_create(g_scr);
    lv_obj_set_size(g_bt_cont, DISPLAY_WIDTH, CONTENT_H);
    lv_obj_set_pos(g_bt_cont, 0, STATUSBAR_H);
    lv_obj_set_style_bg_opa(g_bt_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_bt_cont, 0, 0);
    lv_obj_remove_flag(g_bt_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Bluetooth icon */
    lv_obj_t *icon = lv_label_create(g_bt_cont);
    lv_label_set_text(icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(icon, lv_color_hex(COLOR_BTN1), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 30);

    /* Status */
    g_bt_status_label = lv_label_create(g_bt_cont);
    lv_obj_add_style(g_bt_status_label, &sty_bt_status, 0);
    lv_label_set_text(g_bt_status_label, "Starting...");
    lv_obj_align(g_bt_status_label, LV_ALIGN_TOP_MID, 0, 90);

    /* Device name */
    g_bt_name_label = lv_label_create(g_bt_cont);
    lv_obj_set_style_text_color(g_bt_name_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(g_bt_name_label, g_cjk_font, 0);
    lv_label_set_text(g_bt_name_label, "Gemini-S1 A2DP Sink");
    lv_obj_align(g_bt_name_label, LV_ALIGN_TOP_MID, 0, 120);

    /* Init Bluetooth and set discoverable */
    music_bt_init();

    if (g_bt_timer) lv_timer_del(g_bt_timer);
    g_bt_timer = lv_timer_create(bt_timer_cb, 2000, NULL);
}

/* ======================== Settings Screen ======================== */
static void on_vol_changed(lv_event_t *e)
{
    int vol = lv_slider_get_value(lv_event_get_target(e));
    if (g_vol_label) {
        char buf[32]; snprintf(buf, sizeof(buf), "Volume: %d%%", vol);
        lv_label_set_text(g_vol_label, buf);
    }
}

static void on_test_tone(lv_event_t *e)
{
    (void)e;
    printf("[music] test tone button pressed\n");
    /* 1kHz tone for 2 seconds - bypasses SD card entirely */
    strncpy(g_pending_path, "TONE:1000:2", 255);
    g_pending_path[255] = '\0';
}

static void show_settings(void)
{
    printf("[music] show_settings\n");
    g_showing_settings = true;
    g_browsing = false;

    if (g_list)             { free_list_userdata(); lv_obj_delete(g_list); g_list = NULL; }
    if (g_msg)              { lv_obj_delete(g_msg); g_msg = NULL; }
    if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
    if (g_bt_cont)          { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }
    lv_obj_add_flag(g_main_cont, LV_OBJ_FLAG_HIDDEN);

    lv_obj_remove_flag(g_back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(g_player_bar, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(g_title_label, "Settings");

    if (g_settings_cont) { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }

    g_settings_cont = lv_obj_create(g_scr);
    lv_obj_set_size(g_settings_cont, DISPLAY_WIDTH, CONTENT_H);
    lv_obj_set_pos(g_settings_cont, 0, STATUSBAR_H);
    lv_obj_set_style_bg_opa(g_settings_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_settings_cont, 0, 0);
    lv_obj_remove_flag(g_settings_cont, LV_OBJ_FLAG_SCROLLABLE);

    g_vol_label = lv_label_create(g_settings_cont);
    lv_label_set_text(g_vol_label, "Volume: 80%");
    lv_obj_set_style_text_color(g_vol_label, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_set_style_text_font(g_vol_label, g_cjk_font, 0);
    lv_obj_align(g_vol_label, LV_ALIGN_TOP_MID, 0, 30);

    g_vol_slider = lv_slider_create(g_settings_cont);
    lv_obj_set_size(g_vol_slider, DISPLAY_WIDTH - 60, 20);
    lv_obj_align(g_vol_slider, LV_ALIGN_TOP_MID, 0, 60);
    lv_slider_set_range(g_vol_slider, 0, 100);
    lv_slider_set_value(g_vol_slider, DEFAULT_VOLUME, LV_ANIM_OFF);
    lv_obj_add_style(g_vol_slider, &sty_slider, LV_PART_INDICATOR);
    lv_obj_add_event_cb(g_vol_slider, on_vol_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* Test Tone button - tests audio hardware without SD card */
    lv_obj_t *tone_btn = lv_button_create(g_settings_cont);
    lv_obj_set_size(tone_btn, 200, 40);
    lv_obj_align(tone_btn, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_add_style(tone_btn, &sty_btn, 0);
    lv_obj_set_style_bg_color(tone_btn, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_add_event_cb(tone_btn, on_test_tone, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tl = lv_label_create(tone_btn);
    lv_label_set_text(tl, "Test Tone (1kHz)");
    lv_obj_set_style_text_color(tl, lv_color_white(), 0);
    lv_obj_center(tl);
}

/* ======================== Player Bar Update ======================== */
static void update_player_bar(void)
{
    if (!g_player_bar) return;
    const char *cur = music_playback_current();
    bool playing = music_playback_is_playing();

    if (cur && playing) {
        const char *bn = strrchr(cur, '/');
        lv_label_set_text(g_bar_song_label, bn ? bn + 1 : cur);

        /* Update time display */
        uint32_t elapsed = music_playback_elapsed();
        uint32_t total   = music_playback_total();
        char tbuf[32];
        if (total > 0) {
            snprintf(tbuf, sizeof(tbuf), "%u:%02u / %u:%02u",
                     elapsed / 60, elapsed % 60,
                     total / 60, total % 60);
        } else if (elapsed > 0) {
            snprintf(tbuf, sizeof(tbuf), "%u:%02u",
                     elapsed / 60, elapsed % 60);
        } else {
            tbuf[0] = '\0';
        }
        lv_label_set_text(g_bar_time_label, tbuf);
    } else {
        lv_label_set_text(g_bar_song_label, "No song playing");
        lv_label_set_text(g_bar_time_label, "");
    }
    if (g_bar_play_btn) {
        lv_obj_t *ic = lv_obj_get_child(g_bar_play_btn, 0);
        if (ic) lv_label_set_text(ic, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

static void update_detail_controls(void)
{
    if (!g_detail_play_btn) return;
    bool playing = music_playback_is_playing();
    const char *cur = music_playback_current();

    if (g_detail_song_label) {
        if (cur && playing) {
            const char *bn = strrchr(cur, '/');
            lv_label_set_text(g_detail_song_label, bn ? bn + 1 : cur);
        } else {
            lv_label_set_text(g_detail_song_label, "No song playing");
        }
    }
    lv_obj_t *ic = lv_obj_get_child(g_detail_play_btn, 0);
    if (ic) lv_label_set_text(ic, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

/* ======================== Event Handlers ======================== */
static void on_item(lv_event_t *e)
{
    uintptr_t data = (uintptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (!data) return;
    bool is_dir = (data & 1) != 0;
    const char *fp = (const char *)(data & ~1);

    if (is_dir) {
        strncpy(g_path, fp, 255);
        refresh_list();
    } else if (music_is_audio_file(fp)) {
        printf("[music] PLAY: %s\n", fp);
        /* Defer the actual stop+play to the main loop.
         * Calling nxplayer_stop() in LVGL event handler can block
         * for seconds if DMA is timing out, freezing the UI.
         * Instead, save the path and let main loop handle it.
         */
        strncpy(g_pending_path, fp, 255);
        g_pending_path[255] = '\0';
    }
}

/* Execute pending play in a SEPARATE THREAD so main loop never blocks.
 * nxplayer_playfile() may block if audio device is busy or DMA hangs.
 * Running it in a thread keeps LVGL UI responsive. */
static void *play_thread(void *arg)
{
    (void)arg;
    while (1) {
        /* Wait for a pending request */
        if (g_pending_path[0] == '\0') {
            usleep(100000); /* 100ms poll */
            continue;
        }

        char path[256];
        strncpy(path, g_pending_path, 255);
        path[255] = '\0';
        g_pending_path[0] = '\0';

        printf("[music] play_thread: %s\n", path);

        bool ok;
        if (strncmp(path, "TONE:", 5) == 0) {
            /* Test tone: "TONE:freq:duration" */
            uint32_t freq = 1000, dur = 2;
            sscanf(path, "TONE:%lu:%lu", &freq, &dur);
            ok = music_playback_test_tone(freq, dur);
        } else {
            ok = music_playback_play(path);
        }
        if (ok) {
            strncpy(g_playing_path, path, 255);
            g_playing_path[255] = '\0';
        }
        printf("[music] play_thread done: %d\n", ok);
    }
    return NULL;
}

static void on_bar_click(lv_event_t *e)
{
    (void)e;
    if (music_playback_is_playing() || g_playing_path[0])
        show_now_playing();
}

static void on_bar_play_pause(lv_event_t *e)
{
    (void)e;
    if (music_playback_is_playing()) {
        music_playback_pause();
        update_player_bar();
        update_detail_controls();
    } else if (g_playing_path[0]) {
        /* Defer to avoid blocking UI if DMA is stuck */
        strncpy(g_pending_path, g_playing_path, 255);
        g_pending_path[255] = '\0';
    }
}

static void on_detail_play_pause(lv_event_t *e)
{
    (void)e;
    if (music_playback_is_playing()) {
        music_playback_pause();
        update_player_bar();
        update_detail_controls();
    } else if (g_playing_path[0]) {
        strncpy(g_pending_path, g_playing_path, 255);
        g_pending_path[255] = '\0';
    }
}

static void on_detail_stop(lv_event_t *e)
{
    (void)e;
    music_playback_stop();
    g_playing_path[0] = '\0';
    update_player_bar();
    update_detail_controls();
    if (g_detail_song_label)
        lv_label_set_text(g_detail_song_label, "No song playing");
}

static void do_back(void)
{
    printf("[music] back: np=%d bt=%d set=%d pad=%d browse=%d\n",
           g_showing_now_playing, g_showing_bt, g_showing_settings,
           g_showing_pad, g_browsing);

    if (g_showing_now_playing) {
        g_showing_now_playing = false;
        if (g_now_playing_cont) { lv_obj_delete(g_now_playing_cont); g_now_playing_cont = NULL; }
        show_main();
        return;
    }
    if (g_showing_bt) {
        g_showing_bt = false;
        if (g_bt_cont)  { lv_obj_delete(g_bt_cont); g_bt_cont = NULL; }
        if (g_bt_timer) { lv_timer_del(g_bt_timer); g_bt_timer = NULL; }
        g_bt_status_label = NULL;
        g_bt_name_label = NULL;
        show_main();
        return;
    }
    if (g_showing_settings) {
        g_showing_settings = false;
        if (g_settings_cont) { lv_obj_delete(g_settings_cont); g_settings_cont = NULL; }
        g_vol_label = NULL;
        g_vol_slider = NULL;
        show_main();
        return;
    }
    if (g_showing_pad) {
        g_showing_pad = false;
        if (g_pad_cont) { lv_obj_delete(g_pad_cont); g_pad_cont = NULL; }
        show_main();
        return;
    }
    if (!g_browsing) return;
    /* Back to main from root directories */
    if (strcmp(g_path, SDCARD_PATH) == 0 ||
        strcmp(g_path, DATA_PATH) == 0 ||
        strcmp(g_path, "/") == 0) {
        show_main(); return;
    }
    char *slash = strrchr(g_path, '/');
    if (slash) *slash = '\0';
    refresh_list();
}

/* ======================== Entry ======================== */
static void tick_cb(lv_timer_t *t)
{
    (void)t;
    /* Detect playback completion (nxplayer state transition to IDLE) */
    music_playback_update();
    /* Update time display every second */
    update_player_bar();
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    lv_init();

    lv_nuttx_dsc_t info; lv_nuttx_result_t res;
    lv_nuttx_dsc_init(&info);
    info.fb_path = "/dev/lcd0";
    lv_nuttx_init(&info, &res);
    if (!res.disp) { printf("Music: display init failed\n"); return 1; }

    g_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_scr, lv_color_hex(COLOR_BG), 0);
    lv_scr_load(g_scr);

    init_cjk_font();
    init_styles();
    create_title();
    create_player_bar();
    show_main();

    g_time_timer = lv_timer_create(tick_cb, 1000, NULL);

    /* Start background thread for non-blocking playback */
    pthread_t pt;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 4096);
    pthread_create(&pt, &attr, play_thread, NULL);
    pthread_attr_destroy(&attr);

    while (1) {
        uint32_t idle = lv_timer_handler();
        /* UI updates from play_thread are done via LVGL timer (tick_cb) */
        idle = idle ? idle : 1;
        usleep(idle * 1000);
    }
    return 0;
}
