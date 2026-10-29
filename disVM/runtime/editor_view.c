/* FlashVM TextField presentation; document/history/file policy lives in Limbo. */
#include "editor_view.h"
#include "flashvm/avm1.h"
#include "ide_view_swf.h"
#include "flashvm/standard_text.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/path.h"
#include "flashvm/render_queue.h"
#include "editor_raster.h"
#include "desktop_view.h"
#include "flashvm/utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef DVM_EDITOR_SDL
#include <SDL.h>
#include "editor_window_events.h"
#endif

#define EDITOR_WIDTH 960U                /* Initial viewport width in pixels. */
#define EDITOR_HEIGHT 640U               /* Initial viewport height in pixels. */
#define EDITOR_SOURCE_MAX 4194304U       /* Maximum assembled UTF-8 document snapshot. */
#define EDITOR_VIEW_BYTES 4096U          /* Bounded first-page TextField preview. */
#define EDITOR_REPLY_META 56U            /* Seven canonical eight-character hexadecimal words. */
#define EDITOR_SELECTION 65536UL         /* Dis selection notification code. */
#define EDITOR_ZOOM 65547UL              /* Dis-admitted font-size step proposal. */
#define EDITOR_STYLE 65548UL             /* Complete-view font-size and row geometry. */
#define EDITOR_PINCH 65550UL             /* Normalized two-finger pinch input code. */
#define EDITOR_LEADING 2.0               /* Explicit TextFormat leading shared by all editor geometry. */
#define EDITOR_FONT_MIN 6                /* Bounded minimum editor pixel size. */
#define EDITOR_FONT_MAX 48               /* Bounded maximum editor pixel size. */
#define EDITOR_PINCH_THRESHOLD 0.015     /* Normalized SDL distance change per admitted zoom step. */
#define EDITOR_PANEL_BROWSER 9U          /* Read-only selected directory-entry presentation. */
#define EDITOR_PANEL_EXPORT_PATH 11U /* Editable explicit artifact destination. */
#define EDITOR_PANEL_BROWSE_PATH 10U     /* Editable directory path, with caret and normal field geometry. */
#define EDITOR_PANEL 65543UL             /* Dis-owned prompt mode, selection and counted field text. */
#define EDITOR_VIEWPORT 65540UL          /* Dis-owned Unicode window and monotonic view ticket. */
#define EDITOR_SCROLL 65541UL            /* Ticket/revision-bound logical scroll request. */
#define EDITOR_PLACE 65542UL             /* Ticket/revision-bound glyph hit proposal. */
#define EDITOR_RESIZE 65552UL            /* Raw/checked width-height proposal, bound to the completed view. */
#define EDITOR_CANVAS 65553UL            /* Dis-admitted pixel extent before Style and snapshot frames. */
#define EDITOR_WIDTH_MIN 240U            /* Bounded minimum native surface width. */
#define EDITOR_HEIGHT_MIN 160U           /* Bounded minimum native surface height. */
#define EDITOR_EXTENT_MAX 2048U          /* Bound each dimension and the RGBA allocation to 16 MiB. */
#define EDITOR_GUTTER 65563UL            /* Dis-owned line-number visibility, copied per completed view. */
#define EDITOR_QUIT 65539UL              /* Dis controller's dirty-checked close operation. */

#define EDITOR_CLIP_FIELD_WRITE 65561UL  /* Copy Dis-authorized modal field bytes, excluding the label. */
#define EDITOR_FIELD_PLACE 65562UL       /* Checked modal scalar placement/drag proposal from native glyphs. */
#define EDITOR_PANEL_SELECTION 65560UL   /* Dis-owned modal caret/anchor, independent of the source selection. */
#define EDITOR_CLIP_WRITE 65554UL        /* Dis-authorized source selection copied to native clipboard. */
#define EDITOR_CLIP_READ 65556UL         /* Dis request for one immutable native clipboard snapshot. */
#define EDITOR_CLIP_DATA 65557UL         /* Revision/transfer-bound ordered paste fragment. */
#define EDITOR_CLIP_CONTINUE 65559UL     /* One fragment credit without retransmitting an unchanged source view. */
#define EDITOR_CLIP_ACK 65558UL          /* Acknowledged clipboard write or explicit read failure. */
#define EDITOR_CLIP_MAX 65536U           /* Bound each clipboard snapshot and staged paste in UTF-8 bytes. */
#define EDITOR_CLIP_OK 0                 /* Successful copied-value result, matching Idecore. */
#define EDITOR_CLIP_TOO_LARGE 3          /* UTF-8 clipboard bound rejected before any source edit. */
#define EDITOR_CLIP_INVALID 4            /* Native API, allocation, NUL or UTF-8 admission failure. */
#define EDITOR_CLIP_PREFIX 8U            /* Original completed-view ticket before each Unicode body. */

static char *clipboard_text;             /* Owned replay clipboard value; also retains successful native writes. */
static char *clipboard_transfer;         /* Owned immutable UTF-8 read snapshot until its last copied fragment. */
static size_t clipboard_total;           /* Staged clipboard bytes, at most EDITOR_CLIP_MAX. */
static size_t clipboard_offset;          /* Next byte offset to emit, never a Unicode source coordinate. */
static unsigned long clipboard_ticket;   /* Original view identity retained across fragment acknowledgements. */
static unsigned long clipboard_revision; /* Source revision captured by the Dis-authorized native command. */
static int clipboard_ack;                /* One pending acknowledgement; zero means none. */
static int clipboard_status;             /* Copied Idecore status: 0 success, 3 too large, 4 invalid/native error. */

static unsigned int canvas_width = EDITOR_WIDTH;   /* Last Dis-admitted raster/field width in pixels. */
static unsigned int canvas_height = EDITOR_HEIGHT; /* Last Dis-admitted raster/field height in pixels. */
static unsigned long canvas_revision;              /* Expected document revision for the pending canvas. */
static int desktop_bound; /* Context-local desktop binding is retired before context destruction. */
static const char *font_path;                      /* Borrowed explicit TTF/OpenType command-line path. */
static char *default_font_path;        /* Owned relocatable sibling resource path, unless --font overrides. */
static unsigned char *font_bytes;      /* Owned immutable face bytes for future actual size changes. */
static size_t font_byte_count;         /* Active font byte extent, bounded during admission. */
static int font_size = 11;             /* Initial pixel size; subsequent policy comes from Dis Style. */
static unsigned long style_revision;   /* Expected content revision of the pending font style. */
static double pinch_distance;          /* Bounded native gesture accumulator, never a document coordinate. */
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
static FVM_Object *panel_field;        /* [GC:] Pinned display-only field; input policy remains in Dis. */
static unsigned long
    panel_mode; /* Dis panel mode: 0 file, 1..6 edit, 7 inspect, 8 close, 9 browser, 10 directory path. */
static unsigned long panel_revision;           /* Expected content revision for the pending panel. */
static long panel_caret;                       /* Copied Unicode field caret, excluding its native label prefix. */
static long panel_anchor;                      /* Copied Unicode field selection anchor. */
static unsigned long panel_selection_revision; /* Expected source revision of the pending field endpoints. */
static FVM_TextLayout *panel_layout;           /* Owned same-contract glyph layout for the current completed field. */
static double panel_horizontal;                /* Effective native field scroll, never a source/text authority. */
static size_t panel_prefix;                    /* ASCII label byte/scalar extent used only for geometry. */
static int panel_selected;                     /* Dis-owned field select-all presentation flag. */
static char panel_text[1024];                  /* Copied counted UTF-8 prompt/path label. */
static size_t panel_size;                      /* Active panel bytes, never implicitly strlen-decoded. */
static FVM_Object *field;                      /* [GC:] Pinned display object until destroy. */
static unsigned char *pixels;                  /* Owned RGBA8 raster storage. */
static FVM_RenderQueue *raster_queue;          /* Owned persistent native raster worker, joined before VM teardown. */
static DVM_EditorFrame *recording_frame;       /* Owner-only frame builder; workers never read this global. */
static int raster_pending;            /* Owner-only input geometry barrier until the latest frame is presented. */

