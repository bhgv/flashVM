/* Actual MovieClip/AS widget presentation for copied Limbo desktop scenes. */
#include "desktop_view.h"
#include "desktop_view_swf.h"
#include "flashvm/avm1.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/standard_text.h"
#include "flashvm/text_layout.h"
#include "flashvm/gc.h"
#include "flashvm/utf8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DESK_SLOTS 64U  /* Stable bounded widget identities, independent of VM addresses. */
#define DESK_TEXT 1024U /* Maximum copied UTF-8 payload. */
#define DESK_META 72U   /* parent/x/y/width/height/style/caret/mirror/anchor words. */


/* One immutable copied scene item and its persistent owner-local capabilities. */
typedef struct DeskItem
{
    unsigned long id;     /* Stable controller slot. */
    unsigned long parent; /* Layer group 0..4. */
    unsigned long x;      /* Parent-local pixels. */
    unsigned long y;      /* Parent-local pixels. */
    unsigned long width;  /* Bounded local extent. */
    unsigned long height; /* Bounded local extent. */
    unsigned long mirror; /* Optional source window snapshot in this scene. */
    unsigned long line;   /* One-based logical source line, zero for non-editor panes. */
    unsigned long style;  /* Palette 0..6; line origin is decoded separately. */
    long anchor;          /* Clipped Unicode selection endpoint, -1 hides selection. */
    long caret;           /* Unicode position, -1 disables the caret. */
    char text[DESK_TEXT]; /* Owned counted text, not evaluated. */
    size_t size;          /* Valid bytes in text. */
} DeskItem;

static FVM_Context *owner;                /* Borrowed FlashVM context; never used on raster workers. */
static FVM_Object *root;                  /* [GC:] Pinned MovieClip root retaining all scene children. */
static FVM_Object *groups[5];             /* [GC:] Borrowed children of the retained root. */
static FVM_Object *clips[DESK_SLOTS];     /* [GC:] Borrowed stable MovieClip children. */
static FVM_Object *marks[DESK_SLOTS];     /* [GC:] Selection MovieClips retained beneath widgets. */
static FVM_Object *cursors[DESK_SLOTS];   /* [GC:] Caret MovieClips retained beneath each widget. */
static FVM_Object *labels[DESK_SLOTS];    /* [GC:] TextFields retained on their MovieClips. */
static unsigned char visible[DESK_SLOTS]; /* Slots admitted in the previous complete scene. */
static unsigned long parents[DESK_SLOTS]; /* Immutable parent identity after creation. */
static FVM_Font *face;                    /* Owner-only native face for caret geometry. */
static DeskItem items[DESK_SLOTS];        /* Copied staged descriptors. */
static unsigned int expected;             /* Exact declared item count. */
static unsigned int received;             /* Complete staged descriptors. */
static unsigned long ticket;              /* Committed view token. */
static unsigned long tablezoom = 100;     /* Uniform scale of the table MovieClip only. */
static unsigned long offset;              /* Table scroll magnitude. */
static unsigned int width;                /* Committed canvas pixels. */
static unsigned int height;               /* Committed canvas pixels. */
static int pending;                       /* A declared scene is being assembled. */
static int trace; /* Explicit replay diagnostic mode; ordinary windows do not dump source text. */
static FVM_Path *text_paths[DESK_SLOTS];       /* Owner-local untransformed glyph outlines. */
static DeskItem text_keys[DESK_SLOTS];         /* Immutable layout keys; position is deliberately excluded. */
static unsigned char *drag_pixels[DESK_SLOTS]; /* Owned bounded complete window snapshots. */
static DeskItem drag_keys[DESK_SLOTS];         /* Content/geometry keys for copied drag pixels. */
static int active;                             /* A desktop scene has been admitted. */


static DVM_EditorFrame *prepared_frame; /* Borrowed owner-private capture destination during preparation. */
static int capture_left;                /* Current inclusive canvas capture clip. */
static int capture_top;                 /* Current inclusive canvas capture clip. */
static int capture_right;               /* Current exclusive canvas capture clip. */
static int capture_bottom;              /* Current exclusive canvas capture clip. */


/* Copy a borrowed world-space contour; callback never enters either VM. */
static FVM_Result
capture_path(void *unused, const FVM_Path *path, FVM_RasterColor color, double width_value, int even_odd)
{
    (void)unused;
    return dvm_editor_frame_command(prepared_frame,
                                    path,
                                    color,
                                    width_value,
                                    even_odd,
                                    capture_left,
                                    capture_top,
                                    capture_right,
                                    capture_bottom)
               ? FVM_OK
               : FVM_OUT_OF_MEMORY;
}


/* Capture a solid MovieClip with a copied viewport, or use legacy owner rendering.
 * Both paths retain ordinary Flash geometry and painter ordering. */
static FVM_Result draw_clip(FVM_Object *clip, FVM_RasterSurface *surface, const FVM_Affine *view)
{
    FVM_Affine identity = {1, 0, 0, 1, 0, 0};
    if (prepared_frame)
    {
        return fvm_movieclip_capture_paths(owner, clip, view ? view : &identity, capture_path, NULL);
    }
    return view ? fvm_movieclip_render_view(owner, clip, surface, view) : fvm_movieclip_render(owner, clip, surface);
}


/* Decode one canonical hex word from a checked eight-byte span; output on success. */
static int word(const char *text, unsigned long *output)
{
    unsigned int index;
    unsigned long value = 0;
    for (index = 0; index < 8; ++index)
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
        value = value * 16 + digit;
    }
    *output = value;
    return 1;
}


/* Set one copied number on a same-context object; no pointer leaves this owner. */
static int number(FVM_Object *object, const char *name, double value)
{
    FVM_Value input;
    fvm_value_number(&input, value);
    return fvm_object_set(owner, object, name, &input) == FVM_OK;
}


/* Bind a retained context-local capability as an AS global, never a Dis value. */
static int binding(const char *name, FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return fvm_set_global(owner, name, &value) == FVM_OK;
}


/* Create a genuine MovieClip through the standard Flash method and parent edge.
 * parent/name/depth are borrowed; returned object stays rooted through that edge. */
static FVM_Object *child(FVM_Object *parent, const char *name, unsigned int depth)
{
    FVM_Value args[2];
    FVM_Value result;
    fvm_value_string(&args[0], name);
    fvm_value_number(&args[1], depth);
    if (fvm_object_call(owner, parent, "createEmptyMovieClip", 2, args, &result) != FVM_OK ||
        result.type != FVM_VALUE_OBJECT)
    {
        return NULL;
    }
    return result.object;
}


