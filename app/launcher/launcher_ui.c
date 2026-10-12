/****************************************************************************
 * contest2026_060_Breezystudio/app/launcher/launcher_ui.c
 *
 * Beautiful LVGL-based Launcher UI for Gemini-S1 (240x320).
 * Features: icon grid, page navigation, status bar, app launching.
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <lvgl/lvgl.h>
#include "launcher_ui.h"
#include "launcher_config.h"
#include "launcher_files.h"

/****************************************************************************
 * Icon Color Palette
 ****************************************************************************/

const lv_color_t g_icon_colors[COLOR_PALETTE_SIZE] = {
    LV_COLOR_MAKE(0xE7, 0x4C, 0x3C),  /* Red */
    LV_COLOR_MAKE(0x34, 0x98, 0xDB),  /* Blue */
    LV_COLOR_MAKE(0x2E, 0xCC, 0x71),  /* Green */
    LV_COLOR_MAKE(0xF3, 0x9C, 0x12),  /* Orange */
    LV_COLOR_MAKE(0x9B, 0x59, 0xB6),  /* Purple */
    LV_COLOR_MAKE(0x1A, 0xBC, 0x9C),  /* Teal */
    LV_COLOR_MAKE(0xE6, 0x7E, 0x22),  /* Dark Orange */
    LV_COLOR_MAKE(0x29, 0x80, 0xB9),  /* Dark Blue */
    LV_COLOR_MAKE(0x27, 0xAE, 0x60),  /* Dark Green */
    LV_COLOR_MAKE(0xF1, 0xC4, 0x0F),  /* Yellow */
    LV_COLOR_MAKE(0x8E, 0x44, 0xAD),  /* Dark Purple */
    LV_COLOR_MAKE(0xD3, 0x54, 0x00),  /* Pumpkin */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static launcher_app_t *g_apps = NULL;
static int g_app_count = 0;
static int g_total_pages = 1;
static int g_current_page = 0;

static lv_obj_t *g_screen = NULL;
static lv_obj_t *g_status_bar = NULL;
static lv_obj_t *g_status_title = NULL;
static lv_obj_t *g_status_clock = NULL;
static lv_obj_t *g_grid_container = NULL;
static lv_obj_t *g_page_dots[16];  /* Max 16 pages */
static lv_obj_t *g_left_arrow = NULL;
static lv_obj_t *g_right_arrow = NULL;
static lv_obj_t *g_app_icons[APPS_PER_PAGE];

static lv_style_t g_style_icon_bg;
static lv_style_t g_style_icon_label;
static lv_style_t g_style_status_bar;
static lv_style_t g_style_page_dot;
static lv_style_t g_style_page_dot_active;
static lv_style_t g_style_nav_btn;

/****************************************************************************
 * Forward Declarations
 ****************************************************************************/

static void create_status_bar(void);
static void create_grid(void);
static void create_page_indicators(void);
static void create_nav_buttons(void);
static void refresh_grid(void);
static void update_page_dots(void);
static void on_app_click(lv_event_t *e);
static void on_nav_prev(lv_event_t *e);
static void on_nav_next(lv_event_t *e);
static void on_music_click(lv_event_t *e);
static lv_obj_t *create_music_icon(lv_obj_t *parent, int col, int row);

/****************************************************************************
 * Style Initialization
 ****************************************************************************/

static void init_styles(void)
{
    /* Status bar */
    lv_style_init(&g_style_status_bar);
    lv_style_set_bg_color(&g_style_status_bar, lv_color_hex(STATUSBAR_BG_COLOR));
    lv_style_set_bg_opa(&g_style_status_bar, LV_OPA_COVER);
    lv_style_set_pad_all(&g_style_status_bar, 4);
    lv_style_set_border_width(&g_style_status_bar, 0);

    /* Icon background */
    lv_style_init(&g_style_icon_bg);
    lv_style_set_radius(&g_style_icon_bg, ICON_RADIUS);
    lv_style_set_border_width(&g_style_icon_bg, 0);
    lv_style_set_shadow_width(&g_style_icon_bg, 4);
    lv_style_set_shadow_color(&g_style_icon_bg, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&g_style_icon_bg, LV_OPA_20);
    lv_style_set_shadow_offset_x(&g_style_icon_bg, 0);
    lv_style_set_shadow_offset_y(&g_style_icon_bg, 2);

    /* Icon label */
    lv_style_init(&g_style_icon_label);
    lv_style_set_text_color(&g_style_icon_label, lv_color_white());
    lv_style_set_text_font(&g_style_icon_label, &lv_font_montserrat_12);

    /* Page dots */
    lv_style_init(&g_style_page_dot);
    lv_style_set_bg_color(&g_style_page_dot, lv_color_hex(0x555555));
    lv_style_set_bg_opa(&g_style_page_dot, LV_OPA_COVER);
    lv_style_set_radius(&g_style_page_dot, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&g_style_page_dot, 0);

    lv_style_init(&g_style_page_dot_active);
    lv_style_set_bg_color(&g_style_page_dot_active, lv_color_hex(0x3498DB));
    lv_style_set_bg_opa(&g_style_page_dot_active, LV_OPA_COVER);
    lv_style_set_radius(&g_style_page_dot_active, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&g_style_page_dot_active, 0);

    /* Navigation buttons */
    lv_style_init(&g_style_nav_btn);
    lv_style_set_bg_opa(&g_style_nav_btn, LV_OPA_30);
    lv_style_set_bg_color(&g_style_nav_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&g_style_nav_btn, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&g_style_nav_btn, 0);
    lv_style_set_shadow_width(&g_style_nav_btn, 2);
    lv_style_set_shadow_opa(&g_style_nav_btn, LV_OPA_20);
    lv_style_set_shadow_offset_y(&g_style_nav_btn, 1);
    lv_style_set_text_color(&g_style_nav_btn, lv_color_hex(0x333333));
    lv_style_set_text_font(&g_style_nav_btn, &lv_font_montserrat_18);
}

/****************************************************************************
 * Status Bar
 ****************************************************************************/

static void create_status_bar(void)
{
    g_status_bar = lv_obj_create(g_screen);
    lv_obj_add_style(g_status_bar, &g_style_status_bar, 0);
    lv_obj_set_size(g_status_bar, DISPLAY_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(g_status_bar, 0, 0);
    lv_obj_remove_flag(g_status_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    g_status_title = lv_label_create(g_status_bar);
    lv_label_set_text(g_status_title, "Vela Launcher");
    lv_obj_set_style_text_color(g_status_title, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(g_status_title, &lv_font_montserrat_14, 0);
    lv_obj_align(g_status_title, LV_ALIGN_LEFT_MID, 8, 0);

    /* Clock */
    g_status_clock = lv_label_create(g_status_bar);
    lv_label_set_text(g_status_clock, "00:00");
    lv_obj_set_style_text_color(g_status_clock, lv_color_hex(STATUSBAR_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(g_status_clock, &lv_font_montserrat_14, 0);
    lv_obj_align(g_status_clock, LV_ALIGN_RIGHT_MID, -8, 0);
}

/****************************************************************************
 * App Icon Creation
 ****************************************************************************/

static lv_obj_t *create_app_icon(lv_obj_t *parent, const launcher_app_t *app,
                                  int col, int row)
{
    lv_obj_t *container;
    lv_obj_t *icon_bg;
    lv_obj_t *label;
    char first_char[2];

    /* Container for icon + label */
    container = lv_obj_create(parent);
    lv_obj_set_size(container, ICON_SIZE, ICON_SIZE + ICON_LABEL_HEIGHT);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);

    /* Icon background (colored rounded rect with first letter) */
    icon_bg = lv_obj_create(container);
    lv_obj_add_style(icon_bg, &g_style_icon_bg, 0);
    lv_obj_set_size(icon_bg, ICON_SIZE, ICON_SIZE);
    lv_obj_set_pos(icon_bg, 0, 0);
    lv_obj_set_style_bg_color(icon_bg, app->color, 0);
    lv_obj_remove_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* First character of app name as icon */
    first_char[0] = app->name[0] ? app->name[0] : '?';
    first_char[1] = '\0';

    label = lv_label_create(icon_bg);
    lv_label_set_text(label, first_char);
    lv_obj_add_style(label, &g_style_icon_label, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    lv_obj_center(label);

    /* App name label below icon */
    label = lv_label_create(container);
    lv_label_set_text(label, app->name);
    lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_width(label, ICON_SIZE);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label, 0, ICON_SIZE + 2);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    /* Attach user data for click handler */
    lv_obj_set_user_data(container, (void *)(uintptr_t)(app - g_apps));

    /* Click event */
    lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(container, on_app_click, LV_EVENT_CLICKED, NULL);

    return container;
}

/****************************************************************************
 * Grid Layout
 ****************************************************************************/

static void create_grid(void)
{
    g_grid_container = lv_obj_create(g_screen);
    lv_obj_set_size(g_grid_container, DISPLAY_WIDTH,
                    DISPLAY_HEIGHT - STATUS_BAR_HEIGHT - 30);
    lv_obj_set_pos(g_grid_container, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(g_grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_grid_container, 0, 0);
    lv_obj_set_style_pad_all(g_grid_container, 0, 0);
    lv_obj_remove_flag(g_grid_container, LV_OBJ_FLAG_SCROLLABLE);
}

static void refresh_grid(void)
{
    int i;
    int start_idx = g_current_page * APPS_PER_PAGE;
    int icon_count = 0;

    /* Remove old icons */
    for (i = 0; i < APPS_PER_PAGE; i++) {
        if (g_app_icons[i]) {
            lv_obj_delete(g_app_icons[i]);
            g_app_icons[i] = NULL;
        }
    }

    /* Page 0: add "Play from SD" music icon as first entry */
    if (g_current_page == 0) {
        g_app_icons[0] = create_music_icon(g_grid_container, 0, 0);
        int x = GRID_LEFT_MARGIN;
        int y = GRID_TOP_MARGIN - STATUS_BAR_HEIGHT;
        lv_obj_set_pos(g_app_icons[0], x, y);
        icon_count = 1;
    }

    /* Create app icons for current page */
    for (i = 0; i < APPS_PER_PAGE && icon_count < APPS_PER_PAGE; i++) {
        int app_idx = start_idx + i;
        if (app_idx >= g_app_count) break;

        int col = icon_count % GRID_COLS;
        int row = icon_count / GRID_COLS;

        g_app_icons[icon_count] = create_app_icon(g_grid_container, &g_apps[app_idx], col, row);

        int x = GRID_LEFT_MARGIN + col * (ICON_SIZE + ICON_SPACING_X);
        int y = GRID_TOP_MARGIN - STATUS_BAR_HEIGHT + row * (ICON_SIZE + ICON_LABEL_HEIGHT + ICON_SPACING_Y);
        lv_obj_set_pos(g_app_icons[icon_count], x, y);
        icon_count++;
    }

    update_page_dots();
}

/****************************************************************************
 * Page Indicators
 ****************************************************************************/

static void create_page_indicators(void)
{
    for (int i = 0; i < 16; i++) {
        g_page_dots[i] = NULL;
    }
}

static void update_page_dots(void)
{
    int i;
    int total_width;

    /* Remove old dots */
    for (i = 0; i < 16; i++) {
        if (g_page_dots[i]) {
            lv_obj_delete(g_page_dots[i]);
            g_page_dots[i] = NULL;
        }
    }

    if (g_total_pages <= 1) return;

    total_width = g_total_pages * (PAGE_DOT_RADIUS * 2) +
                  (g_total_pages - 1) * PAGE_DOT_SPACING;
    int start_x = (DISPLAY_WIDTH - total_width) / 2;

    for (i = 0; i < g_total_pages && i < 16; i++) {
        g_page_dots[i] = lv_obj_create(g_screen);
        lv_obj_set_size(g_page_dots[i], PAGE_DOT_RADIUS * 2, PAGE_DOT_RADIUS * 2);
        lv_obj_set_pos(g_page_dots[i],
                       start_x + i * (PAGE_DOT_RADIUS * 2 + PAGE_DOT_SPACING),
                       PAGE_DOT_Y);

        if (i == g_current_page) {
            lv_obj_add_style(g_page_dots[i], &g_style_page_dot_active, 0);
        } else {
            lv_obj_add_style(g_page_dots[i], &g_style_page_dot, 0);
        }
    }
}

/****************************************************************************
 * Navigation Buttons
 ****************************************************************************/

static void create_nav_buttons(void)
{
    /* Left arrow */
    g_left_arrow = lv_button_create(g_screen);
    lv_obj_add_style(g_left_arrow, &g_style_nav_btn, 0);
    lv_obj_set_size(g_left_arrow, 32, 32);
    lv_obj_set_pos(g_left_arrow, 4, (DISPLAY_HEIGHT - 32) / 2);
    lv_obj_add_event_cb(g_left_arrow, on_nav_prev, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(g_left_arrow);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_center(lbl);

    /* Right arrow */
    g_right_arrow = lv_button_create(g_screen);
    lv_obj_add_style(g_right_arrow, &g_style_nav_btn, 0);
    lv_obj_set_size(g_right_arrow, 32, 32);
    lv_obj_set_pos(g_right_arrow, DISPLAY_WIDTH - 36, (DISPLAY_HEIGHT - 32) / 2);
    lv_obj_add_event_cb(g_right_arrow, on_nav_next, LV_EVENT_CLICKED, NULL);

    lbl = lv_label_create(g_right_arrow);
    lv_label_set_text(lbl, LV_SYMBOL_RIGHT);
    lv_obj_center(lbl);
}

static void update_nav_buttons(void)
{
    if (g_total_pages <= 1) {
        if (g_left_arrow) lv_obj_add_flag(g_left_arrow, LV_OBJ_FLAG_HIDDEN);
        if (g_right_arrow) lv_obj_add_flag(g_right_arrow, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (g_current_page > 0) {
        lv_obj_remove_flag(g_left_arrow, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_left_arrow, LV_OBJ_FLAG_HIDDEN);
    }

    if (g_current_page < g_total_pages - 1) {
        lv_obj_remove_flag(g_right_arrow, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_right_arrow, LV_OBJ_FLAG_HIDDEN);
    }
}

/****************************************************************************
 * Music/SD Card Icon
 ****************************************************************************/

static lv_obj_t *create_music_icon(lv_obj_t *parent, int col, int row)
{
    (void)col; (void)row;
    lv_obj_t *container;
    lv_obj_t *icon_bg;
    lv_obj_t *label;

    container = lv_obj_create(parent);
    lv_obj_set_size(container, ICON_SIZE, ICON_SIZE + ICON_LABEL_HEIGHT);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);

    /* Music icon background - special color */
    icon_bg = lv_obj_create(container);
    lv_obj_add_style(icon_bg, &g_style_icon_bg, 0);
    lv_obj_set_size(icon_bg, ICON_SIZE, ICON_SIZE);
    lv_obj_set_pos(icon_bg, 0, 0);
    lv_obj_set_style_bg_color(icon_bg, lv_color_hex(0xE91E63), 0); /* Pink */
    lv_obj_remove_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);

    /* Music symbol */
    label = lv_label_create(icon_bg);
    lv_label_set_text(label, LV_SYMBOL_AUDIO);
    lv_obj_add_style(label, &g_style_icon_label, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    lv_obj_center(label);

    /* Label below icon */
    label = lv_label_create(container);
    lv_label_set_text(label, "SD Music");
    lv_obj_set_style_text_color(label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_width(label, ICON_SIZE);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(label, 0, ICON_SIZE + 2);

    lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(container, on_music_click, LV_EVENT_CLICKED, NULL);

    return container;
}

static void on_music_click(lv_event_t *e)
{
    (void)e;
    launcher_files_show(g_screen);
}

/****************************************************************************
 * Event Handlers
 ****************************************************************************/

static void on_app_click(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    uintptr_t idx = (uintptr_t)lv_obj_get_user_data(obj);

    if (idx < (uintptr_t)g_app_count) {
        launcher_app_launch(&g_apps[idx]);
    }
}

static void on_nav_prev(lv_event_t *e)
{
    (void)e;
    launcher_ui_goto_page(g_current_page - 1);
}

static void on_nav_next(lv_event_t *e)
{
    (void)e;
    launcher_ui_goto_page(g_current_page + 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lv_obj_t *launcher_ui_create(launcher_app_t *apps, int app_count)
{
    g_apps = apps;
    g_app_count = app_count;
    g_total_pages = launcher_app_page_count(app_count);
    g_current_page = 0;

    /* Background */
    g_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(g_screen, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_scr_load(g_screen);

    /* Initialize styles */
    init_styles();

    /* Build UI components */
    create_status_bar();
    create_grid();
    create_page_indicators();
    create_nav_buttons();

    /* Show first page */
    refresh_grid();
    update_nav_buttons();

    printf("Launcher UI: %d apps, %d pages\n", app_count, g_total_pages);
    return g_screen;
}

void launcher_ui_goto_page(int page)
{
    if (page < 0 || page >= g_total_pages) return;
    if (page == g_current_page) return;

    g_current_page = page;
    refresh_grid();
    update_nav_buttons();
}

int launcher_ui_get_current_page(void)
{
    return g_current_page;
}

void launcher_ui_refresh(void)
{
    /* Update clock */
    static char time_str[16];
    time_t now;
    struct tm *tm_info;

    now = time(NULL);
    tm_info = localtime(&now);

    if (tm_info && g_status_clock) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d",
                 tm_info->tm_hour, tm_info->tm_min);
        lv_label_set_text(g_status_clock, time_str);
    }
}
