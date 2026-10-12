/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_files.c
 *
 * LVGL-based SD card file browser for audio playback.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include <lvgl/lvgl.h>
#include "launcher_files.h"
#include "launcher_player.h"
#include "launcher_config.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *g_files_screen = NULL;
static lv_obj_t *g_files_parent = NULL;  /* Screen to return to */
static lv_obj_t *g_title_bar = NULL;
static lv_obj_t *g_path_label = NULL;
static lv_obj_t *g_file_list = NULL;
static lv_obj_t *g_back_btn = NULL;
static lv_obj_t *g_msg_label = NULL;

static char g_current_path[256] = "/sdcard";
static lv_style_t g_style_title;
static lv_style_t g_style_list_btn;
static lv_style_t g_style_dir_label;
static lv_style_t g_style_file_label;

/****************************************************************************
 * Forward Declarations
 ****************************************************************************/

static void refresh_file_list(void);
static void on_back_click(lv_event_t *e);
static void on_item_click(lv_event_t *e);

/****************************************************************************
 * Styles
 ****************************************************************************/

static void init_styles(void)
{
    lv_style_init(&g_style_title);
    lv_style_set_bg_color(&g_style_title, lv_color_hex(0x1A1A2E));
    lv_style_set_bg_opa(&g_style_title, LV_OPA_COVER);
    lv_style_set_text_color(&g_style_title, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&g_style_title, 4);
    lv_style_set_border_width(&g_style_title, 0);

    lv_style_init(&g_style_list_btn);
    lv_style_set_bg_opa(&g_style_list_btn, LV_OPA_TRANSP);
    lv_style_set_border_width(&g_style_list_btn, 0);
    lv_style_set_pad_all(&g_style_list_btn, 4);
    lv_style_set_radius(&g_style_list_btn, 4);

    lv_style_init(&g_style_dir_label);
    lv_style_set_text_color(&g_style_dir_label, lv_color_hex(0x3498DB));
    lv_style_set_text_font(&g_style_dir_label, &lv_font_montserrat_16);

    lv_style_init(&g_style_file_label);
    lv_style_set_text_color(&g_style_file_label, lv_color_hex(0x2ECC71));
    lv_style_set_text_font(&g_style_file_label, &lv_font_montserrat_16);
}

/****************************************************************************
 * UI Creation
 ****************************************************************************/