static char *snapshot;                /* Owned assembly storage, independent of both VM heaps. */
static size_t assembled;              /* Next expected UTF-8 byte offset. */
static size_t extent;                 /* Total expected snapshot bytes. */
static unsigned long revision;        /* Dis revision of the pending/published snapshot. */
static unsigned long status;          /* Dis status published with that snapshot. */
static long saved;                    /* Dis saved revision, used only for display. */
static unsigned long view_ticket;     /* Monotonic Dis view identity, independent of content revision. */
static unsigned long view_revision;   /* Expected content revision for viewport metadata. */
static unsigned long view_total;      /* Expected document Unicode scalar count. */
static int initial_line_numbers = 1;  /* Startup preference only, read once by Dis. */
static int line_numbers;              /* Admitted Dis view preference; no native shortcut changes it. */
static unsigned long gutter_revision; /* Revision that authorizes this view's gutter state. */
static unsigned int gutter_width;     /* Measured native pixels reserved beside source text. */
static unsigned long view_top;        /* Zero-based logical top line chosen by Dis. */
static long view_first;               /* Dis-selected source scalar at display start. */
static long view_last;                /* Dis-selected source scalar after display end. */
static size_t display_start;          /* Native byte translation of the admitted source scalar start. */
static size_t display_size;           /* Current display UTF-8 byte length, at most 4096. */
static FVM_TextLayout *hit_layout;    /* Owned actual glyph layout for the completed view. */
static double horizontal;             /* Renderer pixel translation, never a document coordinate. */
static int complete;                  /* A selection frame completed the current view. */
static int system_clipboard_events;   /* Explicit SDL clipboard replay check; ordinary replay remains isolated. */
static int window_replay;             /* Explicit SDL upload check with deterministic input. */
static int assembling;                /* A document snapshot is being assembled. */
#ifdef DVM_EDITOR_SDL
static SDL_Window *window;            /* Main-thread-only native window. */
static SDL_Renderer *renderer;        /* Main-thread presentation owner. */
static SDL_Texture *texture;          /* RGBA8 upload target. */
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

/* Return startup configuration by value; no renderer or document state is exposed. */
int dvm_editor_line_numbers(void)
{
    return initial_line_numbers;
}

/* Store a known CLI option without copying argv; returns whether it was recognized. */
int dvm_editor_option(const char *name, const char *value)
{
    if (!strcmp(name, "--line-numbers"))
    {
        if (strcmp(value, "0") && strcmp(value, "1"))
        {
            return 0;
        }
        initial_line_numbers = value[0] == '1';
    }
    else if (!strcmp(name, "--font"))
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
    else if (!strcmp(name, "--clipboard-events"))
    {
        event_path = value;
        window_replay = 1;
        system_clipboard_events = 1;
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

/* Choose the bundled font relative to the entry module, preserving the caller's cwd.
 * directory is borrowed. An explicit --font remains an optional override, never a prompt. */
int dvm_editor_default_font(const char *directory)
{
    const char *relative = "/fonts/NotoSansMono-Regular.ttf";
    size_t length;
    if (font_path)
    {
        return 1;
    }
    if (!directory || strlen(directory) > 4000)
    {
        return 0;
    }
    length = strlen(directory) + strlen(relative) + 1;
    default_font_path = malloc(length);
    if (!default_font_path)
    {
        return 0;
    }
    snprintf(default_font_path, length, "%s%s", directory, relative);
    font_path = default_font_path;
    return 1;
}

/* Reserve enough native pixels for a readable prompt at the admitted face size.
 * line_height is an actual finite font metric; returns bounded surface pixels, never text policy. */
static int panel_height(double line_height)
{
    int height = line_height > 300 ? 320 : (int)ceil(line_height) + 12;
    if (height < 48)
    {
        height = 48;
    }
    return height;
}

/* Measure copied multiline panel data for the AS view's bounded native surface.
 * line_height is a trusted font metric. Return pixels; no selection/action policy lives here. */
static int panel_extent(double line_height)
{
    int height = panel_height(line_height);
    if (panel_mode == EDITOR_PANEL_BROWSER)
    {
        size_t index;
        int lines = 1;
        for (index = 0; index < panel_size; index++)
        {
            if (panel_text[index] == '\n')
            {
                lines++;
            }
        }
        height = (int)ceil(lines * line_height) + 12;
        if (height > (int)canvas_height - 48)
        {
            height = (int)canvas_height - 48;
        }
    }
    return height;
}

/* Return the TextField baseline distance for the explicit editor TextFormat.
 * metrics is a borrowed current/proposed font record; no VM or view state changes. */
static double line_step(const FVM_FontMetrics *metrics)
{
    return metrics->ascent + metrics->descent + EDITOR_LEADING;
}

/* Measure bounded visible logical rows from the actual current face metrics.
 * No arguments; returns 1..128, with a conservative single row on an unavailable metric. */
int dvm_editor_rows(void)
{
    FVM_FontMetrics metrics;
    int rows;
    if (!layout_font || fvm_font_metrics(layout_font, &metrics) != FVM_OK || !(line_step(&metrics) > 0))
    {
        return 1;
    }
    rows = (int)floor((canvas_height - panel_height(line_step(&metrics)) - 4) / line_step(&metrics));
    if (rows < 1)
    {
        rows = 1;
    }
    if (rows > 128)
    {
        rows = 128;
    }
    return rows;
}

/* Measure a proposed face size without changing the current view or either VM state.
 * size is bounded 6..48; height is admitted 160..2048 pixels. Returns row capacity or zero on failure. */
static int rows_for_canvas(int size, unsigned int height)
{
    FVM_Font *font = 0;
    FVM_FontMetrics metrics;
    int rows;
    if (size == font_size && height == canvas_height)
    {
        return dvm_editor_rows();
    }
    if (size < EDITOR_FONT_MIN || size > EDITOR_FONT_MAX ||
        fvm_font_read(font_bytes, font_byte_count, 0, size, &font) != FVM_OK)
    {
        return 0;
    }
    if (fvm_font_metrics(font, &metrics) != FVM_OK || !(line_step(&metrics) > 0))
    {
        fvm_font_destroy(font);
        return 0;
    }
    rows = (int)floor((height - panel_height(line_step(&metrics)) - 4) / line_step(&metrics));
    fvm_font_destroy(font);
    if (rows < 1)
    {
        rows = 1;
    }
    if (rows > 128)
    {
        rows = 128;
    }
    return rows;
}

/* Admit a bounded Dis-selected surface with fresh storage before retiring the old one.
 * width/height are copied pixels. Returns zero on allocation/field/SDL failure; no document access.
 * A field failure is terminal to this view; successful allocation ownership changes only here. */
static int apply_canvas(unsigned int width, unsigned int height)
{
    unsigned char *candidate;
    FVM_Value value;
    FVM_Result result;
#ifdef DVM_EDITOR_SDL
    SDL_Texture *replacement = 0;
#endif
    if (width < EDITOR_WIDTH_MIN || height < EDITOR_HEIGHT_MIN || width > EDITOR_EXTENT_MAX ||
        height > EDITOR_EXTENT_MAX)
    {
        return 0;
    }
    if (width == canvas_width && height == canvas_height)
    {
        return 1;
    }
    candidate = malloc((size_t)width * height * 4U);
    if (!candidate)
    {
        return 0;
    }
#ifdef DVM_EDITOR_SDL
    if (renderer)
    {
        replacement = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!replacement)
        {
            free(candidate);
            return 0;
        }
    }
#endif
    fvm_value_number(&value, width);
    result = fvm_object_set(context, field, "_width", &value);
    if (result == FVM_OK)
    {
        fvm_value_number(&value, height);
        result = fvm_object_set(context, field, "_height", &value);
    }
    if (result == FVM_OK)
    {
        fvm_value_number(&value, width - 16);
        result = fvm_object_set(context, panel_field, "_width", &value);
    }
    if (result != FVM_OK)
    {
#ifdef DVM_EDITOR_SDL
        if (replacement)
        {
            SDL_DestroyTexture(replacement);
        }
#endif
        free(candidate);
        return 0;
    }
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
    texture = replacement;
    if (window)
    {
        int current_width;
        int current_height;
        SDL_GetWindowSize(window, &current_width, &current_height);
        if (current_width != (int)width || current_height != (int)height)
        {
            SDL_SetWindowSize(window, width, height);
        }
    }
#endif
    free(pixels);
    pixels = candidate;
    canvas_width = width;
    canvas_height = height;
    return 1;
}

/* Apply a Dis-admitted size to both real TextFields and the glyph-hit face.
 * size is bounded; returns one on actual matching font/layout setup, zero on failure. */
static int apply_font_size(int size)
{
    FVM_Object *format = 0;
    FVM_Font *font = 0;
    FVM_Value value;
    FVM_Value argument;
    FVM_Value ignored;
    FVM_Result result;
    if (size == font_size)
    {
        return 1;
    }
    if (size < EDITOR_FONT_MIN || size > EDITOR_FONT_MAX ||
        fvm_font_read(font_bytes, font_byte_count, 0, size, &font) != FVM_OK ||
        fvm_construct(context, "TextFormat", 0, 0, &format) != FVM_OK || fvm_gc_pin(context, format) != FVM_OK)
    {
        fvm_font_destroy(font);
        return 0;
    }
    fvm_value_number(&value, size);
    result = fvm_object_set(context, format, "size", &value);
    if (result == FVM_OK)
    {
        fvm_value_number(&value, EDITOR_LEADING);
        result = fvm_object_set(context, format, "leading", &value);
    }
    if (result == FVM_OK)
    {
        fvm_value_boolean(&value, 0);
        result = fvm_object_set(context, format, "kerning", &value);
    }
    fvm_value_undefined(&argument);
    argument.type = FVM_VALUE_OBJECT;
    argument.object = format;
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, field, "setNewTextFormat", 1, &argument, &ignored);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, panel_field, "setNewTextFormat", 1, &argument, &ignored);
    }
    /* A loaded SWF retains current text runs independently of future insertion format.
     * Update both domains so the glyph raster and admitted caret metrics scale together. */
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, field, "setTextFormat", 1, &argument, &ignored);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, panel_field, "setTextFormat", 1, &argument, &ignored);
    }
    fvm_gc_unpin(context, format);
    if (result != FVM_OK)
    {
        fvm_font_destroy(font);
        return 0;
    }
    fvm_font_destroy(layout_font);
    layout_font = font;
    font_size = size;
    return 1;
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
        fprintf(stderr, "editor bundled font resource is unavailable\n");
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
    font_bytes = bytes;
    font_byte_count = (size_t)size;
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
        result = fvm_load_memory(context, ide_view_swf, sizeof(ide_view_swf));
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
        result = fvm_font_read(bytes, (size_t)size, 0, font_size, &layout_font);
    }
    if (result != FVM_OK || fvm_textfield_create(context, "Editor", canvas_width, canvas_height, &field) != FVM_OK ||
        fvm_gc_pin(context, field) != FVM_OK ||
        fvm_textfield_create(context, "Editor", canvas_width - 16, 44, &panel_field) != FVM_OK ||
        fvm_gc_pin(context, panel_field) != FVM_OK)
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
        fvm_value_number(&value, font_size);
        result = fvm_object_set(context, format, "size", &value);
        if (result == FVM_OK)
        {
            fvm_value_number(&value, EDITOR_LEADING);
            result = fvm_object_set(context, format, "leading", &value);
        }
        if (result == FVM_OK)
        {
            fvm_value_boolean(&value, 0);
            result = fvm_object_set(context, format, "kerning", &value);
        }
        fvm_value_undefined(&argument);
        argument.type = FVM_VALUE_OBJECT;
        argument.object = format;
        if (result == FVM_OK)
        {
            result = fvm_object_call(context, field, "setNewTextFormat", 1, &argument, &ignored);
            if (result == FVM_OK)
            {
                result = fvm_object_call(context, panel_field, "setNewTextFormat", 1, &argument, &ignored);
            }
        }
        fvm_gc_unpin(context, format);
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    fvm_value_boolean(&value, 1);
    if (fvm_object_set(context, panel_field, "multiline", &value) != FVM_OK ||
        fvm_object_set(context, field, "multiline", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_boolean(&value, 0);
    if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK)
    {
        return 0;
    }
    pixels = malloc(canvas_width * canvas_height * 4U);
    snapshot = malloc(EDITOR_SOURCE_MAX + 1U);
    if (!pixels || !snapshot ||
        fvm_render_queue_create(1, dvm_editor_frame_render, dvm_editor_frame_destroy, &raster_queue) != FVM_OK)
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
        window = SDL_CreateWindow("FlashVM / Dis IDE",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  canvas_width,
                                  canvas_height,
                                  SDL_WINDOW_RESIZABLE);
        if (window)
        {
            SDL_SetWindowMinimumSize(window, EDITOR_WIDTH_MIN, EDITOR_HEIGHT_MIN);
            SDL_SetWindowMaximumSize(window, EDITOR_EXTENT_MAX, EDITOR_EXTENT_MAX);
        }
        renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : 0;
        texture = renderer
                      ? SDL_CreateTexture(
                            renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, canvas_width, canvas_height)
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

/* Record a clipped rectangle in the owner-held frame without document mutation.
 * surface supplies only pixel bounds; color is copied. Return one on admitted
 * geometry (including offscreen no-op), zero on contour allocation/budget failure. */
static int rectangle(FVM_RasterSurface *surface, double x, double y, double width, double height, FVM_RasterColor color)
{
    if (width < 0)
    {
        x += width;
        width = -width;
    }
    if (x + width <= 0 || y + height <= 0 || x >= surface->width || y >= surface->height)
    {
        return 1;
    }
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > surface->width)
    {
        width = surface->width - x;
    }
    if (y + height > surface->height)
    {
        height = surface->height - y;
    }
    FVM_RasterPoint points[4];
    points[0].x = x;
    points[0].y = y;
    points[1].x = x + width;
    points[1].y = y;
    points[2].x = x + width;
    points[2].y = y + height;
    points[3].x = x;
    points[3].y = y + height;
    {
        FVM_Path *path = NULL;
        FVM_PathCommand command;
        unsigned int index;
        FVM_Result result = fvm_path_create(&path);
        memset(&command, 0, sizeof(command));
        for (index = 0; index < 4 && result == FVM_OK; index++)
        {
            command.verb = index ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = points[index].x;
            command.y[0] = points[index].y;
            result = fvm_path_add(path, &command);
        }
        command.verb = FVM_PATH_CLOSE;
        if (result == FVM_OK)
        {
            result = fvm_path_add(path, &command);
        }
        if (result == FVM_OK && dvm_editor_frame_path(recording_frame, path, color))
        {
            return 1;
        }
        fvm_path_destroy(path);
        return 0;
    }
}

