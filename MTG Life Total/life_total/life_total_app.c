#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define INITIAL_LIFE 40
#define LONG_PRESS_THRESHOLD 3000 // 3 seconds in milliseconds
#define BLINK_INTERVAL 500 // 500 ms for blinking

typedef enum {
    SectionTopLeft,
    SectionTopRight,
    SectionBottomLeft,
    SectionBottomRight,
    SectionNone
} Section;

typedef struct {
    int life[4];
    Section highlighted;
    bool editing;
    uint32_t back_pressed_time;
    uint32_t last_blink_time;
    bool blink_state;
} GlobalAppState;

static void draw_life_totals(Canvas* canvas, void* ctx) {
    GlobalAppState* app_state = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    // Draw cross
    canvas_draw_line(canvas, 64, 0, 64, 64);
    canvas_draw_line(canvas, 0, 32, 128, 32);

    // Draw life totals
    char buffer[12];
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(buffer, sizeof(buffer), "%d", app_state->life[SectionTopLeft]);
    canvas_draw_str_aligned(canvas, 32, 16, AlignCenter, AlignCenter, buffer);
    snprintf(buffer, sizeof(buffer), "%d", app_state->life[SectionTopRight]);
    canvas_draw_str_aligned(canvas, 96, 16, AlignCenter, AlignCenter, buffer);
    snprintf(buffer, sizeof(buffer), "%d", app_state->life[SectionBottomLeft]);
    canvas_draw_str_aligned(canvas, 32, 48, AlignCenter, AlignCenter, buffer);
    snprintf(buffer, sizeof(buffer), "%d", app_state->life[SectionBottomRight]);
    canvas_draw_str_aligned(canvas, 96, 48, AlignCenter, AlignCenter, buffer);

    // Highlight selected section
    if (app_state->highlighted != SectionNone) {
        uint8_t x = 0, y = 0;
        uint8_t width = 64, height = 32;
        switch (app_state->highlighted) {
            case SectionTopLeft:
                x = 0;
                y = 0;
                break;
            case SectionTopRight:
                x = 64;
                y = 0;
                break;
            case SectionBottomLeft:
                x = 0;
                y = 32;
                break;
            case SectionBottomRight:
                x = 64;
                y = 32;
                break;
            default:
                break;
        }
        
        if (app_state->editing) {
            // Blinking effect when editing
            uint32_t current_time = furi_get_tick();
            if (current_time - app_state->last_blink_time >= BLINK_INTERVAL) {
                app_state->blink_state = !app_state->blink_state;
                app_state->last_blink_time = current_time;
            }
            
            if (app_state->blink_state) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_frame(canvas, x, y, width, height);
            }
        } else {
            // Solid black border when selecting
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_frame(canvas, x, y, width, height);
        }
    }
}

static void input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static bool handle_user_input(InputEvent* event, GlobalAppState* app_state) {
    if (event->type == InputTypePress && event->key == InputKeyBack) {
        app_state->back_pressed_time = furi_get_tick();
    } else if (event->type == InputTypeRelease && event->key == InputKeyBack) {
        uint32_t press_duration = furi_get_tick() - app_state->back_pressed_time;
        if (press_duration >= LONG_PRESS_THRESHOLD) {
            return false; // Exit the app only on long press
        } else {
            // Short press behavior
            if (app_state->editing) {
                app_state->editing = false;
            } else if (app_state->highlighted != SectionNone) {
                app_state->highlighted = SectionNone;
            }
        }
    } else if (event->type == InputTypeShort) {
        // Handle other short press events
        switch (event->key) {
            case InputKeyOk:
                // ... (existing OK button logic)
                break;
            case InputKeyUp:
                // ... (existing Up button logic)
                break;
            case InputKeyDown:
                // ... (existing Down button logic)
                break;
            case InputKeyLeft:
                // ... (existing Left button logic)
                break;
            case InputKeyRight:
                // ... (existing Right button logic)
                break;
            default:
                break;
        }
    }
    return true; // Continue running the app in all other cases
}

int32_t life_total_app(void* p) {
    UNUSED(p);
    GlobalAppState app_state = {
        .life = {INITIAL_LIFE, INITIAL_LIFE, INITIAL_LIFE, INITIAL_LIFE},
        .highlighted = SectionNone,
        .editing = false,
        .back_pressed_time = 0,
        .last_blink_time = 0,
        .blink_state = false,
    };

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_life_totals, &app_state);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;
    while (running) {
        if (furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            running = handle_user_input(&event, &app_state);
        }
        view_port_update(view_port);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close(RECORD_GUI);

    return 0;
}