/* Build one solid drawing using actual MovieClip drawing methods. The copied
 * palette/extent are values; output is FlashVM-managed graphics, not a C mock. */
static int box(FVM_Object *clip, double wide, double high, unsigned long rgb, unsigned long border)
{
    FVM_Value args[3];
    FVM_Value ignored;
    int index;
    double radius = fmin(7, fmin(wide, high) / 4);
    double x[25];
    double y[25];
    /* Six segments per corner keep the outline independent of font/raster jobs. */
    for (index = 0; index < 25; ++index)
    {
        int corner = (index % 24) / 6;
        double angle = ((index % 24) * 15.0 + 180.0) * 3.141592653589793 / 180.0;
        double cx = corner == 0 || corner == 3 ? radius : wide - radius;
        double cy = corner < 2 ? radius : high - radius;
        x[index] = cx + radius * cos(angle);
        y[index] = cy + radius * sin(angle);
    }
    if (fvm_object_call(owner, clip, "clear", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    {
        fvm_value_number(&args[0], border ? 2 : 0);
        fvm_value_number(&args[1], border);
        fvm_value_number(&args[2], 100);
        if (fvm_object_call(owner, clip, "lineStyle", 3, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    fvm_value_number(&args[0], rgb);
    fvm_value_number(&args[1], 100);
    if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (index = 0; index < 25; ++index)
    {
        fvm_value_number(&args[0], x[index]);
        fvm_value_number(&args[1], y[index]);
        if (fvm_object_call(owner, clip, index ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}


/* Append one value rectangle to a retained MovieClip without clearing siblings.
 * Coordinates/extents/color are copied pixels; return zero on a VM call failure.
 * The caller owns the clip's context and serializes all managed access. */
static int rectangle(FVM_Object *clip, double left, double top, double wide, double high)
{
    FVM_Value args[2];
    FVM_Value ignored;
    double x[5] = {left, left + wide, left + wide, left, left};
    double y[5] = {top, top, top + high, top + high, top};
    int index;
    fvm_value_number(&args[0], 0x95b9d1UL);
    fvm_value_number(&args[1], 100);
    if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (index = 0; index < 5; ++index)
    {
        fvm_value_number(&args[0], x[index]);
        fvm_value_number(&args[1], y[index]);
        if (fvm_object_call(owner, clip, index ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}


/* Initialize owner-local scene state and linked AS bytecode once. The root pins
 * all child capabilities; partial startup is retired by destroy. Return success. */
int dvm_desktop_bind(FVM_Context *context, const unsigned char *font, size_t size, int replay)
{
    unsigned int index;
    FVM_Result result;
    owner = context;
    trace = replay;
    if (!owner || !font || !size)
    {
        return 0;
    }
    result = fvm_load_memory(owner, ide_view_swf, sizeof(ide_view_swf));
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop load: %d\n", result);
        return 0;
    }
    result = fvm_standard_install(owner);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop standard: %d\n", result);
        return 0;
    }
    result = fvm_movieclip_create(owner, "Desktop", 1, &root);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop root: %d\n", result);
        return 0;
    }
    result = fvm_gc_pin(owner, root);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop root pin: %d\n", result);
        return 0;
    }
    result = fvm_font_read(font, size, 0, 11, &face);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop face: %d\n", result);
        return 0;
    }
    groups[0] = root;
    for (index = 1; index <= 4; ++index)
    {
        char name[24];
        snprintf(name, sizeof(name), "layer%u", index);
        groups[index] = child(root, name, 10 + index * 10);
        if (!groups[index])
        {
            fprintf(stderr, "desktop group %u\n", index);
            return 0;
        }
    }
    return 1;
}


/* Validate complete scalar ranges and UTF-8 before any scene object mutation. */
int dvm_desktop_message(const DVM_UIMessage *message)
{
    unsigned long count;
    unsigned long words[9];
    unsigned int index;
    if (!message || message->kind != DVM_UI_VIEW)
    {
        return 0;
    }
    if (message->code == DVM_DESK_BEGIN)
    {
        if (pending || message->first < 240 || message->first > 2048 || message->second < 160 ||
            message->second > 2048 || message->size != 32 || !word(message->text, &ticket) || !ticket ||
            !word(message->text + 8, &count) || count >= DESK_SLOTS || !word(message->text + 16, &offset) ||
            offset > 4096 || !word(message->text + 24, &tablezoom) || tablezoom < 10 || tablezoom > 180)
        {
            return 0;
        }
        width = (unsigned int)message->first;
        height = (unsigned int)message->second;
        expected = (unsigned int)count;
        received = 0;
        pending = 1;
        return 1;
    }
    if (!pending || message->flags != ticket)
    {
        return 0;
    }
    if (message->code == DVM_DESK_END)
    {
        if (message->size || received != expected)
        {
            return 0;
        }
        memset(items + received, 0, (DESK_SLOTS - received) * sizeof(*items));
        pending = 0;
        active = 1;
        return 1;
    }
    if (message->code != DVM_DESK_ITEM || received >= expected || message->first <= 0 || message->first >= DESK_SLOTS ||
        message->size < DESK_META || message->size > DESK_TEXT)
    {
        return 0;
    }
    for (index = 0; index < 9; ++index)
    {
        if (!word(message->text + index * 8, &words[index]))
        {
            return 0;
        }
    }
    if (words[0] > 4 || words[1] > 65536 || words[2] > 65536 || !words[3] || words[3] > 32768 || !words[4] ||
        words[4] > 32768 || words[5] > 80000005UL || (words[5] % 8) > 6 || words[7] >= DESK_SLOTS)
    {
        return 0;
    }
    for (index = 0; index < received; ++index)
    {
        if (items[index].id == (unsigned long)message->first)
        {
            return 0;
        }
    }
    {
        DeskItem *item = &items[received];
        unsigned long scalars;
        item->size = message->size - DESK_META;
        memset(item->text, 0, sizeof(item->text));
        memcpy(item->text, message->text + DESK_META, item->size);
        if (fvm_utf8_validate(item->text, item->size, &scalars) != FVM_UTF8_OK ||
            (words[6] != 0xffffffffUL && words[6] > scalars) || (words[8] != 0xffffffffUL && words[8] > scalars))
        {
            return 0;
        }
        item->id = (unsigned long)message->first;
        item->parent = words[0];
        item->x = words[1];
        item->y = words[2];
        item->width = words[3];
        item->height = words[4];
        item->style = words[5] % 8;
        item->line = words[5] / 8;
        item->mirror = words[7];
        item->anchor = words[8] == 0xffffffffUL ? -1 : (long)words[8];
        item->caret = words[6] == 0xffffffffUL ? -1 : (long)words[6];
        received++;
    }
    return 1;
}


/* Compose the actual group/local matrices for detached field placement. Both
 * matrices are copied observations; no script state is guessed from wire input. */
static int world_matrix(FVM_Object *clip, unsigned long parent, FVM_Affine *output)
{
    FVM_Affine local;
    FVM_Affine group;
    if (fvm_movieclip_get_matrix(owner, clip, &local) != FVM_OK ||
        fvm_movieclip_get_matrix(owner, groups[parent], &group) != FVM_OK)
    {
        return 0;
    }
    output->xx = group.xx * local.xx + group.xy * local.yx;
    output->xy = group.xx * local.xy + group.xy * local.yy;
    output->yx = group.yx * local.xx + group.yy * local.yx;
    output->yy = group.yx * local.xy + group.yy * local.yy;
    output->x0 = group.xx * local.x0 + group.xy * local.y0 + group.x0;
    output->y0 = group.yx * local.x0 + group.yy * local.y0 + group.y0;
    return 1;
}


/* Build the owner's uniform glyph layout shared by caret, selection and hit
 * measurement. Inputs are borrowed copied text and required output cells.
 * Success transfers one layout to the caller; failure clears it. No script,
 * channel, document or raster worker is involved. Serialize mutable face access. */
static int text_layout(const DeskItem *content,
                       FVM_TextLayout **layout,
                       FVM_FontMetrics *font,
                       FVM_TextLayoutOptions *options,
                       FVM_TextLayoutMetrics *metrics)
{
    *layout = NULL;
    if (fvm_font_metrics(face, font) != FVM_OK)
    {
        return 0;
    }
    memset(options, 0, sizeof(*options));
    options->max_width = content->width > (content->line ? 44U : 16U) ? content->width - (content->line ? 44 : 16) : 1;
    options->line_height = font->ascent + font->descent + 2;
    options->alignment = FVM_TEXT_ALIGN_LEFT;
    {
        FVM_Result result;
        if (memchr(content->text, '\t', content->size))
        {
            double average;
            result = fvm_font_average_width(face, &average);
            if (result == FVM_OK)
            {
                result = fvm_text_layout_create_periodic(
                    face, content->text, content->size, options, FVM_FONT_SHAPE_NO_KERNING, 0, 0, average * 4, layout);
            }
        }
        else
        {
            result = fvm_text_layout_create_configured(
                face, content->text, content->size, options, FVM_FONT_SHAPE_NO_KERNING, layout);
        }
        if (result != FVM_OK || fvm_text_layout_metrics(*layout, metrics) != FVM_OK)
        {
            fvm_text_layout_destroy(*layout);
            *layout = NULL;
            return 0;
        }
    }
    return 1;
}


/* Propose one field-relative Unicode insertion offset from the last complete
 * actual MovieClip scene. x/y are copied canvas pixels; widget/position are
 * required output cells. Return zero for occlusion, headers, rails or failure,
 * preserving document state. Only the VM owner may measure these managed clips;
 * Dis must independently admit ticket, identity, revision and source range. */
int dvm_desktop_hit(double x, double y, unsigned long *widget, long *position)
{
    unsigned int ordinal;
    if (!widget || !position)
    {
        return 0;
    }
    *widget = 0;
    *position = -1;
    if (!owner || !active || pending || !isfinite(x) || !isfinite(y) || x < 80 || x >= width - 104 || y < 48 ||
        y >= height)
    {
        return 0;
    }
    for (ordinal = received; ordinal > 0; --ordinal)
    {
        DeskItem *item = &items[ordinal - 1];
        FVM_Affine world;
        double determinant;
        double localx;
        double localy;
        if (!world_matrix(clips[item->id], item->parent, &world))
        {
            return 0;
        }
        determinant = world.xx * world.yy - world.xy * world.yx;
        if (fabs(determinant) < 0.000001)
        {
            return 0;
        }
        localx = (world.yy * (x - world.x0) - world.xy * (y - world.y0)) / determinant;
        localy = (-world.yx * (x - world.x0) + world.xx * (y - world.y0)) / determinant;
        if (localx < 0 || localy < 0 || localx >= item->width || localy >= item->height)
        {
            continue;
        }
        if (item->id > 8 || item->mirror || localy < 24)
        {
            return 0;
        }
        {
            FVM_TextLayout *layout = NULL;
            FVM_TextLayoutMetrics metrics;
            FVM_TextLayoutOptions options;
            FVM_FontMetrics font;
            FVM_TextLayoutLine line;
            size_t index;
            size_t selected = 0;
            size_t byte;
            double distance;
            long scalar = 0;
            if (!text_layout(item, &layout, &font, &options, &metrics) || !metrics.line_count)
            {
                fvm_text_layout_destroy(layout);
                return 0;
            }
            localx -= item->line ? 36 : 8;
            localy -= 8;
            for (index = 0; index < metrics.line_count; ++index)
            {
                if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
                {
                    fvm_text_layout_destroy(layout);
                    return 0;
                }
                selected = index;
                if (localy < line.baseline - font.ascent + options.line_height)
                {
                    break;
                }
            }
            if (fvm_text_layout_line(layout, selected, &line) != FVM_OK)
            {
                fvm_text_layout_destroy(layout);
                return 0;
            }
            byte = line.byte_end;
            distance = fabs(localx - (line.x + line.advance));
            for (index = 0; index < line.glyph_count; ++index)
            {
                FVM_TextLayoutGlyph glyph;
                double candidate;
                if (fvm_text_layout_glyph(layout, line.glyph_start + index, &glyph) != FVM_OK)
                {
                    fvm_text_layout_destroy(layout);
                    return 0;
                }
                candidate = fabs(localx - glyph.x);
                if (candidate < distance)
                {
                    distance = candidate;
                    byte = glyph.shaped.cluster;
                }
            }
            fvm_text_layout_destroy(layout);
            if (byte > item->size)
            {
                return 0;
            }
            for (index = 0; index < byte; ++index)
            {
                if (((unsigned char)item->text[index] & 0xc0U) != 0x80U)
                {
                    ++scalar;
                }
            }
            *widget = item->id;
            *position = scalar;
            if (trace)
            {
                printf("HIT %lu %lu %ld\n", ticket, item->id, scalar);
            }
            return 1;
        }
    }
    return 0;
}


/* Update a real caret MovieClip using the same face/wrap contract as its field.
 * content is immutable copied text; cursor remains owned beneath its widget. */
static int caret_clip(FVM_Object *cursor, const DeskItem *content)
{
    FVM_Value ignored;
    FVM_TextLayout *layout = NULL;
    FVM_TextLayoutOptions options;
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    size_t byte = 0;
    long scalar = 0;
    size_t index;
    int ok = 1;
    if (fvm_object_call(owner, cursor, "clear", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    if (content->caret < 0)
    {
        return 1;
    }
    while (byte < content->size && scalar < content->caret)
    {
        ++byte;
        while (byte < content->size && ((unsigned char)content->text[byte] & 0xc0U) == 0x80U)
        {
            ++byte;
        }
        ++scalar;
    }
    if (!text_layout(content, &layout, &font, &options, &metrics))
    {
        return 0;
    }
    for (index = 0; index < metrics.line_count; ++index)
    {
        FVM_TextLayoutLine line;
        size_t glyph_index;
        double x;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (byte < line.byte_start || byte > line.byte_end)
        {
            continue;
        }
        x = line.x + line.advance;
        for (glyph_index = 0; glyph_index < line.glyph_count; ++glyph_index)
        {
            FVM_TextLayoutGlyph glyph;
            if (fvm_text_layout_glyph(layout, line.glyph_start + glyph_index, &glyph) != FVM_OK)
            {
                ok = 0;
                break;
            }
            if (glyph.shaped.cluster >= byte)
            {
                x = glyph.x;
                break;
            }
        }
        ok = ok && number(cursor, "_x", (content->line ? 36 : 8) + x) &&
             number(cursor, "_y", 8 + line.baseline - font.ascent) &&
             box(cursor, 1, options.line_height, 0x203747UL, 0);
        break;
    }
    fvm_text_layout_destroy(layout);
    return ok;
}


/* Draw selection through actual MovieClip glyph rectangles. content is borrowed
 * copied presentation; selection is clipped to this viewport, never the source.
 * Return zero on layout/VM failure and destroy the temporary glyph layout before
 * returning. Combining/bidi clusters follow the same glyph contract as hit/caret. */
static int selection_clip(FVM_Object *mark, const DeskItem *content)
{
    FVM_TextLayout *layout = NULL;
    FVM_FontMetrics font;
    FVM_TextLayoutOptions options;
    FVM_TextLayoutMetrics metrics;
    size_t first = 0;
    size_t last = 0;
    size_t index;
    long scalar = 0;
    long lo = content->caret < content->anchor ? content->caret : content->anchor;
    long hi = content->caret > content->anchor ? content->caret : content->anchor;
    int ok = 1;
    if (lo < 0 || lo == hi)
    {
        return 1;
    }
    for (index = 0; index < content->size; ++index)
    {
        if (((unsigned char)content->text[index] & 0xc0U) != 0x80U)
        {
            if (scalar == lo)
            {
                first = index;
            }
            if (scalar == hi)
            {
                break;
            }
            ++scalar;
        }
    }
    last = index;
    if (!text_layout(content, &layout, &font, &options, &metrics))
    {
        return 0;
    }
    for (index = 0; index < metrics.glyph_count && ok; ++index)
    {
        FVM_TextLayoutGlyph glyph;
        if (fvm_text_layout_glyph(layout, index, &glyph) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (glyph.shaped.cluster >= first && glyph.shaped.cluster < last)
        {
            double x = glyph.x;
            double wide = glyph.shaped.x_advance;
            if (wide < 0)
            {
                x += wide;
                wide = -wide;
            }
            if (wide > 0)
            {
                ok =
                    rectangle(mark, (content->line ? 36 : 8) + x, 8 + glyph.y - font.ascent, wide, options.line_height);
            }
        }
    }
    fvm_text_layout_destroy(layout);
    return ok;
}


/* Clip field ink and its genuine caret through a private bounded raster tile.
 * The tile copies existing pixels and is released before return; no unmanaged
 * buffer is shared with a VM or worker. Matrix comes from the actual MovieClip. */
static int field_ink(FVM_Path *path,
                     FVM_Object *cursor,
                     FVM_Object *mark,
                     const FVM_Affine *world,
                     const DeskItem *item,
                     const DeskItem *content,
                     FVM_RasterSurface *surface,
                     FVM_RasterColor ink,
                     int header)
{
    FVM_RasterSurface tile;
    FVM_Affine shift;
    int left = (int)floor(world->x0);
    int top = (int)floor(world->y0);
    int right = (int)ceil(world->x0 + world->xx * content->width);
    int bottom = (int)ceil(world->y0 + world->yy * content->height);
    int row;
    int ok;
    if (header && bottom > (int)ceil(world->y0 + world->yy * 24))
    {
        bottom = (int)ceil(world->y0 + world->yy * 24);
    }
    if (left < 0)
    {
        left = 0;
    }
    if (top < 0)
    {
        top = 0;
    }
    if (right > (int)surface->width)
    {
        right = (int)surface->width;
    }
    if (bottom > (int)surface->height)
    {
        bottom = (int)surface->height;
    }
    if (item->parent == 1 && top < 48)
    {
        top = 48;
    }
    if (right <= left || bottom <= top)
    {
        return 1;
    }
    if (prepared_frame)
    {
        int ok;
        FVM_Affine identity = {1, 0, 0, 1, 0, 0};
        capture_left = left;
        capture_top = top;
        capture_right = right;
        capture_bottom = bottom;
        ok = draw_clip(mark, surface, &identity) == FVM_OK && capture_path(NULL, path, ink, 0, 0) == FVM_OK &&
             draw_clip(cursor, surface, &identity) == FVM_OK;
        capture_left = capture_top = 0;
        capture_right = (int)surface->width;
        capture_bottom = (int)surface->height;
        return ok;
    }
    tile.width = (unsigned int)(right - left);
    tile.height = (unsigned int)(bottom - top);
    tile.stride = (size_t)tile.width * 4;
    tile.pixels = malloc(tile.stride * tile.height);
    if (!tile.pixels)
    {
        return 0;
    }
    for (row = 0; row < (int)tile.height; ++row)
    {
        memcpy(
            tile.pixels + row * tile.stride, surface->pixels + (top + row) * surface->stride + left * 4, tile.stride);
    }
    memset(&shift, 0, sizeof(shift));
    shift.xx = shift.yy = 1;
    shift.x0 = -left;
    shift.y0 = -top;
    ok = fvm_movieclip_render_view(owner, mark, &tile, &shift) == FVM_OK &&
         fvm_path_transform(path, &shift) == FVM_OK && fvm_path_draw(path, &tile, ink, 0, 0) == FVM_OK &&
         fvm_movieclip_render_view(owner, cursor, &tile, &shift) == FVM_OK;
    if (ok)
    {
        for (row = 0; row < (int)tile.height; ++row)
        {
            memcpy(surface->pixels + (top + row) * surface->stride + left * 4,
                   tile.pixels + row * tile.stride,
                   tile.stride);
        }
    }
    free(tile.pixels);
    return ok;
}


/* Draw logical source numbers beside the copied viewport. content/world are
 * borrowed owner values; no document is read. Wrapped continuations are blank,
 * and clipping suppresses labels crossing the window header or lower edge. */
static int gutter(const DeskItem *content, const FVM_Affine *world, FVM_RasterSurface *surface)
{
    FVM_TextLayout *layout = NULL;
    FVM_TextLayoutOptions options;
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_RasterColor ink = {100, 111, 126, 255};
    size_t index;
    size_t previous = 0;
    unsigned long number_value = content->line;
    unsigned long last_number = 0;
    int ok = 1;
    if (!number_value)
    {
        return 1;
    }
    if (!text_layout(content, &layout, &font, &options, &metrics))
    {
        return 0;
    }
    for (index = 0; index < metrics.line_count && ok; ++index)
    {
        FVM_TextLayoutLine line;
        FVM_TextRun *run = NULL;
        FVM_Path *path = NULL;
        FVM_Affine placement = *world;
        FVM_Result result;
        double advance = 0;
        double vertical = 0;
        char label[32];
        size_t byte;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (!index)
        {
            previous = line.byte_end;
            continue; /* The first copied line is the caption, never source. */
        }
        for (byte = previous; byte < line.byte_start; ++byte)
        {
            if (content->text[byte] == '\n' && index > 1)
            {
                ++number_value;
            }
        }
        previous = line.byte_start;
        if (number_value == last_number)
        {
            continue;
        }
        last_number = number_value;
        placement.x0 += world->xx * 8;
        placement.y0 += world->yy * (line.baseline + 8);
        if (placement.y0 - font.ascent < 48 || placement.y0 > surface->height ||
            placement.y0 > world->y0 + world->yy * content->height - 4)
        {
            continue;
        }
        snprintf(label, sizeof(label), "%lu", number_value);
        result =
            fvm_font_shape_configured(face, label, strlen(label), FVM_TEXT_LTR, 0, FVM_FONT_SHAPE_NO_KERNING, &run);
        if (result == FVM_OK)
        {
            result = fvm_text_run_path(face, run, &path, &advance, &vertical);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, &placement);
        }
        if (result == FVM_OK)
        {
            result = prepared_frame ? capture_path(NULL, path, ink, 0, 0) : fvm_path_draw(path, surface, ink, 0, 0);
        }
        fvm_path_destroy(path);
        fvm_text_run_destroy(run);
        ok = result == FVM_OK;
    }
    fvm_text_layout_destroy(layout);
    return ok;
}


/* Paint one retained widget. AS owns object property placement; the adapter only
 * supplies copied bindings and invokes standard MovieClip/TextField primitives. */
static int paint(DeskItem *item, FVM_RasterSurface *surface)
{
    static const unsigned long colors[7] = {
        0xeff2f1UL, 0xeff2f1UL, 0x26343fUL, 0xffca69UL, 0x405968UL, 0xeff2f1UL, 0xeff2f1UL};
    FVM_Object *clip;
    FVM_Object *label;
    FVM_Value value;
    FVM_Affine placement;
    FVM_Path *path = NULL;
    FVM_RasterColor ink = {30, 40, 48, 255};
    int ok;
    DeskItem *content = item;
    double scale = 1;
    unsigned int index;
    if (item->mirror)
    {
        content = NULL;
        for (index = 0; index < received; ++index)
        {
            if (items[index].id == item->mirror && !items[index].mirror)
            {
                content = &items[index];
            }
        }
        if (!content)
        {
            return 0;
        }
        scale = (double)item->width / content->width;
        if (scale > 1)
        {
            scale = 1;
        }
        if ((double)item->height / content->height < scale)
        {
            scale = (double)item->height / content->height;
        }
    }
    if (!clips[item->id])
    {
        char name[24];
        snprintf(name, sizeof(name), "widget%lu", item->id);
        clips[item->id] = child(groups[item->parent], name, (unsigned int)item->id);
        parents[item->id] = item->parent;
        if (!clips[item->id])
        {
            fprintf(stderr, "desktop child\n");
            return 0;
        }
        {
            FVM_Result result = fvm_textfield_create(owner, "Editor", item->width, item->height, &labels[item->id]);
            if (result != FVM_OK)
            {
                fprintf(stderr, "desktop create field: %d\n", result);
                return 0;
            }
            result = fvm_gc_pin(owner, labels[item->id]);
            if (result != FVM_OK)
            {
                fprintf(stderr, "desktop field pin: %d\n", result);
                return 0;
            }
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = labels[item->id];
        if (fvm_object_set(owner, clips[item->id], "desktopText", &value) != FVM_OK)
        {
            fprintf(stderr, "desktop label edge\n");
            return 0;
        }
        fvm_gc_unpin(owner, labels[item->id]);
        marks[item->id] = child(clips[item->id], "selection", 900);
        if (!marks[item->id])
        {
            return 0;
        }
        cursors[item->id] = child(clips[item->id], "caret", 1000);
        if (!cursors[item->id])
        {
            return 0;
        }
        {
            FVM_Object *format = NULL;
            FVM_Value argument;
            FVM_Value ignored;
            if (fvm_construct(owner, "TextFormat", 0, NULL, &format) != FVM_OK || fvm_gc_pin(owner, format) != FVM_OK)
            {
                return 0;
            }
            ok = number(format, "size", item->parent == 3 && item->id >= 41 ? 9 : 11) && number(format, "leading", 2);
            fvm_value_undefined(&argument);
            argument.type = FVM_VALUE_OBJECT;
            argument.object = format;
            ok = ok && fvm_object_call(owner, labels[item->id], "setNewTextFormat", 1, &argument, &ignored) == FVM_OK;
            fvm_gc_unpin(owner, format);
            if (!ok)
            {
                return 0;
            }
        }
    }
    if (parents[item->id] != item->parent)
    {
        return 0;
    }
    clip = clips[item->id];
    label = labels[item->id];
    {
        FVM_Value ignored;
        if (fvm_object_call(owner, cursors[item->id], "clear", 0, NULL, &ignored) != FVM_OK ||
            fvm_object_call(owner, marks[item->id], "clear", 0, NULL, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    if (!box(clip,
             content->width,
             content->height,
             colors[content->style],
             item->id <= 8 && (content->style == 1 || content->style == 5) ? 0x2684dbUL : 0))
    {
        fprintf(stderr, "desktop box\n");
        return 0;
    }
    if (item->id <= 8 && !rectangle(clip, 2, 2, content->width - 4, 22))
    {
        return 0;
    }
    if (!binding("ideWidget", clip) || !binding("ideLabel", label) || !binding("ideTable", groups[1]) ||
        !binding("ideRight", groups[3]))
    {
        return 0;
    }
#define GLOBAL_NUMBER(name, number_value)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        fvm_value_number(&value, (double)(number_value));                                                              \
        if (fvm_set_global(owner, name, &value) != FVM_OK)                                                             \
        {                                                                                                              \
            return 0;                                                                                                  \
        }                                                                                                              \
    } while (0)
    GLOBAL_NUMBER("ideTableOffset", -(double)offset);
    GLOBAL_NUMBER("ideRightX", width - 104);
    GLOBAL_NUMBER("ideWidgetX", item->x);
    GLOBAL_NUMBER("ideWidgetY", item->y);
    GLOBAL_NUMBER("ideWidgetWidth", content->width);
    GLOBAL_NUMBER("ideWidgetHeight", content->height);
    GLOBAL_NUMBER("ideWidgetScale", scale * 100);
    GLOBAL_NUMBER("ideLabelWidth",
                  content->width > (content->line ? 40U : 12U) ? content->width - (content->line ? 40 : 12) : 1);
    GLOBAL_NUMBER("ideLabelHeight", content->height > 12 ? content->height - 12 : 1);
#undef GLOBAL_NUMBER
    {
        FVM_Result result;
        if (fvm_value_string_bytes(&value, content->text, content->size) != FVM_OK ||
            fvm_set_global(owner, "ideLabelText", &value) != FVM_OK)
        {
            return 0;
        }
        result = fvm_swf_execute_actions(owner);
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop AS: %d\n", result);
            return 0;
        }
        if (!number(groups[1], "_xscale", tablezoom) || !number(groups[1], "_yscale", tablezoom) ||
            !number(groups[1], "_x", 84.0 * (1.0 - tablezoom / 100.0)) ||
            !number(groups[1], "_y", 48.0 * (1.0 - tablezoom / 100.0) - offset))
        {
            return 0;
        }
        if (fvm_object_get(owner, label, "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
            fvm_value_string_size(&value) != content->size || memcmp(value.string, content->text, content->size))
        {
            return 0;
        }
        if (trace)
        {
            size_t byte;
            FVM_Affine actual;
            if (!world_matrix(clip, item->parent, &actual))
            {
                return 0;
            }
            printf("GEOMETRY %lu %lu %lu %lu %lu %lu\n",
                   ticket,
                   item->id,
                   content->width,
                   content->height,
                   content->style,
                   content->line);
            printf("MC %lu %lu %lu %.0f %.0f ", ticket, item->id, item->parent, actual.x0, actual.y0);
            for (byte = 0; byte < content->size; ++byte)
            {
                printf("%02x", (unsigned char)value.string[byte]);
            }
            printf("\n");
            if (item->id <= 8)
            {
                printf("SELECTION %lu %lu %ld %ld\n", ticket, item->id, content->caret, content->anchor);
            }
        }
        /* A minimized pane remains a typed source for its thumbnail. Do not
         * submit its explicit hidden origin to the raster coordinate domain. */
        if (item->id <= 8 && item->parent == 1 && item->x == 65536 && item->y == 65536)
        {
            return 1;
        }
        result = draw_clip(clip, surface, NULL);
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop movie render: %d\n", result);
            return 0;
        }
        /* Moving a clip must not reshape every glyph. These outlines contain
         * no GC pointers and are invalidated by every text/extent change. */
        if (!text_paths[item->id] || text_keys[item->id].width != content->width ||
            text_keys[item->id].height != content->height || text_keys[item->id].size != content->size ||
            memcmp(text_keys[item->id].text, content->text, content->size))
        {
            fvm_path_destroy(text_paths[item->id]);
            text_paths[item->id] = NULL;
            result = fvm_textfield_path(owner, label, &text_paths[item->id]);
            if (result != FVM_OK)
            {
                return 0;
            }
            text_keys[item->id] = *content;
        }
        if (fvm_path_create(&path) != FVM_OK || fvm_path_append(path, text_paths[item->id], NULL) != FVM_OK)
        {
            fvm_path_destroy(path);
            return 0;
        }
        result = world_matrix(clip, item->parent, &placement) ? FVM_OK : FVM_ERROR;
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop transform: %d\n", result);
            return 0;
        }
    }
    {
        FVM_Affine world = placement;
        placement.x0 += placement.xx * (content->line ? 34 : 6) + placement.xy * 6;
        placement.y0 += placement.yx * (content->line ? 34 : 6) + placement.yy * 6;

        if (content->style == 2 || content->style == 4)
        {
            ink.r = 236;
            ink.g = 242;
            ink.b = 244;
        }
        FVM_Path *caption_path = NULL;
        ok = selection_clip(marks[item->id], content) && caret_clip(cursors[item->id], content) &&
             fvm_path_transform(path, &placement) == FVM_OK;
        if (ok && content->line)
        {
            ok = fvm_path_create(&caption_path) == FVM_OK && fvm_path_append(caption_path, path, NULL) == FVM_OK;
        }
        ok = ok && field_ink(path, cursors[item->id], marks[item->id], &world, item, content, surface, ink, 0);
        if (ok && caption_path)
        {
            FVM_RasterColor caption_ink = {176, 36, 48, 255};
            ok = field_ink(
                caption_path, cursors[item->id], marks[item->id], &world, item, content, surface, caption_ink, 1);
        }
        fvm_path_destroy(caption_path);
        ok = ok && gutter(content, &world, surface);
    }
    fvm_path_destroy(path);
    return ok;
}


/* Retire absent scene slots on the FlashVM owner. present is a borrowed complete
 * scene mask; return zero on a VM operation/readback failure. Empty pooled clips
 * stay rooted for bounded reuse, while retired text and drawing are released.
 * No document/channel ownership is changed and no raster worker invokes this. */
static int retire(const unsigned char *present)
{
    unsigned int id;
    for (id = 1; id < DESK_SLOTS; ++id)
    {
        if (visible[id] && !present[id])
        {
            FVM_Value value;
            FVM_Value ignored;
            if (fvm_value_string_bytes(&value, "", 0) != FVM_OK ||
                fvm_object_set(owner, labels[id], "text", &value) != FVM_OK ||
                fvm_object_call(owner, clips[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_call(owner, cursors[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_call(owner, marks[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_get(owner, labels[id], "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
                fvm_value_string_size(&value) != 0)
            {
                return 0;
            }
            free(drag_pixels[id]);
            drag_pixels[id] = NULL;
            fvm_path_destroy(text_paths[id]);
            text_paths[id] = NULL;
            memset(&text_keys[id], 0, sizeof(text_keys[id]));
            memset(&drag_keys[id], 0, sizeof(drag_keys[id]));
            if (trace)
            {
                printf("RETIRED %lu %u\n", ticket, id);
            }
        }
    }
    return 1;
}


/* Composite an unchanged complete window image, with 90% opacity during gestures.
 * The cache owns copied pixels, never VM objects. Return one on a matching image,
 * zero to request normal painting. Current-frame pixels supply the 10% backdrop. */
static int drag_image(const DeskItem *item, FVM_RasterSurface *surface)
{
    const DeskItem *key = &drag_keys[item->id];
    unsigned char *pixels = drag_pixels[item->id];
    int top = (int)item->y - (int)offset;
    int x;
    int y;
    if (tablezoom != 100 || item->id > 8 || item->parent != 1 || !pixels ||
        (key->style != item->style &&
         !((item->style == 5 && key->style == 1) || (item->style == 6 && key->style == 0))) ||
        key->width != item->width || key->height != item->height || key->size != item->size ||
        key->caret != item->caret || key->anchor != item->anchor || key->line != item->line ||
        memcmp(key->text, item->text, item->size))
    {
        return 0;
    }
    if (!number(clips[item->id], "_x", item->x) || !number(clips[item->id], "_y", item->y))
    {
        return 0;
    }
    for (y = 0; y < (int)item->height; ++y)
    {
        if (top + y < 48 || top + y >= (int)surface->height)
        {
            continue;
        }
        for (x = 0; x < (int)item->width; ++x)
        {
            size_t target;
            size_t source;
            unsigned int channel;
            int dx = x < 7 ? 7 - x : (x >= (int)item->width - 7 ? x - ((int)item->width - 8) : 0);
            int dy = y < 7 ? 7 - y : (y >= (int)item->height - 7 ? y - ((int)item->height - 8) : 0);
            if (item->x + x >= surface->width || (dx && dy && dx * dx + dy * dy > 49))
            {
                continue;
            }
            target = (size_t)(top + y) * surface->stride + (item->x + x) * 4;
            source = ((size_t)y * item->width + x) * 4;
            for (channel = 0; channel < 3; ++channel)
            {
                surface->pixels[target + channel] =
                    (unsigned char)(((item->style == 5 || item->style == 6)
                                         ? (pixels[source + channel] * 9 + surface->pixels[target + channel] + 5) / 10
                                         : pixels[source + channel]));
            }
        }
    }
    if (trace)
    {
        size_t byte;
        printf("DRAGCACHE %lu %lu\n", ticket, item->id);
        printf(
            "GEOMETRY %lu %lu %lu %lu %lu %lu\n", ticket, item->id, item->width, item->height, item->style, item->line);
        printf("MC %lu %lu %lu %lu %d ", ticket, item->id, item->parent, item->x, top);
        for (byte = 0; byte < item->size; ++byte)
        {
            printf("%02x", (unsigned char)item->text[byte]);
        }
        printf("\n");
    }
    return 1;
}


/* Retain a bounded complete window image before transient blending. A partial
 * offscreen image is never mistaken for a full snapshot. Allocation failure
 * merely disables the optimization; ordinary painting remains authoritative. */
static void remember_window(const DeskItem *item, const FVM_RasterSurface *surface)
{
    int top = (int)item->y - (int)offset;
    unsigned char *pixels;
    unsigned int y;
    size_t size;
    if (tablezoom != 100 || item->id > 8 || item->parent != 1 || top < 48 || item->x + item->width > surface->width ||
        top + item->height > surface->height)
    {
        return;
    }
    size = (size_t)item->width * item->height * 4;
    if (size > 4U * 1024U * 1024U)
    {
        return;
    }
    pixels = realloc(drag_pixels[item->id], size);
    if (!pixels)
    {
        return;
    }
    drag_pixels[item->id] = pixels;
    for (y = 0; y < item->height; ++y)
    {
        memcpy(pixels + (size_t)y * item->width * 4,
               surface->pixels + (top + y) * surface->stride + item->x * 4,
               item->width * 4);
    }
    drag_keys[item->id] = *item;
}


/* Execute actual FlashVM object drawing on the owner, then hand only copied
 * pixels to detached presentation. No worker accesses managed scene objects. */
int dvm_desktop_draw(FVM_RasterSurface *surface)
{
    unsigned int index;
    unsigned char present[DESK_SLOTS] = {0};
    if (!root || !active || !surface || surface->width != width || surface->height != height)
    {
        return 0;
    }
    for (index = 0; index < received; ++index)
    {
        present[items[index].id] = 1;
    }
    if (!retire(present))
    {
        return 0;
    }
    memset(surface->pixels, 255, surface->stride * surface->height);
    for (index = 0; index < received; ++index)
    {
        unsigned char *behind = NULL;
        DeskItem *item = &items[index];
        if (drag_image(item, surface))
        {
            continue;
        }
        if (item->id <= 8 && (item->style == 5 || item->style == 6))
        {
            behind = malloc(surface->stride * surface->height);
            if (!behind)
            {
                return 0;
            }
            memcpy(behind, surface->pixels, surface->stride * surface->height);
        }
        if (!paint(item, surface))
        {
            free(behind);
            fprintf(stderr, "desktop paint slot=%lu parent=%lu\n", item->id, item->parent);
            return 0;
        }
        remember_window(item, surface);
        if (behind)
        {
            /* The copied background belongs to this frame, never to a VM heap.
             * Blend the complete dragged window by 90%; release before return. */
            int x;
            int y;
            FVM_Affine world;
            int top;
            int left;
            int right;
            int bottom;
            if (!world_matrix(clips[item->id], item->parent, &world))
            {
                free(behind);
                return 0;
            }
            top = (int)floor(world.y0);
            left = (int)floor(world.x0);
            right = (int)ceil(world.x0 + world.xx * item->width);
            bottom = (int)ceil(world.y0 + world.yy * item->height);
            for (y = top < 48 ? 48 : top; y < bottom && y < (int)surface->height; ++y)
            {
                for (x = left < 0 ? 0 : left; x < right && x < (int)surface->width; ++x)
                {
                    size_t byte = (size_t)y * surface->stride + (size_t)x * 4;
                    unsigned int channel;
                    for (channel = 0; channel < 3; ++channel)
                    {
                        surface->pixels[byte + channel] =
                            (unsigned char)((surface->pixels[byte + channel] * 9 + behind[byte + channel] + 5) / 10);
                    }
                }
            }
            free(behind);
        }
    }
    memcpy(visible, present, sizeof(visible));
    if (trace)
    {
        printf("TABLEZOOM %lu %lu\n", ticket, tablezoom);
        printf("DESKTOP %lu %lu %u\n", ticket, offset, received);
    }
    return 1;
}


/* Build detached painter commands from the complete owner-local MovieClip scene.
 * Capture ends before returning, including failures; workers receive no VM state. */
int dvm_desktop_prepare(DVM_EditorFrame *frame)
{
    FVM_RasterSurface surface;
    unsigned char present[DESK_SLOTS] = {0};
    unsigned int index;
    int ok = 1;
    if (!frame || !root || !active || pending || prepared_frame)
    {
        return 0;
    }
    surface = *dvm_editor_frame_surface(frame);
    if (surface.width != width || surface.height != height)
    {
        return 0;
    }
    for (index = 0; index < received; ++index)
    {
        present[items[index].id] = 1;
    }
    if (!retire(present))
    {
        return 0;
    }
    prepared_frame = frame;
    capture_left = capture_top = 0;
    capture_right = (int)width;
    capture_bottom = (int)height;
    for (index = 0; index < received && ok; ++index)
    {
        DeskItem *item = &items[index];
        int translucent = item->id <= 8 && (item->style == 5 || item->style == 6);
        FVM_Affine world;
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        if (translucent)
        {
            double zoom = tablezoom / 100.0;
            double x = item->x * zoom + 84 * (1 - zoom);
            double y = item->y * zoom + 48 * (1 - zoom) - offset;
            left = (int)floor(x);
            top = (int)floor(y);
            right = (int)ceil(x + item->width * zoom);
            bottom = (int)ceil(y + item->height * zoom);
            if (top < 48)
            {
                top = 48;
            }
            ok = dvm_editor_frame_group(frame, 1, left, top, right, bottom);
        }
        ok = ok && paint(item, &surface);
        if (ok && translucent)
        {
            ok = world_matrix(clips[item->id], item->parent, &world) &&
                 dvm_editor_frame_group(frame, 0, left, top, right, bottom);
        }
    }
    prepared_frame = NULL;
    if (ok)
    {
        memcpy(visible, present, sizeof(visible));
        if (trace)
        {
            printf("TABLEZOOM %lu %lu\n", ticket, tablezoom);
            printf("DESKTOP %lu %lu %u\n", ticket, offset, received);
        }
    }
    return ok;
}


/* Hit the visible header in actual paint order. Coordinates are copied canvas
 * pixels; no source or touch policy is acquired. Return zero for occlusion. */
int dvm_desktop_header(double x, double y)
{
    unsigned int ordinal;
    if (!active || pending || !isfinite(x) || !isfinite(y) || y < 48)
    {
        return 0;
    }
    for (ordinal = received; ordinal > 0; --ordinal)
    {
        DeskItem *item = &items[ordinal - 1];
        FVM_Affine world;
        if (!world_matrix(clips[item->id], item->parent, &world))
        {
            return 0;
        }
        if (x >= world.x0 && y >= world.y0 && x < world.x0 + world.xx * item->width &&
            y < world.y0 + world.yy * item->height)
        {
            return item->id <= 8 && y < world.y0 + world.yy * 24;
        }
    }
    return 0;
}


/* Return copied primitive state; neither observation allocates or invokes AS. */
int dvm_desktop_active(void)
{
    return active;
}


unsigned long dvm_desktop_ticket(void)
{
    return ticket;
}


/* [GC:] Drop the root pin while the borrowed context is live; all child/label
 * pointers become invalid after context teardown and must never reach workers. */
void dvm_desktop_destroy(void)
{
    unsigned int index;
    for (index = 0; index < DESK_SLOTS; ++index)
    {
        fvm_path_destroy(text_paths[index]);
        text_paths[index] = NULL;
        free(drag_pixels[index]);
        drag_pixels[index] = NULL;
    }
    memset(text_keys, 0, sizeof(text_keys));
    if (owner && root)
    {
        fvm_gc_unpin(owner, root);
    }
    fvm_font_destroy(face);
    face = NULL;
    root = NULL;
    owner = NULL;
    active = 0;
    pending = 0;
    memset(marks, 0, sizeof(marks));
    memset(cursors, 0, sizeof(cursors));
    memset(clips, 0, sizeof(clips));
    memset(labels, 0, sizeof(labels));
    memset(visible, 0, sizeof(visible));
    memset(items, 0, sizeof(items));
}