/* Map borrowed valid UTF-8 scalar positions to bounded preview byte offsets.
 * Offscreen positions return shown+1; bytes and document authority stay unchanged. */
static size_t scalar_byte(size_t shown, long position)
{
    size_t byte = 0;
    long scalar = view_first;
    const char *text = snapshot + display_start;
    if (position < view_first)
    {
        return shown + 1;
    }
    while (byte < shown && scalar < position)
    {
        byte++;
        while (byte < shown && ((unsigned char)text[byte] & 0xc0U) == 0x80U)
        {
            byte++;
        }
        scalar++;
    }
    return scalar == position ? byte : shown + 1;
}

/* Translate an admitted scalar insertion coordinate in borrowed valid UTF-8.
 * text/length are borrowed bytes; position is nonnegative. Returns a bounded byte count without mutation. */
static size_t text_byte(const char *text, size_t length, long position)
{
    size_t byte = 0;
    long scalar = 0;
    while (byte < length && scalar < position)
    {
        byte++;
        while (byte < length && ((unsigned char)text[byte] & 0xc0U) == 0x80U)
        {
            byte++;
        }
        scalar++;
    }
    return byte;
}

/* Translate a prevalidated source scalar through its immutable complete UTF-8 snapshot.
 * position is copied; returns a byte count and performs no VM, allocation or I/O operation. */
static size_t document_byte(long position)
{
    return text_byte(snapshot, extent, position);
}

/* Queue one copied clipboard result for the next ordinary input rendezvous.
 * status is an Idecore-compatible scalar; no source edit or channel operation occurs here. */
static void clipboard_result(int status)
{
    clipboard_status = status;
    clipboard_ack = 1;
}

/* Execute a Dis-authorized clipboard command against the previous complete immutable snapshot.
 * message is borrowed. Returns zero for a framing/identity error, one for a queued value outcome.
 * Native buffers remain owner-local; failed writes never authorize a Cut source mutation.
 * Platform clipboard access may block this native UI owner; no Dis heap pointer escapes. */
