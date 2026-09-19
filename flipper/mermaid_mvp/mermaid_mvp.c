#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

static void mermaid_draw(Canvas* canvas, void* context) {
    UNUSED(context);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 13, "918 Mermaid MVP");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 30, "Provisioner online");
    canvas_draw_str(canvas, 4, 42, "CYD + S3-N16R8");
    canvas_draw_str(canvas, 4, 54, "BACK: exit");
}

static void mermaid_input(InputEvent* event, void* context) {
    furi_assert(context);
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, event, 0);
}

int32_t mermaid_mvp_app(void* p) {
    UNUSED(p);

    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* viewport = view_port_alloc();
    view_port_draw_callback_set(viewport, mermaid_draw, NULL);
    view_port_input_callback_set(viewport, mermaid_input, queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, viewport, GuiLayerFullscreen);

    bool running = true;
    InputEvent event;
    while(running) {
        if(furi_message_queue_get(queue, &event, FuriWaitForever) != FuriStatusOk) {
            continue;
        }
        if(event.type == InputTypeShort && event.key == InputKeyBack) {
            running = false;
        }
    }

    gui_remove_view_port(gui, viewport);
    furi_record_close(RECORD_GUI);
    view_port_free(viewport);
    furi_message_queue_free(queue);
    return 0;
}