static void create_title_bar(void)
{
    g_title_bar = lv_obj_create(g_files_screen);
    lv_obj_add_style(g_title_bar, &g_style_title, 0);
    lv_obj_set_size(g_title_bar, DISPLAY_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(g_title_bar, 0, 0);
    lv_obj_remove_flag(g_title_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Back button */
    g_back_btn = lv_button_create(g_title_bar);
    lv_obj_set_size(g_back_btn, 60, 24);
    lv_obj_align(g_back_btn, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_add_event_cb(g_back_btn, on_back_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(g_back_btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl);

    /* Path label */
    g_path_label = lv_label_create(g_title_bar);
    lv_obj_set_style_text_color(g_path_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(g_path_label, &lv_font_montserrat_10, 0);
    lv_obj_align(g_path_label, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_label_set_long_mode(g_path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(g_path_label, DISPLAY_WIDTH - 80);
}

static void create_msg_screen(const char *msg)
{
    g_msg_label = lv_label_create(g_files_screen);
    lv_label_set_text(g_msg_label, msg);
    lv_obj_set_style_text_color(g_msg_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(g_msg_label, &lv_font_montserrat_18, 0);
    lv_obj_center(g_msg_label);
    lv_obj_set_style_text_align(g_msg_label, LV_TEXT_ALIGN_CENTER, 0);

    /* Hide file list */
    if (g_file_list) {
        lv_obj_add_flag(g_file_list, LV_OBJ_FLAG_HIDDEN);
    }
}

/****************************************************************************
 * File List
 ****************************************************************************/

static void refresh_file_list(void)
{
    DIR *dir;
    struct dirent *entry;
    char fullpath[512];
    struct stat st;

    if (!g_file_list) return;

    /* Clear existing items */
    lv_obj_clean(g_file_list);

    /* Hide message label */
    if (g_msg_label) {
        lv_obj_add_flag(g_msg_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* Update path display */
    lv_label_set_text(g_path_label, g_current_path);

    dir = opendir(g_current_path);
    if (!dir) {
        create_msg_screen("Cannot open directory");
        return;
    }

    /* Collect entries: directories first, then audio files */
    struct {
        char name[256];
        bool is_dir;
    } entries[256];
    int n_entries = 0;

    while ((entry = readdir(dir)) != NULL && n_entries < 256) {
        if (strcmp(entry->d_name, ".") == 0) continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", g_current_path, entry->d_name);
        if (stat(fullpath, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            /* Skip hidden dirs */
            if (entry->d_name[0] == '.') continue;
            strncpy(entries[n_entries].name, entry->d_name, 255);
            entries[n_entries].is_dir = true;
            n_entries++;
        }
    }

    /* Second pass: audio files */
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL && n_entries < 256) {
        if (strcmp(entry->d_name, ".") == 0) continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", g_current_path, entry->d_name);
        if (stat(fullpath, &st) != 0) continue;

        if (S_ISREG(st.st_mode) && launcher_player_is_audio_file(entry->d_name)) {
            strncpy(entries[n_entries].name, entry->d_name, 255);
            entries[n_entries].is_dir = false;
            n_entries++;
        }
    }
    closedir(dir);

    if (n_entries == 0) {
        create_msg_screen("No audio files found");
        return;
    }

    /* Show the list */
    lv_obj_remove_flag(g_file_list, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < n_entries; i++) {
        lv_obj_t *btn = lv_button_create(g_file_list);
        lv_obj_add_style(btn, &g_style_list_btn, 0);
        lv_obj_set_size(btn, DISPLAY_WIDTH - 16, 38);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

        /* Store fullpath as user data */
        char *path_copy = malloc(512);
        if (path_copy) {
            snprintf(path_copy, 512, "%s/%s", g_current_path, entries[i].name);
            lv_obj_set_user_data(btn, path_copy);
        }

        lv_obj_add_event_cb(btn, on_item_click, LV_EVENT_CLICKED, NULL);

        /* Icon + label */
        lv_obj_t *icon = lv_label_create(btn);
        if (entries[i].is_dir) {
            lv_label_set_text(icon, LV_SYMBOL_DIRECTORY);
            lv_obj_add_style(icon, &g_style_dir_label, 0);
        } else {
            lv_label_set_text(icon, LV_SYMBOL_AUDIO);
            lv_obj_add_style(icon, &g_style_file_label, 0);
        }
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 8, 0);

        lv_obj_t *name_label = lv_label_create(btn);
        lv_label_set_text(name_label, entries[i].name);
        if (entries[i].is_dir) {
            lv_obj_add_style(name_label, &g_style_dir_label, 0);
        } else {
            lv_obj_add_style(name_label, &g_style_file_label, 0);
        }
        lv_obj_align(name_label, LV_ALIGN_LEFT_MID, 36, 0);
        lv_label_set_long_mode(name_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(name_label, DISPLAY_WIDTH - 70);
    }
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

static void on_back_click(lv_event_t *e)
{
    (void)e;
    char *parent_path = strdup(g_current_path);
    char *last_slash = strrchr(parent_path, '/');

    if (last_slash && last_slash != parent_path) {
        *last_slash = '\0';
        if (strncmp(parent_path, "/sdcard", 7) == 0 || strcmp(parent_path, "/sdcard") == 0) {
            /* Still within /sdcard, navigate up */
            strncpy(g_current_path, parent_path, sizeof(g_current_path) - 1);
            refresh_file_list();
        } else {
            /* Trying to go above /sdcard - return to main screen */
            launcher_files_hide();
        }
    } else {
        /* At root of /sdcard - return to main */
        launcher_files_hide();
    }
    free(parent_path);
}

static void on_item_click(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *path = (const char *)lv_obj_get_user_data(btn);

    if (!path) return;

    struct stat st;
    if (stat(path, &st) != 0) return;

    if (S_ISDIR(st.st_mode)) {
        /* Navigate into directory */
        strncpy(g_current_path, path, sizeof(g_current_path) - 1);
        refresh_file_list();
    } else {
        /* Play the file */
        launcher_player_play(path);
        /* Stay on this screen for now */
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void launcher_files_show(lv_obj_t *parent)
{
    g_files_parent = parent;

    /* Check SD card */
    if (!launcher_sdcard_available()) {
        /* Create a simple screen showing "TF未插入" */
        g_files_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(g_files_screen, lv_color_hex(0x0D0D1A), 0);
        lv_scr_load(g_files_screen);

        init_styles();
        create_title_bar();
        lv_label_set_text(g_path_label, "SD Card");

        create_msg_screen("TF卡未插入\n\n请插入TF卡后重试");
        return;
    }

    /* Create file browser */
    g_files_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_files_screen, lv_color_hex(0x0D0D1A), 0);
    lv_scr_load(g_files_screen);

    init_styles();
    create_title_bar();

    /* Create scrollable list */
    g_file_list = lv_list_create(g_files_screen);
    lv_obj_set_size(g_file_list, DISPLAY_WIDTH, DISPLAY_HEIGHT - STATUS_BAR_HEIGHT - 8);
    lv_obj_set_pos(g_file_list, 0, STATUS_BAR_HEIGHT + 4);
    lv_obj_set_style_bg_opa(g_file_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_file_list, 0, 0);
    lv_obj_set_style_pad_all(g_file_list, 4, 0);

    /* Reset to root */
    strncpy(g_current_path, "/sdcard", sizeof(g_current_path) - 1);

    /* Populate */
    refresh_file_list();
}

void launcher_files_hide(void)
{
    /* Stop playback if any */
    launcher_player_stop();

    if (g_files_screen) {
        lv_obj_delete(g_files_screen);
        g_files_screen = NULL;
        g_file_list = NULL;
        g_msg_label = NULL;
        g_title_bar = NULL;
        g_path_label = NULL;
        g_back_btn = NULL;
    }

    /* Return to parent screen */
    if (g_files_parent) {
        lv_scr_load(g_files_parent);
    }
}