static int clipboard_command(const DVM_UIMessage *message)
{
    unsigned long ticket;
    char *candidate;
    const char *source;
    size_t length;
    size_t first;
    size_t last;
    size_t scalars;
    if (assembling || clipboard_ack || clipboard_transfer || message->size != EDITOR_CLIP_PREFIX ||
        message->flags != revision || message->flags != view_revision || !hexword(message->text, &ticket) ||
        ticket != view_ticket)
    {
        return 0;
    }
    clipboard_ticket = ticket;
    clipboard_revision = message->flags;
    if (message->code == EDITOR_CLIP_WRITE || message->code == EDITOR_CLIP_FIELD_WRITE)
    {
        source = snapshot;
        length = extent;
        scalars = view_total;
        if (message->code == EDITOR_CLIP_FIELD_WRITE)
        {
            if (!panel_mode || panel_revision != revision ||
                fvm_utf8_validate(panel_text, panel_size, &scalars) != FVM_UTF8_OK)
            {
                clipboard_result(EDITOR_CLIP_INVALID);
                return 1;
            }
            source = panel_text;
            length = panel_size;
        }
        if (message->first < 0 || message->second < message->first || (unsigned long)message->second > scalars)
        {
            return 0;
        }
        first = text_byte(source, length, message->first);
        last = text_byte(source, length, message->second);
        length = last - first;
        if (length > EDITOR_CLIP_MAX)
        {
            clipboard_result(EDITOR_CLIP_TOO_LARGE);
            return 1;
        }
        if (memchr(source + first, 0, length))
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        candidate = malloc(length + 1);
        if (!candidate)
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        memcpy(candidate, source + first, length);
        candidate[length] = 0;
#ifdef DVM_EDITOR_SDL
        if ((!events || system_clipboard_events) && SDL_SetClipboardText(candidate) != 0)
        {
            free(candidate);
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
#endif
        free(clipboard_text);
        clipboard_text = candidate;
        clipboard_result(EDITOR_CLIP_OK);
        return 1;
    }
    if (message->first || message->second)
    {
        return 0;
    }
    source = clipboard_text ? clipboard_text : "";
#ifdef DVM_EDITOR_SDL
    candidate = !events || system_clipboard_events ? SDL_GetClipboardText() : 0;
    if (!events || system_clipboard_events)
    {
        if (!candidate)
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        source = candidate;
    }
#endif
    length = 0;
    while (length <= EDITOR_CLIP_MAX && source[length])
    {
        length++;
    }
    clipboard_status = length > EDITOR_CLIP_MAX                                     ? 3
                       : fvm_utf8_validate(source, length, &scalars) == FVM_UTF8_OK ? 0
                                                                                    : 4;
    clipboard_transfer = clipboard_status ? 0 : malloc(length + 1);
    if (!clipboard_status && !clipboard_transfer)
    {
        clipboard_status = EDITOR_CLIP_INVALID;
    }
    if (!clipboard_status)
    {
        memcpy(clipboard_transfer, source, length + 1);
        clipboard_total = length;
        clipboard_offset = 0;
    }
#ifdef DVM_EDITOR_SDL
    if (!events || system_clipboard_events)
    {
        SDL_free(candidate);
    }
#endif
    if (clipboard_status)
    {
        clipboard_result(clipboard_status);
    }
    return 1;
}

/* Copy at most one acknowledged result or Unicode-aligned read fragment into message.
 * message is caller-owned. Returns one for a pending result; zero leaves ordinary input available.
 * Bytes are copied before native storage is freed; a later document edit is solely Dis policy. */
static int clipboard_event(DVM_UIMessage *message)
{
    char identity[9];
    size_t count;
    if (!clipboard_ack && !clipboard_transfer)
    {
        return 0;
    }
    message->kind = DVM_UI_REQUEST;
    message->id = 1;
    message->flags = clipboard_revision;
    snprintf(identity, sizeof(identity), "%08lx", clipboard_ticket);
    memcpy(message->text, identity, EDITOR_CLIP_PREFIX);
    message->size = EDITOR_CLIP_PREFIX;
    if (clipboard_ack)
    {
        message->code = EDITOR_CLIP_ACK;
        message->first = clipboard_status;
        clipboard_ack = 0;
        return 1;
    }
    count = clipboard_total - clipboard_offset;
    if (count > DVM_UI_TEXT_MAX - EDITOR_CLIP_PREFIX)
    {
        count = DVM_UI_TEXT_MAX - EDITOR_CLIP_PREFIX;
        while (count && ((unsigned char)clipboard_transfer[clipboard_offset + count] & 0xc0U) == 0x80U)
        {
            count--;
        }
    }
    message->code = EDITOR_CLIP_DATA;
    message->first = (long)clipboard_offset;
    message->second = (long)clipboard_total;
    memcpy(message->text + EDITOR_CLIP_PREFIX, clipboard_transfer + clipboard_offset, count);
    message->size += count;
    clipboard_offset += count;
    if (clipboard_offset == clipboard_total)
    {
        free(clipboard_transfer);
        clipboard_transfer = 0;
    }
    return 1;
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
    FVM_TextFieldScrollInfo scrolling;
    FVM_Value value;
    FVM_RasterColor selected = {204, 222, 255, 255};
    FVM_RasterColor cursor = {35, 85, 170, 255};
    size_t first = scalar_byte(shown, caret < anchor ? caret : anchor);
    size_t last = scalar_byte(shown, caret > anchor ? caret : anchor);
    size_t current = scalar_byte(shown, caret);
    size_t index;
    double cursor_x = 0;
    double cursor_y = 0;
    int cursor_visible = 0;
    int ok = 1;
    if ((caret < anchor ? caret : anchor) < view_first)
    {
        first = 0;
    }
    if ((caret > anchor ? caret : anchor) > view_last)
    {
        last = shown;
    }
    memset(&options, 0, sizeof(options));
    options.alignment = FVM_TEXT_ALIGN_LEFT;
    if (fvm_font_metrics(layout_font, &font) != FVM_OK)
    {
        return 0;
    }
    options.line_height = line_step(&font);
    {
        FVM_Result result;
        if (memchr(snapshot + display_start, '\t', shown))
        {
            double average;
            result = fvm_font_average_width(layout_font, &average);
            if (result == FVM_OK)
            {
                /* Match AS2 TextField's implicit tab interval: four average character widths. */
                result = fvm_text_layout_create_periodic(layout_font,
                                                         snapshot + display_start,
                                                         shown,
                                                         &options,
                                                         FVM_FONT_SHAPE_NO_KERNING,
                                                         0,
                                                         0,
                                                         average * 4,
                                                         &layout);
            }
        }
        else
        {
            result = fvm_text_layout_create_configured(
                layout_font, snapshot + display_start, shown, &options, FVM_FONT_SHAPE_NO_KERNING, &layout);
        }
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    if (fvm_text_layout_metrics(layout, &metrics) != FVM_OK)
    {
        fvm_text_layout_destroy(layout);
        return 0;
    }
    for (index = 0; index < metrics.line_count && current <= shown && ok; index++)
    {
        FVM_TextLayoutLine line;
        size_t glyph_index;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (current < line.byte_start || (current >= line.byte_next && index + 1 < metrics.line_count))
        {
            continue;
        }
        cursor_x = line.x + line.advance;
        cursor_y = line.baseline - font.ascent;
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
                cursor_x = glyph.x;
                break;
            }
        }
        cursor_visible = ok;
        break;
    }
    if (cursor_visible)
    {
        if (cursor_x - horizontal < 0)
        {
            horizontal = floor(cursor_x);
        }
        if (cursor_x - horizontal > canvas_width - gutter_width - 12)
        {
            horizontal = ceil(cursor_x - (canvas_width - gutter_width - 12));
        }
    }
    if (horizontal < 0)
    {
        horizontal = 0;
    }
    fvm_value_number(&value, horizontal);
    scrolling.struct_size = sizeof(scrolling);
    if (fvm_object_set(context, field, "hscroll", &value) != FVM_OK ||
        fvm_textfield_scroll_info(context, field, &scrolling) != FVM_OK)
    {
        ok = 0;
    }
    if (ok)
    {
        horizontal = (double)scrolling.hscroll;
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
            ok = rectangle(surface,
                           glyph.x + 2 + gutter_width - horizontal,
                           glyph.y - font.ascent + 2,
                           glyph.shaped.x_advance,
                           line_step(&font),
                           selected);
        }
    }
    if (ok && cursor_visible && !panel_mode)
    {
        ok = rectangle(surface, cursor_x + 2 + gutter_width - horizontal, cursor_y + 2, 1, line_step(&font), cursor);
    }
    if (ok)
    {
        fvm_text_layout_destroy(hit_layout);
        hit_layout = layout;
    }
    else
    {
        fvm_text_layout_destroy(layout);
    }
    return ok;
}

/* Cull entire offscreen font contours before the bounded public raster API.
 * path/surface are borrowed, color is copied. Retained curves/control points stay exact;
 * this is conservative contour culling, never point clamping or glyph distortion. */
