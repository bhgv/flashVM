/* Compare reflowed MDI capture with a fresh identical scene, through real VM objects. */
#include "desktop_view.h"
#include "flashvm/standard.h"
#include "flashvm/standard_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 960U /* Fixed replay viewport width. */
#define HEIGHT 640U /* Fixed replay viewport height. */


/* Send one complete copied descriptor/envelope; text is borrowed only until return. */
static int send(unsigned long code, long first, long second, const char *text)
{
    DVM_UIMessage message;
    memset(&message, 0, sizeof(message));
    message.kind = DVM_UI_VIEW;
    message.code = code;
    message.flags = 7;
    message.first = first;
    message.second = second;
    message.size = strlen(text);
    memcpy(message.text, text, message.size);
    return dvm_desktop_message(&message);
}


/* Stage a root and either two initial children or the surviving reflowed child.
 * Root precedes children. No document policy or native pointer enters the wire. */
static int scene(int initial)
{
    if (!send(DVM_DESK_BEGIN, WIDTH, HEIGHT, initial ? "00000007000000030000000000000064" : "00000007000000020000000000000064") ||
        !send(DVM_DESK_ITEM, 13, 0, "000000010000006000000048000002e80000013400000004ffffffff00000000ffffffffSource MDI"))
    {
        return 0;
    }
    if (initial && !send(DVM_DESK_ITEM, 1, 0, "00000005000000080000002800000168000000fc00000000ffffffff00000000ffffffffleft"))
    {
        return 0;
    }
    return send(DVM_DESK_ITEM, 2, 0, initial ?
        "000000050000017c0000002800000168000000fc00000000ffffffff00000000ffffffffsurvivor" :
        "00000005000000080000002800000168000000fc00000000ffffffff00000000ffffffffsurvivor") &&
        send(DVM_DESK_END, 0, 0, "");
}


/* Capture/raster real native contours and text. output receives an owned pixel
 * copy while all managed state remains on the context owner. Errors return0. */
static int pixels(unsigned char *output)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(WIDTH, HEIGHT);
    int ok = frame && dvm_desktop_prepare(frame) && dvm_editor_frame_incremental(frame, NULL) &&
             dvm_editor_frame_render(frame) == FVM_OK;
    if (ok)
    {
        memcpy(output, dvm_editor_frame_surface(frame)->pixels, WIDTH * HEIGHT * 4U);
    }
    dvm_editor_frame_destroy(frame);
    return ok;
}


/* Own one context through a fresh/reflow sequence; font bytes stay readable.
 * No frame advances, tasks or mock display lists are used. Destroy before return. */
static int render(const unsigned char *font, size_t size, int reflow, unsigned char *output)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    int ok = fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
             fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK &&
             fvm_standard_text_install(context) == FVM_OK &&
             fvm_standard_text_register_font(context, "Editor", font, size, 0) == FVM_OK &&
             dvm_desktop_bind(context, font, size, 0);
    if (ok && reflow)
    {
        ok = scene(1) && pixels(output);
    }
    if (ok)
    {
        ok = scene(0) && pixels(output);
    }
    dvm_desktop_destroy();
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return ok;
}


/* Load an explicit font and compare every RGBA byte after removing/retiling a
 * child. Previously retained child contours must not be captured a second time. */
int main(int argc, char **argv)
{
    FILE *file;
    long size;
    unsigned char *font;
    unsigned char *reflow;
    unsigned char *fresh;
    int ok;
    if (argc != 2 || !(file = fopen(argv[1], "rb")))
    {
        return 2;
    }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET))
    {
        fclose(file);
        return 2;
    }
    font = malloc((size_t)size);
    reflow = malloc(WIDTH * HEIGHT * 4U);
    fresh = malloc(WIDTH * HEIGHT * 4U);
    ok = font && reflow && fresh && fread(font, 1, (size_t)size, file) == (size_t)size;
    fclose(file);
    if (ok)
    {
        ok = render(font, (size_t)size, 1, reflow) && render(font, (size_t)size, 0, fresh) &&
             memcmp(reflow, fresh, WIDTH * HEIGHT * 4U) == 0;
    }
    free(fresh);
    free(reflow);
    free(font);
    if (!ok)
    {
        fputs("FAIL MDI reflow capture differs from fresh scene; retained child ghost or duplicate capture\n", stderr);
        return 1;
    }
    puts("PASS actual MDI reflow capture equals fresh scene byte-for-byte, no retained child ghosts");
    return 0;
}
