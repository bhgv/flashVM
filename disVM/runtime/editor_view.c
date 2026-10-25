/* FlashVM TextField presentation; document/history/file policy lives in Limbo. */
#include "editor_view.h"
#include "flashvm/standard_text.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/path.h"
#include "flashvm/utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DVM_EDITOR_SDL
#include <SDL.h>
#endif

#define EDITOR_WIDTH 960U              /* Fixed initial viewport width in pixels. */
#define EDITOR_HEIGHT 640U             /* Fixed initial viewport height in pixels. */
#define EDITOR_SOURCE_MAX 4194304U     /* Maximum assembled UTF-8 document snapshot. */
#define EDITOR_VIEW_BYTES 4096U        /* Bounded first-page TextField preview. */
#define EDITOR_REPLY_META 56U          /* Seven canonical eight-character hexadecimal words. */
#define EDITOR_SELECTION 65536UL       /* Dis selection notification code. */
#define EDITOR_QUIT 65539UL            /* Dis controller's dirty-checked close operation. */

static const char *font_path;          /* Borrowed explicit TTF/OpenType command-line path. */
static const char *document_path = ""; /* Borrowed configured source/destination, never guessed. */
static const char *event_path;         /* Optional deterministic counted-envelope input. */
static const char *capture_path;       /* Optional final PPM capture destination. */
static int new_document;               /* Explicit --new selects a destination without opening it. */
static FILE *events;                   /* Owned replay stream; no deployed Python dependency. */
static FVM_Runtime *runtime;           /* Owner-local FlashVM runtime. */
static FVM_Context *context;           /* Owner-local context; no pointer enters Dis. */
static FVM_Font *layout_font;          /* Owned same-face layout for Dis selection geometry. */
static long caret;                     /* Last Dis Unicode scalar caret, presentation only. */
static long anchor;                    /* Last Dis Unicode scalar selection anchor. */
static FVM_Object *field;              /* [GC:] Pinned display object until destroy. */
static unsigned char *pixels;          /* Owned RGBA8 raster storage. */
static char *snapshot;                 /* Owned assembly storage, independent of both VM heaps. */
static size_t assembled;               /* Next expected UTF-8 byte offset. */
static size_t extent;                  /* Total expected snapshot bytes. */
static unsigned long revision;         /* Dis revision of the pending/published snapshot. */
static unsigned long status;           /* Dis status published with that snapshot. */
static long saved;                     /* Dis saved revision, used only for display. */
static int complete;                   /* A selection frame completed the current view. */
static int window_replay;              /* Explicit SDL upload check with deterministic input. */
static int assembling;                 /* A document snapshot is being assembled. */
#ifdef DVM_EDITOR_SDL
static SDL_Window *window;             /* Main-thread-only native window. */
static SDL_Renderer *renderer;         /* Main-thread presentation owner. */
static SDL_Texture *texture;           /* RGBA8 upload target. */
#endif


/* Parse a bounded canonical metadata word; text is eight borrowed ASCII bytes.
 * Returns one with out set, or zero without publishing an invalid value. */
static int hexword(const char *text, unsigned long *out)
{
    unsigned int index;
    unsigned long value = 0;
    for (index = 0; index < 8; index++)
    {
        unsigned int digit = (unsigned char)text[index];
        if (digit >= '0' && digit <= '9')
        {
            digit -= '0';
        }
        else if (digit >= 'a' && digit <= 'f')
        {
            digit = digit - 'a' + 10;
        }
        else
        {
            return 0;
        }
        value = (value << 4) | digit;
    }
    *out = value;
    return 1;
}


/* Store a known CLI option without copying argv; returns whether it was recognized. */
int dvm_editor_option(const char *name, const char *value)
{
    if (!strcmp(name, "--font"))
    {
        font_path = value;
    }
    else if (!strcmp(name, "--document"))
    {
        document_path = value;
        new_document = 0;
    }
    else if (!strcmp(name, "--new"))
    {
        document_path = value;
        new_document = 1;
    }
    else if (!strcmp(name, "--events"))
    {
        event_path = value;
    }
    else if (!strcmp(name, "--window-events"))
    {
        event_path = value;
        window_replay = 1;
    }
    else if (!strcmp(name, "--capture"))
    {
        capture_path = value;
    }
    else
    {
        return 0;
    }
    return 1;
}


/* Expose immutable configuration to the Dis builtin; no I/O or heap pointer escapes. */
const char *dvm_editor_path(void)
{
    return document_path;
}