static FVM_Result draw_visible(const FVM_Path *path, FVM_RasterSurface *surface, FVM_RasterColor color)
{
    FVM_Path *visible = 0;
    FVM_Result result = fvm_path_create(&visible);
    size_t count = fvm_path_count(path);
    size_t first = 0;
    while (first < count && result == FVM_OK)
    {
        size_t last = first;
        double left = 1e100;
        double right = -1e100;
        double top = 1e100;
        double bottom = -1e100;
        while (last < count)
        {
            FVM_PathCommand command;
            unsigned int points;
            unsigned int index;
            result = fvm_path_get(path, last, &command);
            if (result != FVM_OK || (last != first && command.verb == FVM_PATH_MOVE))
            {
                break;
            }
            points = command.verb == FVM_PATH_CLOSE       ? 0U
                     : command.verb == FVM_PATH_CUBIC     ? 3U
                     : command.verb == FVM_PATH_QUADRATIC ? 2U
                                                          : 1U;
            for (index = 0; index < points; index++)
            {
                if (command.x[index] < left)
                {
                    left = command.x[index];
                }
                if (command.x[index] > right)
                {
                    right = command.x[index];
                }
                if (command.y[index] < top)
                {
                    top = command.y[index];
                }
                if (command.y[index] > bottom)
                {
                    bottom = command.y[index];
                }
            }
            last++;
        }
        if (last == first || result != FVM_OK)
        {
            result = FVM_INVALID_ARGUMENT;
            break;
        }
        if (right >= 0 && bottom >= 0 && left <= surface->width && top <= surface->height)
        {
            size_t index;
            for (index = first; index < last && result == FVM_OK; index++)
            {
                FVM_PathCommand command;
                result = fvm_path_get(path, index, &command);
                if (result == FVM_OK)
                {
                    result = fvm_path_add(visible, &command);
                }
            }
        }
        first = last;
    }
    if (result == FVM_OK)
    {
        if (dvm_editor_frame_path(recording_frame, visible, color))
        {
            visible = NULL;
        }
        else
        {
            result = FVM_OUT_OF_MEMORY;
        }
    }
    fvm_path_destroy(visible);
    return result;
}

/* Resolve an admitted byte insertion coordinate in one owned single-line field layout.
 * layout is borrowed; out receives native pixels. Returns zero on invalid geometry, without policy changes. */
static int field_position(const FVM_TextLayout *layout, size_t byte, double *out)
{
    FVM_TextLayoutLine line;
    size_t index;
    if (fvm_text_layout_line(layout, 0, &line) != FVM_OK)
    {
        return 0;
    }
    *out = line.x + line.advance;
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        if (fvm_text_layout_glyph(layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return 0;
        }
        if (glyph.shaped.cluster >= byte)
        {
            *out = glyph.x;
            break;
        }
    }
    return 1;
}

/* Draw a channel-published prompt/status field over the document surface.
 * surface is borrowed; mode/text/selection are copied Dis values. Returns one only on real rendering. */
