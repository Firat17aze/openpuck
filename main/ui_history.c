// The history panel: the changes since the last new game, newest first, in a
// scrolling list with a Done button at the end.
#include "ui_internal.h"

#include <stdio.h>

#include "counter.h"
#include "puck_color.h"

#define LIST_TOP        40   // below the title
#define LIST_WIDTH      170   // the circle is ~178 px wide at the top of the list
#define LIST_PAD_TOP    8
#define LIST_PAD_BOTTOM 50   // lets the Done button scroll into the wide middle
#define ROW_GAP         6
#define ROW_RADIUS      10
#define ROW_PAD         4

#define PLAYER_1_COLOR  lv_color_hex(0x4FC3F7)   // light blue
#define PLAYER_2_COLOR  lv_color_hex(0xFFB74D)   // amber

static lv_obj_t *s_panel;

// Closes History and refreshes the counter; false when it is not open.
bool ui_history_close(void)
{
    if (!s_panel) {
        return false;
    }
    lv_obj_delete_async(s_panel);
    s_panel = NULL;
    ui_counter_refresh(false, lv_tick_get());
    return true;
}

// Done, or a tap anywhere on the panel. LVGL sends CLICKED only for a press
// that did not scroll, so scrolling the list never closes it.
static void done_event(lv_event_t *e)
{
    ui_history_close();
}

static void add_entry(lv_obj_t *list, const puck_history_entry_t *entry, bool two_players)
{
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, ROW_GAP, 0);
    // Not clickable: a tap on a row goes to the list underneath.
    lv_obj_set_clickable(row, false);

    // The row is drawn in the player's colors; on the default black background
    // the change keeps its green/red and the tag its player color.
    const puck_settings_t *settings = counter_settings();
    const uint32_t background = settings->player_background[entry->player];
    const uint32_t text_color = settings->player_text[entry->player];
    const bool plain = background == PUCK_COLOR_BLACK;
    lv_obj_set_style_radius(row, ROW_RADIUS, 0);
    lv_obj_set_style_pad_all(row, ROW_PAD, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(background), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(row, lv_color_hex(text_color), 0);

    char text[16];
    if (two_players) {
        lv_obj_t *player = ui_add_label(row, &lv_font_montserrat_20, entry->player == 0 ? "P1" : "P2");
        if (plain) {
            lv_obj_set_style_text_color(player, entry->player == 0 ? PLAYER_1_COLOR : PLAYER_2_COLOR, 0);
        }
    }
    snprintf(text, sizeof(text), "%+d", entry->delta);
    lv_obj_t *delta = ui_add_label(row, &lv_font_montserrat_28, text);
    if (plain) {
        lv_obj_set_style_text_color(
            delta, lv_palette_main(entry->delta > 0 ? LV_PALETTE_GREEN : LV_PALETTE_RED), 0);
    }
    ui_add_label(row, &lv_font_montserrat_20, LV_SYMBOL_RIGHT);
    snprintf(text, sizeof(text), "%d", entry->total);
    ui_add_label(row, &lv_font_montserrat_28, text);
}

void ui_history_open(void)
{
    if (s_panel) {
        return;
    }
    s_panel = ui_panel_create("History");
    lv_obj_add_event_cb(s_panel, done_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *list = lv_obj_create(s_panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LIST_WIDTH, 240 - LIST_TOP);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, LIST_TOP);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(list, LIST_PAD_TOP, 0);
    lv_obj_set_style_pad_bottom(list, LIST_PAD_BOTTOM, 0);
    lv_obj_set_style_pad_row(list, ROW_GAP, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(list, done_event, LV_EVENT_CLICKED, NULL);

    const puck_history_t *history = counter_history();
    const bool two_players = counter_game()->player_count == 2;
    const uint16_t count = puck_history_count(history);
    if (count == 0) {
        lv_obj_t *empty = ui_add_label(list, &lv_font_montserrat_20, "No changes yet");
        lv_obj_set_style_text_color(empty, lv_palette_main(LV_PALETTE_GREY), 0);
    }
    for (uint16_t i = 0; i < count; i++) {
        add_entry(list, puck_history_get(history, i), two_players);
    }
    ui_add_done_button(list, done_event);
}