/* Return copied new-document configuration; no file or VM operation occurs. */
int dvm_editor_fresh(void)
{
    return new_document;
}


/* Load an explicitly selected bounded font and create a pinned uniform TextField.
 * Returns one on success, zero on any actual backend/SDK/file failure. */
int dvm_editor_create(void)
{
    FILE *font;
    long size;
    void *bytes;
    FVM_Config config;
    FVM_Value value;
    FVM_Result result;
    if (!font_path)
    {
        fprintf(stderr, "editor requires --font PATH (an explicit TTF/OpenType font)\n");
        return 0;
    }
    font = fopen(font_path, "rb");
    if (!font)
    {
        perror("editor font");
        return 0;
    }
    if (fseek(font, 0, SEEK_END) || (size = ftell(font)) <= 0 || size > 16777216L || fseek(font, 0, SEEK_SET))
    {
        fclose(font);
        return 0;
    }
    bytes = malloc((size_t)size);
    if (!bytes || fread(bytes, 1, (size_t)size, font) != (size_t)size)
    {
        free(bytes);
        fclose(font);
        return 0;
    }
    fclose(font);
    result = fvm_config_init(&config);
    if (result == FVM_OK)
    {
        result = fvm_runtime_create(&config, &runtime);
    }
    if (result == FVM_OK)
    {
        result = fvm_context_create(runtime, &context);
    }
    if (result == FVM_OK)
    {
        result = fvm_standard_text_install(context);
    }
    if (result == FVM_OK)
    {
        result = fvm_standard_text_register_font(context, "Editor", bytes, (size_t)size, 0);
    }
    if (result == FVM_OK)
    {
        result = fvm_font_read(bytes, (size_t)size, 0, 18, &layout_font);
    }
    free(bytes);
    if (result != FVM_OK || fvm_textfield_create(context, "Editor", EDITOR_WIDTH, EDITOR_HEIGHT, &field) != FVM_OK ||
        fvm_gc_pin(context, field) != FVM_OK)
    {
        fprintf(stderr, "editor FlashVM/font initialization failed\n");
        return 0;
    }
    {
        FVM_Object *format = 0;
        FVM_Value argument;
        FVM_Value ignored;
        if (fvm_construct(context, "TextFormat", 0, 0, &format) != FVM_OK || fvm_gc_pin(context, format) != FVM_OK)
        {
            return 0;
        }
        fvm_value_number(&value, 18);
        result = fvm_object_set(context, format, "size", &value);
        fvm_value_undefined(&argument);
        argument.type = FVM_VALUE_OBJECT;
        argument.object = format;
        if (result == FVM_OK)
        {
            result = fvm_object_call(context, field, "setNewTextFormat", 1, &argument, &ignored);
        }
        fvm_gc_unpin(context, format);
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    fvm_value_boolean(&value, 1);
    if (fvm_object_set(context, field, "multiline", &value) != FVM_OK ||
        fvm_object_set(context, field, "wordWrap", &value) != FVM_OK)
    {
        return 0;
    }
    pixels = malloc(EDITOR_WIDTH * EDITOR_HEIGHT * 4U);
    snapshot = malloc(EDITOR_SOURCE_MAX + 1U);
    if (!pixels || !snapshot)
    {
        return 0;
    }
    if (event_path)
    {
        events = fopen(event_path, "rb");
        if (!events)
        {
            return 0;
        }
    }
    if (!event_path || window_replay)
    {
#ifdef DVM_EDITOR_SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        {
            fprintf(stderr, "SDL: %s\n", SDL_GetError());
            return 0;
        }
        window = SDL_CreateWindow(
            "FlashVM / Dis IDE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, EDITOR_WIDTH, EDITOR_HEIGHT, 0);
        renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : 0;
        texture = renderer
                      ? SDL_CreateTexture(
                            renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, EDITOR_WIDTH, EDITOR_HEIGHT)
                      : 0;
        if (!texture)
        {
            fprintf(stderr, "SDL window: %s\n", SDL_GetError());
            return 0;
        }
        SDL_StartTextInput();
#else
        fprintf(stderr, "window support unavailable: build with DVM_EDITOR_SDL or provide --events\n");
        return 0;
#endif
    }
    return 1;
}


/* Draw a clipped rectangle through FlashVM's raster backend; no document mutation.
 * Coordinates are pixels, color is copied. Returns one on actual raster success. */
static int rectangle(FVM_RasterSurface *surface, double x, double y, double width, double height, FVM_RasterColor color)
{
    FVM_RasterPoint points[4];
    points[0].x = x;
    points[0].y = y;
    points[1].x = x + width;
    points[1].y = y;
    points[2].x = x + width;
    points[2].y = y + height;
    points[3].x = x;
    points[3].y = y + height;
    return fvm_raster_fill_polygon(surface, points, 4, color) == FVM_OK;
}


/* Map borrowed valid UTF-8 scalar positions to bounded preview byte offsets.
 * Offscreen positions return shown+1; bytes and document authority stay unchanged. */
static size_t scalar_byte(size_t shown, long position)
{
    size_t byte = 0;
    long scalar = 0;
    while (byte < shown && scalar < position)
    {
        byte++;
        while (byte < shown && ((unsigned char)snapshot[byte] & 0xc0U) == 0x80U)
        {
            byte++;
        }
        scalar++;
    }
    return scalar == position ? byte : shown + 1;
}


/* Paint acknowledged caret/selection using the same face/size and FlashVM layout.
 * surface is borrowed; shown bounds the preview. Returns zero on layout/raster failure.
 * Combining/bidi clusters highlight as glyph clusters; no native editing policy is inferred. */
static int selection_geometry(FVM_RasterSurface *surface, size_t shown)
{
    FVM_TextLayoutOptions options;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_RasterColor selected = {204, 222, 255, 255};
    FVM_RasterColor cursor = {35, 85, 170, 255};
    size_t first = scalar_byte(shown, caret < anchor ? caret : anchor);
    size_t last = scalar_byte(shown, caret > anchor ? caret : anchor);
    size_t current = scalar_byte(shown, caret);
    size_t index;
    int ok = 1;
    memset(&options, 0, sizeof(options));
    options.max_width = EDITOR_WIDTH - 4;
    options.alignment = FVM_TEXT_ALIGN_LEFT;
    if (fvm_text_layout_create(layout_font, snapshot, shown, &options, &layout) != FVM_OK ||
        fvm_text_layout_metrics(layout, &metrics) != FVM_OK || fvm_font_metrics(layout_font, &font) != FVM_OK)
    {
        fvm_text_layout_destroy(layout);
        return 0;
    }
    for (index = 0; index < metrics.glyph_count && ok; index++)
    {
        FVM_TextLayoutGlyph glyph;
        if (fvm_text_layout_glyph(layout, index, &glyph) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (glyph.shaped.cluster >= first && glyph.shaped.cluster < last)
        {
            ok = rectangle(
                surface, glyph.x + 2, glyph.y - font.ascent + 2, glyph.shaped.x_advance, font.line_height, selected);
        }
    }
    for (index = 0; index < metrics.line_count && current <= shown && ok; index++)
    {
        FVM_TextLayoutLine line;
        size_t glyph_index;
        double x;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (current < line.byte_start || (current >= line.byte_next && index + 1 < metrics.line_count))
        {
            continue;
        }
        x = line.x + line.advance;
        for (glyph_index = 0; glyph_index < line.glyph_count; glyph_index++)
        {
            FVM_TextLayoutGlyph glyph;
            if (fvm_text_layout_glyph(layout, line.glyph_start + glyph_index, &glyph) != FVM_OK)
            {
                ok = 0;
                break;
            }
            if (glyph.shaped.cluster >= current)
            {
                x = glyph.x;
                break;
            }
        }
        if (ok)
        {
            ok = rectangle(surface, x + 2, line.baseline - font.ascent + 2, 1, font.line_height, cursor);
        }
        break;
    }
    fvm_text_layout_destroy(layout);
    return ok;
}


/* Render only a complete document through actual FlashVM contours and rasterization.
 * No parameters; returns one on success, zero on backend/file/presentation failure. */
static int render(void)
{
    FVM_Value value;
    FVM_Path *path = 0;
    FVM_RasterSurface surface;
    FVM_RasterColor ink = {32, 36, 48, 255};
    FVM_Result result;
    size_t shown = extent < EDITOR_VIEW_BYTES ? extent : EDITOR_VIEW_BYTES;
    while (shown < extent && ((unsigned char)snapshot[shown] & 0xc0U) == 0x80U)
    {
        shown--;
    }
    if (fvm_value_string_bytes(&value, snapshot, shown) != FVM_OK ||
        fvm_object_set(context, field, "text", &value) != FVM_OK)
    {
        return 0;
    }
    memset(pixels, 255, EDITOR_WIDTH * EDITOR_HEIGHT * 4U);
    surface.pixels = pixels;
    surface.width = EDITOR_WIDTH;
    surface.height = EDITOR_HEIGHT;
    surface.stride = EDITOR_WIDTH * 4U;
    if (!selection_geometry(&surface, shown))
    {
        return 0;
    }
    result = fvm_textfield_path(context, field, &path);
    if (result == FVM_OK)
    {
        result = fvm_path_draw(path, &surface, ink, 0, 0);
    }
    fvm_path_destroy(path);
    if (result != FVM_OK)
    {
        fprintf(stderr, "editor TextField raster failed: %d\n", result);
        return 0;
    }
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        char title[256];
        snprintf(title,
                 sizeof(title),
                 "Dis IDE | rev %lu saved %ld status %lu | Ctrl+S save, Ctrl+O reload%s",
                 revision,
                 saved,
                 status,
                 extent > shown ? " | first page" : "");
        SDL_SetWindowTitle(window, title);
        if (SDL_UpdateTexture(texture, 0, pixels, EDITOR_WIDTH * 4U) || SDL_RenderClear(renderer) ||
            SDL_RenderCopy(renderer, texture, 0, 0))
        {
            return 0;
        }
        SDL_RenderPresent(renderer);
    }
#endif
    if (capture_path)
    {
        FILE *capture = fopen(capture_path, "wb");
        size_t index;
        int failed = 0;
        if (!capture)
        {
            return 0;
        }
        if (fprintf(capture, "P6\n%u %u\n255\n", EDITOR_WIDTH, EDITOR_HEIGHT) < 0)
        {
            failed = 1;
        }
        for (index = 0; index < EDITOR_WIDTH * EDITOR_HEIGHT && !failed; index++)
        {
            if (fwrite(pixels + index * 4, 1, 3, capture) != 3)
            {
                failed = 1;
            }
        }
        if (fclose(capture))
        {
            failed = 1;
        }
        if (failed)
        {
            return 0;
        }
    }
    return fvm_gc_collect(context, 0) == FVM_OK;
}


/* Assemble ordered Dis snapshot fragments, publishing TextField only at selection boundary.
 * message is borrowed, never retained. Returns zero on malformed/stale/inconsistent framing. */
int dvm_editor_present(const DVM_UIMessage *message)
{
    unsigned long offset;
    unsigned long total;
    if (message->kind != DVM_UI_VIEW)
    {
        return 0;
    }
    if (message->code == EDITOR_SELECTION)
    {
        if (!assembling || assembled != extent || message->flags != revision || message->first < 0 ||
            message->second < 0)
        {
            return 0;
        }
        size_t scalars;
        snapshot[extent] = 0;
        if (fvm_utf8_validate(snapshot, extent, &scalars) != FVM_UTF8_OK || (unsigned long)message->first > scalars ||
            (unsigned long)message->second > scalars)
        {
            return 0;
        }
        caret = message->first;
        anchor = message->second;
        if (!render())
        {
            return 0;
        }
        complete = 1;
        assembling = 0;
        if (events)
        {
            size_t index;
            printf("VIEW %lu %lu %ld %ld %ld ", revision, status, saved, message->first, message->second);
            for (index = 0; index < extent; index++)
            {
                printf("%02x", (unsigned char)snapshot[index]);
            }
            putchar('\n');
        }
        return 1;
    }
    if (message->size < EDITOR_REPLY_META || !hexword(message->text + 40, &offset) ||
        !hexword(message->text + 48, &total) || total > EDITOR_SOURCE_MAX || offset > total ||
        message->size - EDITOR_REPLY_META > total - offset)
    {
        return 0;
    }
    if (!assembling)
    {
        if (offset != 0)
        {
            return 0;
        }
        revision = message->flags;
        status = message->code;
        saved = message->second;
        extent = total;
        assembled = 0;
        assembling = 1;
    }
    if (message->flags != revision || message->code != status || message->second != saved || total != extent ||
        offset != assembled)
    {
        return 0;
    }
    memcpy(snapshot + offset, message->text + EDITOR_REPLY_META, message->size - EDITOR_REPLY_META);
    assembled += message->size - EDITOR_REPLY_META;
    return 1;
}


#ifdef DVM_EDITOR_SDL
/* Translate SDL virtual keys; zero means no supported Dis key, never synthesize text. */
static unsigned int keycode(SDL_Keycode key)
{
    if (key >= SDLK_a && key <= SDLK_z)
    {
        return (unsigned int)(key - SDLK_a + 'A');
    }
    switch (key)
    {
    case SDLK_BACKSPACE:
        return FVM_KEY_BACKSPACE;
    case SDLK_DELETE:
        return FVM_KEY_DELETE;
    case SDLK_LEFT:
        return FVM_KEY_LEFT;
    case SDLK_RIGHT:
        return FVM_KEY_RIGHT;
    case SDLK_UP:
        return FVM_KEY_UP;
    case SDLK_DOWN:
        return FVM_KEY_DOWN;
    case SDLK_PAGEUP:
        return FVM_KEY_PGUP;
    case SDLK_PAGEDOWN:
        return FVM_KEY_PGDN;
    case SDLK_HOME:
        return FVM_KEY_HOME;
    case SDLK_END:
        return FVM_KEY_END;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return FVM_KEY_CONTROL;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return FVM_KEY_SHIFT;
    default:
        return 0;
    }
}
#endif


/* Poll at most one event after the previous Dis view completes; never wait or edit text.
 * message receives a value only on one. Replay truncation/EOF without Quit returns minus one. */
int dvm_editor_event(DVM_UIMessage *message)
{
    if (!complete)
    {
        return 0;
    }
    memset(message, 0, sizeof(*message));
    if (events)
    {
        unsigned char length[4];
        unsigned long count;
        char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
        FVM_Value value;
        if (fread(length, 1, 4, events) != 4)
        {
            fprintf(stderr, "editor replay ended before acknowledged Quit\n");
            return -1;
        }
        count = length[0] | ((unsigned long)length[1] << 8) | ((unsigned long)length[2] << 16) |
                ((unsigned long)length[3] << 24);
        if (count < DVM_UI_HEADER_SIZE || count > sizeof(storage) || fread(storage, 1, count, events) != count ||
            fvm_value_string_bytes(&value, storage, count) != FVM_OK || dvm_ui_decode(&value, message) != FVM_OK)
        {
            return -1;
        }
        complete = 0;
        return 1;
    }
#ifdef DVM_EDITOR_SDL
    {
        SDL_Event event;
        if (!SDL_PollEvent(&event))
        {
            return 0;
        }
        if (event.type == SDL_QUIT)
        {
            message->kind = DVM_UI_REQUEST;
            message->id = 1;
            message->code = EDITOR_QUIT;
            message->flags = (SDL_GetModState() & KMOD_SHIFT) != 0;
        }
        else if (event.type == SDL_TEXTINPUT)
        {
            message->kind = DVM_UI_TEXT;
            message->size = strlen(event.text.text);
            memcpy(message->text, event.text.text, message->size);
        }
        else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            unsigned int code = keycode(event.key.keysym.sym);
            if (!code && event.type == SDL_KEYDOWN &&
                (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_TAB))
            {
                message->kind = DVM_UI_TEXT;
                message->size = 1;
                message->text[0] = event.key.keysym.sym == SDLK_RETURN ? '\n' : '\t';
            }
            else if (code)
            {
                FVM_KeyboardState state;
                memset(&state, 0, sizeof(state));
                state.struct_size = sizeof(state);
                if (fvm_input_get_keyboard(context, &state) != FVM_OK)
                {
                    return -1;
                }
                state.down[code] = event.type == SDL_KEYDOWN;
                state.last_code = code;
                state.last_ascii = 0;
                if (fvm_input_set_keyboard(context, &state) != FVM_OK ||
                    dvm_ui_capture_keyboard(context, code, event.type == SDL_KEYDOWN, message) != FVM_OK)
                {
                    return -1;
                }
            }
            else
            {
                return 0;
            }
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            if (fvm_input_reset_keyboard(context) != FVM_OK)
            {
                return -1;
            }
            message->kind = DVM_UI_FOCUS;
        }
        else
        {
            return 0;
        }
        complete = 0;
        return 1;
    }
#else
    return -1;
#endif
}


/* Tear down only after scheduler/channel routing is quiescent; null resources are allowed. */
void dvm_editor_destroy(void)
{
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
#endif
    if (events)
    {
        fclose(events);
    }
    if (field && context)
    {
        fvm_gc_unpin(context, field);
    }
    fvm_font_destroy(layout_font);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    free(snapshot);
    free(pixels);
}