static int render_panel(FVM_RasterSurface *surface)
{
    static const char *labels[] = {"File: ",
                                   "Find [Enter/Esc]: ",
                                   "Open [Enter/Esc]: ",
                                   "Save As [Enter/Esc]: ",
                                   "Replace needle: ",
                                   "Replacement: ",
                                   "Go to line[:column]: ",
                                   "Inspect [Esc]: ",
                                   "Unsaved [S save / D discard / Esc cancel]: ",
                                   "",
                                   "Directory path [Enter/Esc]: ",
                                   "Export NEW file [Enter/Esc]: "};
    char text[1100];
    size_t prefix = strlen(labels[panel_mode]);
    FVM_Value value;
    FVM_Path *path = 0;
    FVM_Affine placement;
    FVM_TextFieldScrollInfo scrolling;
    FVM_RasterColor background = {232, 237, 247, 255};
    FVM_RasterColor active = {197, 217, 252, 255};
    FVM_RasterColor ink = {24, 40, 72, 255};
    FVM_Result result;
    FVM_FontMetrics metrics;
    int height;
    double cursor_x = 0;
    size_t first = 0;
    size_t last = 0;
    size_t index;
    if (fvm_font_metrics(layout_font, &metrics) != FVM_OK)
    {
        return 0;
    }
    height = panel_extent(line_step(&metrics));
    if (!rectangle(surface, 0, canvas_height - height, canvas_width, height, background))
    {
        return 0;
    }
    memcpy(text, labels[panel_mode], prefix);
    memcpy(text + prefix, panel_text, panel_size);
    if (fvm_value_string_bytes(&value, text, prefix + panel_size) != FVM_OK ||
        fvm_set_global(context, "idePanelText", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = panel_field;
    if (fvm_set_global(context, "idePanel", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, canvas_width);
    if (fvm_set_global(context, "ideWidth", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, canvas_height);
    if (fvm_set_global(context, "ideHeight", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, height);
    if (fvm_set_global(context, "idePanelHeight", &value) != FVM_OK || fvm_swf_execute_actions(context) != FVM_OK)
    {
        return 0;
    }
    scrolling.struct_size = sizeof(scrolling);
    if (fvm_textfield_scroll_info(context, panel_field, &scrolling) != FVM_OK)
    {
        return 0;
    }
    if (panel_mode)
    {
        FVM_TextLayoutOptions options;
        FVM_TextLayout *layout = 0;
        memset(&options, 0, sizeof(options));
        options.alignment = FVM_TEXT_ALIGN_LEFT;
        options.line_height = line_step(&metrics);
        if (fvm_text_layout_create_configured(
                layout_font, text, prefix + panel_size, &options, FVM_FONT_SHAPE_NO_KERNING, &layout) != FVM_OK)
        {
            return 0;
        }
        fvm_text_layout_destroy(panel_layout);
        panel_layout = layout;
        panel_prefix = prefix;
        first = prefix + text_byte(panel_text, panel_size, panel_caret < panel_anchor ? panel_caret : panel_anchor);
        last = prefix + text_byte(panel_text, panel_size, panel_caret > panel_anchor ? panel_caret : panel_anchor);
        if (panel_mode == EDITOR_PANEL_BROWSER)
        {
            panel_horizontal = 0;
        }
        else if (!field_position(panel_layout, prefix + text_byte(panel_text, panel_size, panel_caret), &cursor_x))
        {
            return 0;
        }
        if (cursor_x - panel_horizontal < 0)
        {
            panel_horizontal = floor(cursor_x);
        }
        if (cursor_x - panel_horizontal > canvas_width - 24)
        {
            panel_horizontal = ceil(cursor_x - (canvas_width - 24));
        }
    }
    else
    {
        fvm_text_layout_destroy(panel_layout);
        panel_layout = 0;
        panel_horizontal = 0;
    }
    fvm_value_number(&value, panel_horizontal);
    if (fvm_object_set(context, panel_field, "hscroll", &value) != FVM_OK ||
        fvm_textfield_scroll_info(context, panel_field, &scrolling) != FVM_OK)
    {
        return 0;
    }
    panel_horizontal = scrolling.hscroll;
    memset(&placement, 0, sizeof(placement));
    placement.xx = 1;
    placement.yy = 1;
    if (fvm_object_get(context, panel_field, "_x", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER)
    {
        return 0;
    }
    placement.x0 = value.number;
    if (fvm_object_get(context, panel_field, "_y", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER)
    {
        return 0;
    }
    placement.y0 = value.number;
    if (panel_layout)
    {
        FVM_TextLayoutMetrics layout_metrics;
        if (fvm_text_layout_metrics(panel_layout, &layout_metrics) != FVM_OK)
        {
            return 0;
        }
        for (index = 0; index < layout_metrics.glyph_count; index++)
        {
            FVM_TextLayoutGlyph glyph;
            if (fvm_text_layout_glyph(panel_layout, index, &glyph) != FVM_OK)
            {
                return 0;
            }
            if (glyph.shaped.cluster >= first && glyph.shaped.cluster < last &&
                !rectangle(surface,
                           placement.x0 + 2 + glyph.x - panel_horizontal,
                           placement.y0 + 2 + glyph.y - metrics.ascent,
                           glyph.shaped.x_advance,
                           line_step(&metrics),
                           active))
            {
                return 0;
            }
        }
    }
    result = fvm_textfield_path(context, panel_field, &path);
    if (result == FVM_OK)
    {
        result = fvm_path_transform(path, &placement);
    }
    if (result == FVM_OK)
    {
        result = draw_visible(path, surface, ink);
    }
    fvm_path_destroy(path);
    if (result == FVM_OK && panel_mode && (panel_mode < 7 || panel_mode == EDITOR_PANEL_BROWSE_PATH || panel_mode == EDITOR_PANEL_EXPORT_PATH))
    {
        FVM_RasterColor cursor = {35, 85, 170, 255};
        if (!rectangle(surface,
                       placement.x0 + 2 + cursor_x - panel_horizontal,
                       placement.y0 + 2,
                       1,
                       line_step(&metrics),
                       cursor))
        {
            return 0;
        }
    }
    return result == FVM_OK;
}

/* Reserve a measured strip and update only the native source field's available width.
 * Uses copied viewport/font/visibility data; returns zero on font/field errors, never edits source. */
static int measure_gutter(void)
{
    unsigned long highest = view_top + (unsigned long)dvm_editor_rows() + 1;
    unsigned int digits = 1;
    unsigned int digit;
    double widest = 0;
    FVM_Value value;
    gutter_width = 0;
    if (line_numbers)
    {
        while (highest >= 10)
        {
            highest /= 10;
            digits++;
        }
        for (digit = 0; digit < 10; digit++)
        {
            FVM_Path *path = 0;
            double advance = 0;
            double vertical = 0;
            if (fvm_font_glyph(layout_font, fvm_font_char(layout_font, '0' + digit), &path, &advance, &vertical) !=
                FVM_OK)
            {
                return 0;
            }
            fvm_path_destroy(path);
            if (advance > widest)
            {
                widest = advance;
            }
        }
        gutter_width = (unsigned int)ceil(widest * digits + 12);
        if (gutter_width > canvas_width / 2)
        {
            gutter_width = canvas_width / 2;
        }
    }
    fvm_value_number(&value, canvas_width - gutter_width);
    return fvm_object_set(context, field, "_width", &value) == FVM_OK;
}

/* Paint logical numbers at the actual source layout baselines, covering scrolled source ink.
 * surface is borrowed, layout contains only the acknowledged viewport. No source parsing/mutation. */
static int render_gutter(FVM_RasterSurface *surface)
{
    FVM_RasterColor background = {242, 244, 248, 255};
    FVM_RasterColor ink = {105, 113, 128, 255};
    FVM_FontMetrics font;
    FVM_TextLayoutMetrics metrics;
    unsigned int index;
    double bottom;
    if (!gutter_width)
    {
        return 1;
    }
    if (fvm_font_metrics(layout_font, &font) != FVM_OK || fvm_text_layout_metrics(hit_layout, &metrics) != FVM_OK)
    {
        return 0;
    }
    bottom = canvas_height - panel_height(line_step(&font));
    if (!rectangle(surface, 0, 0, gutter_width, bottom, background))
    {
        return 0;
    }
    for (index = 0; index < metrics.line_count; index++)
    {
        FVM_TextLayoutLine line;
        FVM_TextRun *run = 0;
        FVM_Path *path = 0;
        FVM_Affine placement;
        FVM_Result result;
        double advance = 0;
        double vertical = 0;
        char label[32];
        if (fvm_text_layout_line(hit_layout, index, &line) != FVM_OK)
        {
            return 0;
        }
        if (line.baseline - font.ascent + 2 >= bottom)
        {
            break;
        }
        snprintf(label, sizeof(label), "%lu", view_top + index + 1);
        result = fvm_font_shape_configured(
            layout_font, label, strlen(label), FVM_TEXT_LTR, 0, FVM_FONT_SHAPE_NO_KERNING, &run);
        if (result == FVM_OK)
        {
            result = fvm_text_run_path(layout_font, run, &path, &advance, &vertical);
        }
        memset(&placement, 0, sizeof(placement));
        placement.xx = 1;
        placement.yy = 1;
        placement.x0 = gutter_width - 6 - advance;
        placement.y0 = line.baseline + 2;
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, &placement);
        }
        if (result == FVM_OK)
        {
            result = draw_visible(path, surface, ink);
        }
        fvm_path_destroy(path);
        fvm_text_run_destroy(run);
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}

/* Prepare a complete owner-only document view as detached native paint commands.
 * recording_frame is borrowed and receives owned paths. Return one on success or
 * zero on layout/geometry/allocation failure; no pixels, files or SDL are touched. */
static int prepare_frame(void)
{
    FVM_Value value;
    FVM_Path *path = 0;
    FVM_RasterSurface surface;
    FVM_RasterColor ink = {32, 36, 48, 255};
    FVM_Result result;
    size_t shown;
    display_start = document_byte(view_first);
    shown = document_byte(view_last) - display_start;
    if (shown > EDITOR_VIEW_BYTES)
    {
        return 0;
    }
    display_size = shown;
    if (fvm_value_string_bytes(&value, snapshot + display_start, shown) != FVM_OK ||
        fvm_object_set(context, field, "text", &value) != FVM_OK ||
        fvm_object_get(context, field, "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
        fvm_value_string_size(&value) != shown || memcmp(value.string, snapshot + display_start, shown))
    {
        return 0;
    }
    surface.pixels = NULL; /* Geometry only: worker owns the detached destination. */
    surface.width = canvas_width;
    surface.height = canvas_height;
    surface.stride = canvas_width * 4U;
    if (!measure_gutter() || !selection_geometry(&surface, shown))
    {
        return 0;
    }
    result = fvm_textfield_path(context, field, &path);
    if (result == FVM_OK && gutter_width)
    {
        FVM_Affine shift;
        memset(&shift, 0, sizeof(shift));
        shift.xx = shift.yy = 1;
        shift.x0 = gutter_width;
        result = fvm_path_transform(path, &shift);
    }
    if (result == FVM_OK)
    {
        result = draw_visible(path, &surface, ink);
    }
    fvm_path_destroy(path);
    if (result != FVM_OK)
    {
        fprintf(stderr, "editor TextField raster failed: %d\n", result);
        return 0;
    }
    if (!render_gutter(&surface) || !render_panel(&surface))
    {
        return 0;
    }
    return 1;
}


/* Present only completed native pixels on the UI owner; SDL and capture stay here.
 * No VM state enters the raster worker. Returns zero on SDL/file failure. */
static int present_pixels(void)
{
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        char title[256];
        snprintf(title,
                 sizeof(title),
                 "Dis IDE | rev %lu saved %ld status %lu | top %lu | Ctrl+S save, Ctrl+O open, Ctrl+G location%s",
                 revision,
                 saved,
                 status,
                 view_top + 1,
                 view_first != 0 || view_last != (long)view_total ? " | bounded window" : "");
        SDL_SetWindowTitle(window, title);
        if (SDL_UpdateTexture(texture, 0, pixels, canvas_width * 4U) || SDL_RenderClear(renderer) ||
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
        if (fprintf(capture, "P6\n%u %u\n255\n", canvas_width, canvas_height) < 0)
        {
            failed = 1;
        }
        for (index = 0; index < canvas_width * canvas_height && !failed; index++)
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
    return 1;
}


/* Collect the newest detached raster result; owner-only, wait is explicit capture mode.
 * Return one for ready or idle, zero on native failure. Never publish a stale-size frame. */
static int collect_frame(int wait)
{
    void *job = NULL;
    FVM_Result rendered;
    FVM_Result result = fvm_render_queue_take(raster_queue, &job, &rendered, wait);
    const FVM_RasterSurface *surface;
    int ok = 1;
    if (result == FVM_NOT_FOUND)
    {
        return 1;
    }
    if (result != FVM_OK)
    {
        return 0;
    }
    surface = dvm_editor_frame_surface(job);
    raster_pending = 0;
    if (rendered != FVM_OK)
    {
        fprintf(stderr, "editor background raster failed: %d\n", rendered);
        ok = 0;
    }
    else if (surface->width == canvas_width && surface->height == canvas_height)
    {
        memcpy(pixels, surface->pixels, (size_t)canvas_width * canvas_height * 4);
        ok = present_pixels();
    }
    dvm_editor_frame_destroy(job);
    return ok;
}


/* Prepare independent native geometry and submit without waiting for rasterization.
 * [GC:] All VM/font queries and collection remain on this owner. Worker paths have
 * no heap references. Capture deliberately waits; replay input advances only after owner presentation. */
static int render(void)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(canvas_width, canvas_height);
    int prepared;
    if (!frame)
    {
        return 0;
    }
    recording_frame = frame;
    prepared = prepare_frame();
    recording_frame = NULL;
    if (!prepared || fvm_gc_collect(context, 0) != FVM_OK || fvm_render_queue_submit(raster_queue, frame) != FVM_OK)
    {
        dvm_editor_frame_destroy(frame);
        return 0;
    }
    raster_pending = 1;
    return collect_frame(capture_path != NULL);
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
    if (message->code >= DVM_DESK_BEGIN && message->code <= DVM_DESK_END)
    {
        if (!desktop_bound)
        {
            if (!dvm_desktop_bind(context, font_bytes, font_byte_count, events != NULL)) { return 0; }
            desktop_bound = 1;
        }
        if (message->code == DVM_DESK_BEGIN && !apply_canvas((unsigned int)message->first, (unsigned int)message->second))
        { return 0; }
        if (!dvm_desktop_message(message)) { fprintf(stderr, "desktop wire code=%lu first=%ld size=%lu\n", message->code, message->first, (unsigned long)message->size); return 0; }
        if (message->code == DVM_DESK_END)
        {
            FVM_RasterSurface surface;
            DVM_EditorFrame *frame = dvm_editor_frame_create(canvas_width, canvas_height);
            surface.pixels = pixels; surface.width = canvas_width; surface.height = canvas_height;
            surface.stride = (size_t)canvas_width * 4;
            if (!frame || !dvm_desktop_draw(&surface) || !dvm_editor_frame_background(frame, &surface) ||
                fvm_gc_collect(context, 0) != FVM_OK || fvm_render_queue_submit(raster_queue, frame) != FVM_OK)
            { dvm_editor_frame_destroy(frame); return 0; }
            raster_pending = 1;
            complete = 1;
            return collect_frame(capture_path != NULL);
        }
        return 1;
    }
    if (message->code == EDITOR_CLIP_CONTINUE)
    {
        unsigned long ticket;
        if (complete || assembling || !clipboard_transfer || message->size != EDITOR_CLIP_PREFIX ||
            message->flags != clipboard_revision || message->first != (long)clipboard_offset ||
            message->second != (long)clipboard_total || !hexword(message->text, &ticket) || ticket != clipboard_ticket)
        {
            return 0;
        }
        complete = 1;
        return 1;
    }
    if (message->code == EDITOR_CLIP_WRITE || message->code == EDITOR_CLIP_FIELD_WRITE ||
        message->code == EDITOR_CLIP_READ)
    {
        return clipboard_command(message);
    }
    if (message->code == EDITOR_CANVAS)
    {
        if (assembling || message->first < EDITOR_WIDTH_MIN || message->second < EDITOR_HEIGHT_MIN ||
            message->first > EDITOR_EXTENT_MAX || message->second > EDITOR_EXTENT_MAX || message->size ||
            !apply_canvas((unsigned int)message->first, (unsigned int)message->second))
        {
            return 0;
        }
        canvas_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_GUTTER)
    {
        if (assembling || message->flags != style_revision || message->size || message->second || message->first < 0 ||
            message->first > 1)
        {
            return 0;
        }
        line_numbers = message->first != 0;
        gutter_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_STYLE)
    {
        if (assembling || message->flags != canvas_revision || message->size || message->first < EDITOR_FONT_MIN ||
            message->first > EDITOR_FONT_MAX || message->second < 1 || message->second > 128 ||
            !apply_font_size((int)message->first) || message->second != dvm_editor_rows())
        {
            return 0;
        }
        style_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_PANEL)
    {
        if (assembling || message->first < 0 || message->first > EDITOR_PANEL_EXPORT_PATH || message->second < 0 ||
            message->second > 1 || message->size > sizeof(panel_text))
        {
            return 0;
        }
        panel_mode = (unsigned long)message->first;
        panel_selected = (int)message->second;
        panel_revision = message->flags;
        panel_size = message->size;
        memcpy(panel_text, message->text, panel_size);
        return 1;
    }
    if (message->code == EDITOR_PANEL_SELECTION)
    {
        size_t scalars;
        if (assembling || message->size || message->flags != panel_revision || message->first < 0 ||
            message->second < 0 || fvm_utf8_validate(panel_text, panel_size, &scalars) != FVM_UTF8_OK ||
            (unsigned long)message->first > scalars || (unsigned long)message->second > scalars ||
            (!panel_mode && (message->first || message->second)))
        {
            return 0;
        }
        panel_caret = message->first;
        panel_anchor = message->second;
        panel_selection_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_VIEWPORT)
    {
        unsigned long top;
        unsigned long scalars;
        unsigned long ticket;
        if (assembling || message->size != 24 || message->first < 0 || message->second < message->first ||
            !hexword(message->text, &top) || !hexword(message->text + 8, &scalars) ||
            !hexword(message->text + 16, &ticket) || scalars > 1048576UL || (unsigned long)message->second > scalars ||
            ticket == 0 || ticket > 2147483647UL || ticket <= view_ticket)
        {
            return 0;
        }
        view_first = message->first;
        view_last = message->second;
        view_total = scalars;
        view_top = top;
        view_ticket = ticket;
        view_revision = message->flags;
        return 1;
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
            (unsigned long)message->second > scalars || scalars != view_total || revision != view_revision ||
            revision != canvas_revision || revision != style_revision || revision != panel_revision ||
            revision != panel_selection_revision || revision != gutter_revision || !view_ticket)
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
            printf("WINDOW %lu %ld %ld %lu\n", view_ticket, view_first, view_last, view_top);
            printf("GUTTER %d %u %lu\n", line_numbers, gutter_width, view_top + 1);
            printf("STYLE %d %d\n", font_size, dvm_editor_rows());
            printf("CANVAS %u %u\n", canvas_width, canvas_height);
            printf("FIELD %ld %ld %.0f\n", panel_caret, panel_anchor, panel_horizontal);
            printf("PANEL %lu %d ", panel_mode, panel_selected);
            for (index = 0; index < panel_size; index++)
            {
                printf("%02x", (unsigned char)panel_text[index]);
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

/* Convert one actual glyph-layout hit into a source scalar proposal.
 * Coordinates are borrowed pixel values. Returns -1 if no completed layout is available. */
static long hit_position(double x, double y)
{
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_TextLayoutLine line;
    size_t index;
    size_t selected = 0;
    size_t byte;
    long position = view_first;
    double distance;
    if (!hit_layout || fvm_text_layout_metrics(hit_layout, &metrics) != FVM_OK || !metrics.line_count ||
        fvm_font_metrics(layout_font, &font) != FVM_OK)
    {
        return -1;
    }
    if (x < 0 || y < 0 || y >= canvas_height - panel_height(line_step(&font)))
    {
        return -1;
    }
    y -= 2;
    x += horizontal - 2 - gutter_width;
    for (index = 0; index < metrics.line_count; index++)
    {
        if (fvm_text_layout_line(hit_layout, index, &line) != FVM_OK)
        {
            return -1;
        }
        selected = index;
        if (y < line.baseline - font.ascent + line_step(&font))
        {
            break;
        }
    }
    if (fvm_text_layout_line(hit_layout, selected, &line) != FVM_OK)
    {
        return -1;
    }
    byte = line.byte_end;
    distance = fabs(x - (line.x + line.advance));
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        double candidate;
        if (fvm_text_layout_glyph(hit_layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return -1;
        }
        candidate = fabs(x - glyph.x);
        if (candidate < distance)
        {
            distance = candidate;
            byte = glyph.shaped.cluster;
        }
    }
    if (byte > display_size)
    {
        return -1;
    }
    for (index = 0; index < byte; index++)
    {
        if (((unsigned char)snapshot[display_start + index] & 0xc0U) != 0x80U)
        {
            position++;
        }
    }
    return position;
}

/* Propose a scalar in the completed single-line field using its actual label/scroll layout.
 * Coordinates are native pixels. Returns -1 outside the footer or before a valid completed layout. */
static long field_hit(double x, double y)
{
    FVM_FontMetrics font;
    FVM_TextLayoutLine line;
    FVM_TextLayoutMetrics layout_metrics;
    size_t byte;
    size_t index;
    long scalar = 0;
    double distance;
    if (!panel_mode || !panel_layout || fvm_font_metrics(layout_font, &font) != FVM_OK ||
        y < canvas_height - panel_extent(line_step(&font)) || y >= canvas_height ||
        fvm_text_layout_line(panel_layout, 0, &line) != FVM_OK)
    {
        return -1;
    }
    if (panel_mode == EDITOR_PANEL_BROWSER)
    {
        double local_y = y - (canvas_height - panel_extent(line_step(&font)) + 8);
        unsigned int row;
        if (local_y < 0 || fvm_text_layout_metrics(panel_layout, &layout_metrics) != FVM_OK)
        {
            return -1;
        }
        row = (unsigned int)floor(local_y / line_step(&font));
        if (row >= layout_metrics.line_count || fvm_text_layout_line(panel_layout, row, &line) != FVM_OK)
        {
            return -1;
        }
    }
    x += panel_horizontal - 10;
    byte = line.byte_end;
    distance = fabs(x - (line.x + line.advance));
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        double candidate;
        if (fvm_text_layout_glyph(panel_layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return -1;
        }
        candidate = fabs(x - glyph.x);
        if (candidate < distance)
        {
            distance = candidate;
            byte = glyph.shaped.cluster;
        }
    }
    if (byte <= panel_prefix)
    {
        return 0;
    }
    byte -= panel_prefix;
    if (byte > panel_size)
    {
        return -1;
    }
    for (index = 0; index < byte; index++)
    {
        if (((unsigned char)panel_text[index] & 0xc0U) != 0x80U)
        {
            scalar++;
        }
    }
    return scalar;
}

/* Resolve a copied raw pointer event using only the completed actual glyph layout.
 * message is replaced with a value request carrying revision and view ticket; Dis decides policy. */
static void resolve_pointer(DVM_UIMessage *message)
{
    if (message->kind != DVM_UI_POINTER)
    {
        return;
    }
    if (dvm_desktop_active())
    {
        char metadata[17];
        unsigned long phase = message->flags;
        unsigned long code = message->code;
        message->kind = DVM_UI_REQUEST;
        message->id = 1;
        message->flags = dvm_desktop_ticket();
        if (code == EDITOR_RESIZE)
        {
            message->code = DVM_DESK_RESIZE;
            message->size = 0;
            return;
        }
        message->code = DVM_DESK_POINTER;
        message->first = message->first < 0 ? -1 : message->first / 20;
        message->second = message->second < 0 ? -1 : message->second / 20;
        snprintf(metadata, sizeof(metadata), "%08lx%08lx", phase, code);
        memcpy(message->text, metadata, 16);
        message->size = 16;
        return;
    }
    if (message->code == EDITOR_RESIZE)
    {
        int rows = 0;
        char metadata[17];
        if (message->first >= EDITOR_WIDTH_MIN && message->first <= EDITOR_EXTENT_MAX &&
            message->second >= EDITOR_HEIGHT_MIN && message->second <= EDITOR_EXTENT_MAX)
        {
            rows = rows_for_canvas(font_size, (unsigned int)message->second);
        }
        message->kind = DVM_UI_REQUEST;
        message->id = 1;
        message->flags = view_revision;
        message->size = 16;
        snprintf(metadata, sizeof(metadata), "%08lx%08x", view_ticket, (unsigned int)rows);
        memcpy(message->text, metadata, 16);
        return;
    }
    {
        FVM_KeyboardState state;
        int wheel = message->code == FVM_MOUSE_WHEEL_UP || message->code == FVM_MOUSE_WHEEL_DOWN;
        int pinch = message->code == EDITOR_PINCH;
        int control;
        memset(&state, 0, sizeof(state));
        state.struct_size = sizeof(state);
        control = fvm_input_get_keyboard(context, &state) == FVM_OK && state.down[FVM_KEY_CONTROL];
        if ((wheel && control) || pinch)
        {
            int delta = pinch ? (message->first == 1    ? 1
                                 : message->first == -1 ? -1
                                                        : 0)
                              : (message->code == FVM_MOUSE_WHEEL_UP ? 1 : -1);
            int target = font_size + delta;
            int rows;
            if (target < EDITOR_FONT_MIN)
            {
                target = EDITOR_FONT_MIN;
            }
            if (target > EDITOR_FONT_MAX)
            {
                target = EDITOR_FONT_MAX;
            }
            rows = rows_for_canvas(target, canvas_height);
            if (!rows)
            {
                message->first = 0;
                message->second = 0;
            }
            else
            {
                message->first = delta;
                message->second = rows;
            }
            message->code = EDITOR_ZOOM;
        }
    }
    if (message->code == EDITOR_ZOOM)
    {
        /* Geometry was only measured above; Dis still validates and chooses its size. */
    }
    else if (message->code == FVM_MOUSE_WHEEL_UP || message->code == FVM_MOUSE_WHEEL_DOWN)
    {
        message->first = message->code == FVM_MOUSE_WHEEL_UP ? -3 : 3;
        message->second = 0;
        message->code = EDITOR_SCROLL;
    }
    else if (message->code == FVM_MOUSE_LEFT)
    {
        message->first = panel_mode ? field_hit((double)message->first / 20, (double)message->second / 20)
                                    : hit_position((double)message->first / 20, (double)message->second / 20);
        message->second = (long)message->flags;
        message->code = panel_mode ? EDITOR_FIELD_PLACE : EDITOR_PLACE;
    }
    else
    {
        return;
    }
    message->kind = DVM_UI_REQUEST;
    message->id = 1;
    message->flags = view_revision;
    message->size = 8;
    {
        char ticket[9];
        snprintf(ticket, sizeof(ticket), "%08lx", view_ticket);
        memcpy(message->text, ticket, 8);
    }
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
    case SDLK_TAB:
        return 9;
    case SDLK_RETURN:
        return FVM_KEY_ENTER;
    case SDLK_ESCAPE:
        return FVM_KEY_ESCAPE;
    case SDLK_F1:
        return FVM_KEY_F1;
    case SDLK_F7:
        return FVM_KEY_F1 + 6;
    case SDLK_F8:
        return FVM_KEY_F1 + 7;
    case SDLK_F3:
        return FVM_KEY_F1 + 2;
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
    if (!collect_frame(0))
    {
        return -1;
    }
    if (raster_pending)
    {
        return 0; /* Keep OS input queued until displayed pixels match owner hit geometry. */
    }
    if (!complete)
    {
        return 0;
    }
    memset(message, 0, sizeof(*message));
    if (clipboard_event(message))
    {
        complete = 0;
        return 1;
    }
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
        if (message->kind == DVM_UI_KEY && message->code >= 1 && message->code < FVM_KEYBOARD_CODE_COUNT &&
            message->flags <= 1)
        {
            FVM_KeyboardState state;
            memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            if (fvm_input_get_keyboard(context, &state) != FVM_OK)
            {
                return -1;
            }
            state.down[message->code] = message->flags != 0;
            state.last_code = (unsigned int)message->code;
            state.last_ascii = 0;
            if (fvm_input_set_keyboard(context, &state) != FVM_OK)
            {
                return -1;
            }
        }
        if (message->kind == DVM_UI_FOCUS && !message->flags)
        {
            pinch_distance = 0;
            if (fvm_input_reset_keyboard(context) != FVM_OK)
            {
                return -1;
            }
        }
        resolve_pointer(message);
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
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED && window &&
            event.window.windowID == SDL_GetWindowID(window))
        {
            return render() ? 0 : -1; /* Restore cached presentation without asking Dis to edit/rebuild a view. */
        }
        if (dvm_editor_window_event(&event, window, message))
        {
            if (message->code == EDITOR_RESIZE)
            {
                if (message->first == (long)canvas_width && message->second == (long)canvas_height)
                {
                    return 0;
                }
                pinch_distance = 0;
            }
        }
        else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP)
        {
            pinch_distance = 0;
            return 0;
        }
        else if (event.type == SDL_MULTIGESTURE)
        {
            if (event.mgesture.numFingers < 2 || !(event.mgesture.dDist >= -1 && event.mgesture.dDist <= 1))
            {
                pinch_distance = 0;
                return 0;
            }
            pinch_distance += event.mgesture.dDist;
            if (fabs(pinch_distance) < EDITOR_PINCH_THRESHOLD)
            {
                return 0;
            }
            message->kind = DVM_UI_POINTER;
            message->code = EDITOR_PINCH;
            message->first = pinch_distance > 0 ? 1 : -1;
            pinch_distance = 0;
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
            if (!event.wheel.y)
            {
                return 0;
            }
            message->kind = DVM_UI_POINTER;
            message->code = event.wheel.y > 0 ? FVM_MOUSE_WHEEL_UP : FVM_MOUSE_WHEEL_DOWN;
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            {
                message->code = event.wheel.y > 0 ? FVM_MOUSE_WHEEL_DOWN : FVM_MOUSE_WHEEL_UP;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
                 (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)))
        {
            if ((event.type == SDL_MOUSEMOTION && event.motion.which == SDL_TOUCH_MOUSEID) ||
                (event.type != SDL_MOUSEMOTION && event.button.which == SDL_TOUCH_MOUSEID))
            {
                return 0;
            }
            double x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
            double y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
            FVM_InputPhase phase = event.type == SDL_MOUSEBUTTONDOWN ? FVM_INPUT_BEGIN
                                   : event.type == SDL_MOUSEBUTTONUP ? FVM_INPUT_END
                                                                     : FVM_INPUT_UPDATE;
            if (event.type != SDL_MOUSEMOTION && event.button.button != SDL_BUTTON_LEFT)
            {
                return 0;
            }
            if (fvm_input_set_pointer(context, x, y) != FVM_OK ||
                dvm_ui_capture_pointer(context, FVM_MOUSE_LEFT, phase, message) != FVM_OK)
            {
                return -1;
            }
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
            pinch_distance = 0;
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
        resolve_pointer(message);
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
    /* [GC:] Join all native callbacks before destroying fonts or either VM. */
    fvm_render_queue_destroy(raster_queue);
    raster_queue = NULL;
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
    if (panel_field && context)
    {
        fvm_gc_unpin(context, panel_field);
    }
    if (field && context)
    {
        fvm_gc_unpin(context, field);
    }
    fvm_text_layout_destroy(hit_layout);
    fvm_text_layout_destroy(panel_layout);
    fvm_font_destroy(layout_font);
    dvm_desktop_destroy();
    desktop_bound = 0;
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    free(snapshot);
    free(clipboard_text);
    free(clipboard_transfer);
    free(pixels);
    free(font_bytes);
    free(default_font_path);
}
