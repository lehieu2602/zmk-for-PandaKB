/*
 * Show "CAPS" on the peripheral nice_oled screen while Caps Lock is on.
 *
 * nice_oled draws the peripheral screen on a portrait canvas and rotates it
 * 90 degrees onto the 128x32 display. Its peripheral layout leaves the
 * left-most 18px of the display (bottom of the portrait view) empty, so we
 * overlay a small canvas there, drawn and rotated the same way.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators_types.h>

LV_FONT_DECLARE(pixel_operator_mono_12);

#define LED_CLCK 0x02

// Square buffer so the 90 degree rotation fits, matching nice_oled's rotate_canvas()
#define CAPS_CANVAS_SIZE 32
// Only the left 18px of the display are free; shift the canvas so its free columns line up
#define CAPS_CANVAS_X -14

#define CAPS_BACKGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_black() : lv_color_white()
#define CAPS_FOREGROUND                                                                            \
    IS_ENABLED(CONFIG_NICE_OLED_WIDGET_INVERTED) ? lv_color_white() : lv_color_black()

static lv_obj_t *caps_canvas;
static lv_color_t caps_cbuf[CAPS_CANVAS_SIZE * CAPS_CANVAS_SIZE];
static lv_color_t caps_cbuf_tmp[CAPS_CANVAS_SIZE * CAPS_CANVAS_SIZE];

struct caps_state {
    bool caps_lock;
};

static void draw_caps(struct caps_state state) {
    if (caps_canvas == NULL) {
        // Created lazily: the status screen only exists once the display is initialized
        caps_canvas = lv_canvas_create(lv_scr_act());
        lv_canvas_set_buffer(caps_canvas, caps_cbuf, CAPS_CANVAS_SIZE, CAPS_CANVAS_SIZE,
                             LV_IMG_CF_TRUE_COLOR);
        lv_obj_align(caps_canvas, LV_ALIGN_TOP_LEFT, CAPS_CANVAS_X, 0);
    }

    // Draw in portrait orientation, like the rest of the nice_oled peripheral screen
    lv_canvas_fill_bg(caps_canvas, CAPS_BACKGROUND, LV_OPA_COVER);
    if (state.caps_lock) {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = CAPS_FOREGROUND;
        label_dsc.font = &pixel_operator_mono_12;
        label_dsc.align = LV_TEXT_ALIGN_CENTER;
        lv_canvas_draw_text(caps_canvas, 0, 2, CAPS_CANVAS_SIZE, &label_dsc, "CAPS");
    }

    // Rotate onto the horizontal display
    memcpy(caps_cbuf_tmp, caps_cbuf, sizeof(caps_cbuf_tmp));
    lv_img_dsc_t img = {
        .header.cf = LV_IMG_CF_TRUE_COLOR,
        .header.w = CAPS_CANVAS_SIZE,
        .header.h = CAPS_CANVAS_SIZE,
        .data = (void *)caps_cbuf_tmp,
    };
    lv_canvas_fill_bg(caps_canvas, CAPS_BACKGROUND, LV_OPA_COVER);
    lv_canvas_transform(caps_canvas, &img, 900, LV_IMG_ZOOM_NONE, -1, 0, CAPS_CANVAS_SIZE / 2,
                        CAPS_CANVAS_SIZE / 2, false);
}

static struct caps_state caps_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
    return (struct caps_state){
        .caps_lock = (ev != NULL) && (ev->indicators & LED_CLCK),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_caps, struct caps_state, draw_caps, caps_get_state)
ZMK_SUBSCRIPTION(widget_peripheral_caps, zmk_hid_indicators_changed);
