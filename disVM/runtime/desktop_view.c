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
#define DESK_META 64U   /* parent/x/y/width/height/style/caret words. */


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
    unsigned long style;  /* Palette 0..4. */
    long caret;           /* Unicode position, -1 disables the caret. */
    char text[DESK_TEXT]; /* Owned counted text, not evaluated. */
    size_t size;          /* Valid bytes in text. */
} DeskItem;

static FVM_Context *owner;                /* Borrowed FlashVM context; never used on raster workers. */
static FVM_Object *root;                  /* [GC:] Pinned MovieClip root retaining all scene children. */
static FVM_Object *groups[5];             /* [GC:] Borrowed children of the retained root. */
static FVM_Object *clips[DESK_SLOTS];     /* [GC:] Borrowed stable MovieClip children. */
static FVM_Object *cursors[DESK_SLOTS];   /* [GC:] Caret MovieClips retained beneath each widget. */
static FVM_Object *labels[DESK_SLOTS];    /* [GC:] TextFields retained on their MovieClips. */
static unsigned long parents[DESK_SLOTS]; /* Immutable parent identity after creation. */
static FVM_Font *face;                    /* Owner-only native face for caret geometry. */
static DeskItem items[DESK_SLOTS];        /* Copied staged descriptors. */
static unsigned int expected;             /* Exact declared item count. */
static unsigned int received;             /* Complete staged descriptors. */
static unsigned long ticket;              /* Committed view token. */
static unsigned long offset;              /* Table scroll magnitude. */
static unsigned int width;                /* Committed canvas pixels. */
static unsigned int height;               /* Committed canvas pixels. */
static int pending;                       /* A declared scene is being assembled. */
static int trace;  /* Explicit replay diagnostic mode; ordinary windows do not dump source text. */
static int active; /* A desktop scene has been admitted. */


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
static int box(FVM_Object *clip, double wide, double high, unsigned long rgb)
{
    FVM_Value args[2];
    FVM_Value ignored;
    int index;
    double x[5] = {0, wide, wide, 0, 0};
    double y[5] = {0, 0, high, high, 0};
    if (fvm_object_call(owner, clip, "clear", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&args[0], rgb);
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
    unsigned long words[8];
    unsigned int index;
    if (!message || message->kind != DVM_UI_VIEW)
    {
        return 0;
    }
    if (message->code == DVM_DESK_BEGIN)
    {
        if (pending || message->first < 240 || message->first > 2048 || message->second < 160 ||
            message->second > 2048 || message->size != 24 || !word(message->text, &ticket) || !ticket ||
            !word(message->text + 8, &count) || count >= DESK_SLOTS || !word(message->text + 16, &offset) ||
            offset > 4096)
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
        pending = 0;
        active = 1;
        return 1;
    }
    if (message->code != DVM_DESK_ITEM || received >= expected || message->first <= 0 || message->first >= DESK_SLOTS ||
        message->size < DESK_META || message->size > DESK_TEXT)
    {
        return 0;
    }
    for (index = 0; index < 8; ++index)
    {
        if (!word(message->text + index * 8, &words[index]))
        {
            return 0;
        }
    }
    if (words[0] > 4 || words[1] > 4096 || words[2] > 4096 || !words[3] || words[3] > 2048 || !words[4] ||
        words[4] > 2048 || words[5] > 4 || words[7] >= DESK_SLOTS)
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
        memcpy(item->text, message->text + DESK_META, item->size);
        if (fvm_utf8_validate(item->text, item->size, &scalars) != FVM_UTF8_OK ||
            (words[6] != 0xffffffffUL && words[6] > scalars))
        {
            return 0;
        }
        item->id = (unsigned long)message->first;
        item->parent = words[0];
        item->x = words[1];
        item->y = words[2];
        item->width = words[3];
        item->height = words[4];
        item->style = words[5];
        item->mirror = words[7];
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
    if (fvm_font_metrics(face, &font) != FVM_OK)
    {
        return 0;
    }
    memset(&options, 0, sizeof(options));
    options.max_width = content->width > 16 ? content->width - 16 : 1;
    options.line_height = font.ascent + font.descent + 2;
    options.alignment = FVM_TEXT_ALIGN_LEFT;
    {
        FVM_Result result;
        if (memchr(content->text, '\t', content->size))
        {
            double average;
            result = fvm_font_average_width(face, &average);
            if (result == FVM_OK)
            {
                result = fvm_text_layout_create_periodic(face,
                                                         content->text,
                                                         content->size,
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
                face, content->text, content->size, &options, FVM_FONT_SHAPE_NO_KERNING, &layout);
        }
        if (result != FVM_OK || fvm_text_layout_metrics(layout, &metrics) != FVM_OK)
        {
            fvm_text_layout_destroy(layout);
            return 0;
        }
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
        ok = ok && number(cursor, "_x", 8 + x) && number(cursor, "_y", 8 + line.baseline - font.ascent) &&
             box(cursor, 1, options.line_height, 0x203747UL);
        break;
    }
    fvm_text_layout_destroy(layout);
    return ok;
}


/* Clip field ink and its genuine caret through a private bounded raster tile.
 * The tile copies existing pixels and is released before return; no unmanaged
 * buffer is shared with a VM or worker. Matrix comes from the actual MovieClip. */
static int field_ink(FVM_Path *path,
                     FVM_Object *cursor,
                     const FVM_Affine *world,
                     const DeskItem *item,
                     const DeskItem *content,
                     FVM_RasterSurface *surface,
                     FVM_RasterColor ink)
{
    FVM_RasterSurface tile;
    FVM_Affine shift;
    int left = (int)floor(world->x0);
    int top = (int)floor(world->y0);
    int right = (int)ceil(world->x0 + world->xx * content->width);
    int bottom = (int)ceil(world->y0 + world->yy * content->height);
    int row;
    int ok;
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
    ok = fvm_path_transform(path, &shift) == FVM_OK && fvm_path_draw(path, &tile, ink, 0, 0) == FVM_OK &&
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


/* Paint one retained widget. AS owns object property placement; the adapter only
 * supplies copied bindings and invokes standard MovieClip/TextField primitives. */
static int paint(DeskItem *item, FVM_RasterSurface *surface)
{
    static const unsigned long colors[5] = {0xeff2f1UL, 0xd4e7f1UL, 0x26343fUL, 0xffca69UL, 0x405968UL};
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
            ok = number(format, "size", 11) && number(format, "leading", 2);
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
        if (fvm_object_call(owner, cursors[item->id], "clear", 0, NULL, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    if (!box(clip, content->width, content->height, colors[content->style]))
    {
        fprintf(stderr, "desktop box\n");
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
    GLOBAL_NUMBER("ideLabelWidth", content->width > 12 ? content->width - 12 : 1);
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
            printf("MC %lu %lu %lu %.0f %.0f ", ticket, item->id, item->parent, actual.x0, actual.y0);
            for (byte = 0; byte < content->size; ++byte)
            {
                printf("%02x", (unsigned char)value.string[byte]);
            }
            printf("\n");
        }
        result = fvm_movieclip_render(owner, clip, surface);
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop movie render: %d\n", result);
            return 0;
        }
        result = fvm_textfield_path(owner, label, &path);
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop text: %d\n", result);
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
        placement.x0 += placement.xx * 6 + placement.xy * 6;
        placement.y0 += placement.yx * 6 + placement.yy * 6;

        if (content->style == 2 || content->style == 4)
        {
            ink.r = 236;
            ink.g = 242;
            ink.b = 244;
        }
        ok = caret_clip(cursors[item->id], content) && fvm_path_transform(path, &placement) == FVM_OK &&
             field_ink(path, cursors[item->id], &world, item, content, surface, ink);
    }
    fvm_path_destroy(path);
    return ok;
}


/* Execute actual FlashVM object drawing on the owner, then hand only copied
 * pixels to detached presentation. No worker accesses managed scene objects. */
int dvm_desktop_draw(FVM_RasterSurface *surface)
{
    unsigned int index;
    if (!root || !active || !surface || surface->width != width || surface->height != height)
    {
        return 0;
    }
    memset(surface->pixels, 255, surface->stride * surface->height);
    for (index = 0; index < received; ++index)
    {
        if (!paint(&items[index], surface))
        {
            fprintf(stderr, "desktop paint slot=%lu parent=%lu\n", items[index].id, items[index].parent);
            return 0;
        }
    }
    if (trace)
    {
        printf("DESKTOP %lu %lu %u\n", ticket, offset, received);
    }
    return 1;
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
    memset(cursors, 0, sizeof(cursors));
    memset(clips, 0, sizeof(clips));
    memset(labels, 0, sizeof(labels));
}
