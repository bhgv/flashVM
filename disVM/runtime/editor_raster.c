/* Rasterize detached native editor geometry without accessing either interpreter. */
#include "editor_raster.h"
#include "flashvm/render_jobs.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


/* One painter-ordered owned contour; disjoint frames share none of this state. */
typedef struct FramePath
{
    FVM_Path *path;         /* Owned native contour, no font/VM references. */
    FVM_RasterColor color;  /* Copied straight-alpha ink. */
    int group;              /* 0 contour, 1 opacity begin, 2 opacity end. */
    double width;           /* Round stroke width, zero selects a fill. */
    int even_odd;           /* Copied winding rule. */
    int left;               /* Inclusive canvas clip left. */
    int top;                /* Inclusive canvas clip top. */
    int right;              /* Exclusive canvas clip right. */
    int bottom;             /* Exclusive canvas clip bottom. */
    FVM_PathBounds bounds;  /* Conservative antialiased ink bounds in canvas pixels. */
    struct FramePath *next; /* Owned next command. */
} FramePath;


/* A detached frame is mutable only during owner preparation or exclusive rendering. */
struct DVM_EditorFrame
{
    FVM_RasterSurface surface;                       /* Owned RGBA storage, allocated before submission. */
    FramePath *first;                                /* First owned path in paint order. */
    FramePath *last;                                 /* Borrowed tail of the same owned list. */
    size_t paths;                                    /* Count checked against the command budget. */
    size_t verbs;                                    /* Total contour commands checked before transfer. */
    int background;                                  /* Preserve the explicitly copied base for legacy jobs. */
    int incremental;                                 /* Rasterize only dirty tiles with independent outputs. */
    unsigned int columns;                            /* Row-major canvas tile columns. */
    unsigned int rows;                               /* Row-major canvas tile rows. */
    unsigned char dirty[DVM_EDITOR_FRAME_MAX_TILES]; /* Maximum 32 by 32 tiles at the admitted extent. */
    unsigned int work[DVM_EDITOR_FRAME_MAX_TILES];   /* Packed dirty indices, so jobs distribute real work. */
    unsigned int work_count;                         /* Number of dirty tiles, zero requires no worker wakeup. */
    FVM_Result errors[DVM_EDITOR_FRAME_MAX_TILES];   /* One worker-owned status slot per tile. */
};


/* Allocate bounded native storage; width/height are copied pixels, null signals failure. */
DVM_EditorFrame *dvm_editor_frame_create(unsigned int width, unsigned int height)
{
    DVM_EditorFrame *frame;
    if (!width || !height || width > DVM_EDITOR_FRAME_EXTENT || height > DVM_EDITOR_FRAME_EXTENT)
    {
        return NULL;
    }
    frame = calloc(1, sizeof(*frame));
    if (!frame)
    {
        return NULL;
    }
    frame->surface.width = width;
    frame->surface.height = height;
    frame->surface.stride = (size_t)width * 4;
    frame->surface.pixels = malloc(frame->surface.stride * height);
    if (!frame->surface.pixels)
    {
        free(frame);
        return NULL;
    }
    return frame;
}


/* Append an exclusively owned native path, retaining caller ownership on any failure. */
int dvm_editor_frame_path(DVM_EditorFrame *frame, FVM_Path *path, FVM_RasterColor color)
{
    FramePath *entry;
    size_t verbs;
    if (!frame || !path)
    {
        return 0;
    }
    verbs = fvm_path_count(path);
    if (frame->paths == DVM_EDITOR_FRAME_MAX_PATHS || verbs > DVM_EDITOR_FRAME_MAX_VERBS - frame->verbs)
    {
        return 0;
    }
    entry = malloc(sizeof(*entry));
    if (!entry)
    {
        return 0;
    }
    memset(entry, 0, sizeof(*entry));
    if (fvm_path_bounds(path, &entry->bounds) != FVM_OK)
    {
        free(entry);
        return 0;
    }
    entry->left = 0;
    entry->top = 0;
    entry->right = (int)frame->surface.width;
    entry->bottom = (int)frame->surface.height;
    entry->bounds.x_min -= 2;
    entry->bounds.y_min -= 2;
    entry->bounds.x_max += 2;
    entry->bounds.y_max += 2;
    entry->path = path;
    entry->color = color;
    entry->next = NULL;
    if (frame->last)
    {
        frame->last->next = entry;
    }
    else
    {
        frame->first = entry;
    }
    frame->last = entry;
    frame->paths++;
    frame->verbs += verbs;
    return 1;
}


/* Return a borrowed completed surface; caller must enforce queue completion ownership. */
const FVM_RasterSurface *dvm_editor_frame_surface(const DVM_EditorFrame *frame)
{
    return frame ? &frame->surface : NULL;
}


/* Copy a complete owner base while this frame is still private; no VM access. */
int dvm_editor_frame_background(DVM_EditorFrame *frame, const FVM_RasterSurface *source)
{
    unsigned int row;
    if (!frame || !source || !source->pixels || source->width != frame->surface.width ||
        source->height != frame->surface.height || source->stride < source->width * 4U)
    {
        return 0;
    }
    for (row = 0; row < source->height; ++row)
    {
        memmove(frame->surface.pixels + row * frame->surface.stride,
                source->pixels + row * source->stride,
                source->width * 4U);
    }
    frame->background = 1;
    return 1;
}


/* Copy one immutable contour with clipping; no caller or VM resource is retained. */
int dvm_editor_frame_command(DVM_EditorFrame *frame,
                             const FVM_Path *path,
                             FVM_RasterColor color,
                             double width,
                             int even_odd,
                             int left,
                             int top,
                             int right,
                             int bottom)
{
    FVM_Path *copy = NULL;
    FramePath *entry;
    if (!frame || !path || !(width >= 0) || width > DVM_EDITOR_FRAME_EXTENT || (even_odd != 0 && even_odd != 1))
    {
        return 0;
    }
    if (left < 0)
    {
        left = 0;
    }
    if (top < 0)
    {
        top = 0;
    }
    if (right > (int)frame->surface.width)
    {
        right = (int)frame->surface.width;
    }
    if (bottom > (int)frame->surface.height)
    {
        bottom = (int)frame->surface.height;
    }
    if (left >= right || top >= bottom)
    {
        return 1;
    }
    if (fvm_path_create(&copy) != FVM_OK || fvm_path_append(copy, path, NULL) != FVM_OK ||
        !dvm_editor_frame_path(frame, copy, color))
    {
        fvm_path_destroy(copy);
        return 0;
    }
    entry = frame->last;
    entry->width = width;
    entry->even_odd = even_odd;
    entry->left = left;
    entry->top = top;
    entry->right = right;
    entry->bottom = bottom;
    entry->bounds.x_min -= width / 2;
    entry->bounds.y_min -= width / 2;
    entry->bounds.x_max += width / 2;
    entry->bounds.y_max += width / 2;
    return 1;
}


/* Record an opacity boundary with a dummy owned contour for ordinary lifetime
 * management. Geometry is a copied rectangle, never a managed scene reference. */
int dvm_editor_frame_group(DVM_EditorFrame *frame, int begin, int left, int top, int right, int bottom)
{
    FVM_Path *path = NULL;
    FVM_RasterColor color = {0, 0, 0, 0};
    FramePath *entry;
    if (!frame || fvm_path_create(&path) != FVM_OK || !dvm_editor_frame_path(frame, path, color))
    {
        fvm_path_destroy(path);
        return 0;
    }
    entry = frame->last;
    entry->group = begin ? 1 : 2;
    entry->bounds.x_min = left;
    entry->bounds.y_min = top;
    entry->bounds.x_max = right;
    entry->bounds.y_max = bottom;
    return 1;
}


/* Compare exact contours/scalars rather than a lossy hash. Both entries are
 * borrowed immutable native data; return one only for identical painted commands. */
static int same_command(const FramePath *a, const FramePath *b)
{
    size_t index;
    if (a->group != b->group || (a->group && memcmp(&a->bounds, &b->bounds, sizeof(a->bounds))) ||
        a->width != b->width || a->even_odd != b->even_odd || memcmp(&a->color, &b->color, sizeof(a->color)) ||
        a->left != b->left || a->top != b->top || a->right != b->right || a->bottom != b->bottom ||
        fvm_path_count(a->path) != fvm_path_count(b->path))
    {
        return 0;
    }
    for (index = 0; index < fvm_path_count(a->path); ++index)
    {
        FVM_PathCommand x;
        FVM_PathCommand y;
        unsigned int point;
        unsigned int points;
        if (fvm_path_get(a->path, index, &x) != FVM_OK || fvm_path_get(b->path, index, &y) != FVM_OK ||
            x.verb != y.verb)
        {
            return 0;
        }
        points = x.verb == FVM_PATH_CUBIC ? 3 : x.verb == FVM_PATH_QUADRATIC ? 2 : x.verb == FVM_PATH_CLOSE ? 0 : 1;
        for (point = 0; point < points; ++point)
        {
            if (x.x[point] != y.x[point] || x.y[point] != y.y[point])
            {
                return 0;
            }
        }
    }
    return 1;
}


/* Compare only initialized coordinates for one native verb. */
static int same_verb(const FVM_Path *a, size_t ai, const FVM_Path *b, size_t bi)
{
    FVM_PathCommand x;
    FVM_PathCommand y;
    unsigned int index;
    unsigned int points;
    if (fvm_path_get(a, ai, &x) != FVM_OK || fvm_path_get(b, bi, &y) != FVM_OK || x.verb != y.verb)
    {
        return 0;
    }
    points = x.verb == FVM_PATH_CUBIC ? 3 : x.verb == FVM_PATH_QUADRATIC ? 2 : x.verb == FVM_PATH_CLOSE ? 0 : 1;
    for (index = 0; index < points; ++index)
    {
        if (x.x[index] != y.x[index] || x.y[index] != y.y[index])
        {
            return 0;
        }
    }
    return 1;
}


/* Invalidate both old and new ink bounds, intersected with their original clip.
 * frame is owner-private; no worker has access until preparation finishes. */
static void damage_command(DVM_EditorFrame *frame, const FramePath *entry)
{
    int left = (int)floor(entry->bounds.x_min);
    int top = (int)floor(entry->bounds.y_min);
    int right = (int)ceil(entry->bounds.x_max);
    int bottom = (int)ceil(entry->bounds.y_max);
    unsigned int x;
    unsigned int y;
    if (left < entry->left)
    {
        left = entry->left;
    }
    if (top < entry->top)
    {
        top = entry->top;
    }
    if (right > entry->right)
    {
        right = entry->right;
    }
    if (bottom > entry->bottom)
    {
        bottom = entry->bottom;
    }
    if (right <= left || bottom <= top)
    {
        return;
    }
    for (y = (unsigned int)top / DVM_EDITOR_TILE_SIZE; y <= (unsigned int)(bottom - 1) / DVM_EDITOR_TILE_SIZE; ++y)
    {
        for (x = (unsigned int)left / DVM_EDITOR_TILE_SIZE; x <= (unsigned int)(right - 1) / DVM_EDITOR_TILE_SIZE; ++x)
        {
            frame->dirty[y * frame->columns + x] = 1;
        }
    }
}


/* Invalidate only complete contours containing changed verbs. Text outlines
 * retain common glyph prefixes/suffixes, so adding a letter does not dirty all
 * previous lines. Allocation/malformed-path failures conservatively use full ink. */
static void damage_contours(DVM_EditorFrame *frame, const FramePath *entry, size_t first, size_t after)
{
    FVM_PathCommand command;
    FVM_Path *subset = NULL;
    FramePath changed = *entry;
    size_t count = fvm_path_count(entry->path);
    size_t index;
    if (first >= after || first >= count)
    {
        return;
    }
    while (first && fvm_path_get(entry->path, first, &command) == FVM_OK && command.verb != FVM_PATH_MOVE)
    {
        --first;
    }
    while (after < count && fvm_path_get(entry->path, after, &command) == FVM_OK && command.verb != FVM_PATH_MOVE)
    {
        ++after;
    }
    if (fvm_path_create(&subset) != FVM_OK)
    {
        damage_command(frame, entry);
        return;
    }
    for (index = first; index < after; ++index)
    {
        if (fvm_path_get(entry->path, index, &command) != FVM_OK || fvm_path_add(subset, &command) != FVM_OK)
        {
            fvm_path_destroy(subset);
            damage_command(frame, entry);
            return;
        }
    }
    if (fvm_path_bounds(subset, &changed.bounds) == FVM_OK)
    {
        changed.bounds.x_min -= entry->width / 2 + 2;
        changed.bounds.y_min -= entry->width / 2 + 2;
        changed.bounds.x_max += entry->width / 2 + 2;
        changed.bounds.y_max += entry->width / 2 + 2;
        damage_command(frame, &changed);
    }
    else
    {
        damage_command(frame, entry);
    }
    fvm_path_destroy(subset);
}


/* Narrow same-paint changes to changed closed/open contours. Winding cannot
 * couple disjoint glyph bounds outside these dirty regions; affected tiles
 * always replay the complete original path, preserving holes and antialiasing. */
static int damage_path_change(DVM_EditorFrame *frame, const FramePath *a, const FramePath *b)
{
    size_t ac;
    size_t bc;
    size_t first = 0;
    size_t suffix = 0;
    if (a->group || b->group || a->width != b->width || a->even_odd != b->even_odd ||
        memcmp(&a->color, &b->color, sizeof(a->color)) || a->left != b->left || a->top != b->top ||
        a->right != b->right || a->bottom != b->bottom)
    {
        return 0;
    }
    ac = fvm_path_count(a->path);
    bc = fvm_path_count(b->path);
    while (first < ac && first < bc && same_verb(a->path, first, b->path, first))
    {
        ++first;
    }
    while (suffix < ac - first && suffix < bc - first && same_verb(a->path, ac - suffix - 1, b->path, bc - suffix - 1))
    {
        ++suffix;
    }
    damage_contours(frame, a, first, ac - suffix);
    damage_contours(frame, b, first, bc - suffix);
    return 1;
}


/* Pack actual dirty work rather than partitioning mostly clean canvas indices. */
static void pack_damage(DVM_EditorFrame *frame)
{
    unsigned int index;
    frame->work_count = 0;
    for (index = 0; index < frame->columns * frame->rows; ++index)
    {
        if (frame->dirty[index])
        {
            frame->work[frame->work_count++] = index;
        }
    }
}


/* Establish a self-contained baseline from the last actually displayed frame.
 * Intermediate queued frames never become a baseline, so supersession cannot lose damage. */
int dvm_editor_frame_incremental(DVM_EditorFrame *frame, const DVM_EditorFrame *prior)
{
    const FramePath *a;
    const FramePath *b;
    if (!frame || frame == prior || frame->background)
    {
        return 0;
    }
    frame->incremental = 1;
    frame->columns = (frame->surface.width + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE;
    frame->rows = (frame->surface.height + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE;
    memset(frame->dirty, 0, sizeof(frame->dirty));
    if (!prior || !prior->incremental || frame->surface.width != prior->surface.width ||
        frame->surface.height != prior->surface.height)
    {
        memset(frame->dirty, 1, frame->columns * frame->rows);
        pack_damage(frame);
        return 1;
    }
    memcpy(frame->surface.pixels, prior->surface.pixels, frame->surface.stride * frame->surface.height);
    a = frame->first;
    b = prior->first;
    while (a || b)
    {
        if ((!a || !b || !same_command(a, b)) && (!a || !b || !damage_path_change(frame, a, b)))
        {
            if (a)
            {
                damage_command(frame, a);
            }
            if (b)
            {
                damage_command(frame, b);
            }
        }
        a = a ? a->next : NULL;
        b = b ? b->next : NULL;
    }
    pack_damage(frame);
    return 1;
}


/* Expose a bounded dirty tile for native texture upload without any VM access. */
int dvm_editor_frame_damage(const DVM_EditorFrame *frame,
                            unsigned int index,
                            unsigned int *x,
                            unsigned int *y,
                            unsigned int *width,
                            unsigned int *height)
{
    if (!frame || !x || !y || !width || !height)
    {
        return 0;
    }
    if (!frame->incremental)
    {
        if (index)
        {
            return 0;
        }
        *x = *y = 0;
        *width = frame->surface.width;
        *height = frame->surface.height;
        return 1;
    }
    if (index >= frame->columns * frame->rows || !frame->dirty[index])
    {
        return 0;
    }
    *x = index % frame->columns * DVM_EDITOR_TILE_SIZE;
    *y = index / frame->columns * DVM_EDITOR_TILE_SIZE;
    *width = frame->surface.width - *x;
    *height = frame->surface.height - *y;
    if (*width > DVM_EDITOR_TILE_SIZE)
    {
        *width = DVM_EDITOR_TILE_SIZE;
    }
    if (*height > DVM_EDITOR_TILE_SIZE)
    {
        *height = DVM_EDITOR_TILE_SIZE;
    }
    return 1;
}


/* Rasterize one dirty tile in painter order on an independent native worker.
 * Immutable contours are only read; scratch paths/pixels and status belong to
 * this tile. Replay every overlapping layer, including unchanged foregrounds. */
static void render_tile(void *value, unsigned int work_index)
{
    DVM_EditorFrame *frame = value;
    unsigned int index = frame->work[work_index];
    const FramePath *entry;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned int row;
    unsigned char pixels[DVM_EDITOR_TILE_SIZE * DVM_EDITOR_TILE_SIZE * 4];
    unsigned char behind[DVM_EDITOR_TILE_SIZE * DVM_EDITOR_TILE_SIZE * 4];
    int group_active = 0; /* One desktop window group at a time, never nested. */
    if (!dvm_editor_frame_damage(frame, index, &x, &y, &width, &height))
    {
        return;
    }
    memset(pixels, 255, sizeof(pixels));
    for (entry = frame->first; entry; entry = entry->next)
    {
        FVM_Path *path = NULL;
        FVM_Affine shift;
        FVM_RasterSurface tile;
        int left = entry->left > (int)x ? entry->left : (int)x;
        int top = entry->top > (int)y ? entry->top : (int)y;
        int right = entry->right < (int)(x + width) ? entry->right : (int)(x + width);
        int bottom = entry->bottom < (int)(y + height) ? entry->bottom : (int)(y + height);
        FVM_Result result;
        if (right <= left || bottom <= top || entry->bounds.x_max < left || entry->bounds.x_min > right ||
            entry->bounds.y_max < top || entry->bounds.y_min > bottom)
        {
            continue;
        }
        if (entry->group)
        {
            if (entry->group == 1)
            {
                if (group_active)
                {
                    frame->errors[index] = FVM_INVALID_ARGUMENT;
                    return;
                }
                memcpy(behind, pixels, sizeof(pixels));
                group_active = 1;
            }
            else
            {
                unsigned int px;
                unsigned int py;
                if (!group_active)
                {
                    frame->errors[index] = FVM_INVALID_ARGUMENT;
                    return;
                }
                for (py = 0; py < height; ++py)
                {
                    for (px = 0; px < width; ++px)
                    {
                        unsigned int channel;
                        size_t byte = ((size_t)py * DVM_EDITOR_TILE_SIZE + px) * 4;
                        if (x + px < entry->bounds.x_min || x + px >= entry->bounds.x_max ||
                            y + py < entry->bounds.y_min || y + py >= entry->bounds.y_max)
                        {
                            continue;
                        }
                        for (channel = 0; channel < 3; ++channel)
                        {
                            pixels[byte + channel] =
                                (unsigned char)((pixels[byte + channel] * 9 + behind[byte + channel] + 5) / 10);
                        }
                    }
                }
                group_active = 0;
            }
            continue;
        }
        memset(&shift, 0, sizeof(shift));
        shift.xx = shift.yy = 1;
        shift.x0 = -left;
        shift.y0 = -top;
        tile.pixels = pixels + ((unsigned int)(top - (int)y) * DVM_EDITOR_TILE_SIZE + left - (int)x) * 4;
        tile.width = (unsigned int)(right - left);
        tile.height = (unsigned int)(bottom - top);
        tile.stride = DVM_EDITOR_TILE_SIZE * 4;
        result = fvm_path_create(&path);
        if (result == FVM_OK)
        {
            result = fvm_path_append(path, entry->path, &shift);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_draw(path, &tile, entry->color, entry->width, entry->even_odd);
        }
        fvm_path_destroy(path);
        if (result != FVM_OK)
        {
            frame->errors[index] = result;
            return;
        }
    }
    for (row = 0; row < height; ++row)
    {
        memcpy(frame->surface.pixels + (y + row) * frame->surface.stride + x * 4,
               pixels + row * DVM_EDITOR_TILE_SIZE * 4,
               width * 4);
    }
}


/* Paint in order on a worker; every rasterizer and destination belongs to this frame. */
FVM_Result dvm_editor_frame_render(void *value)
{
    DVM_EditorFrame *frame = value;
    FramePath *entry;
    if (!frame)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (frame->incremental)
    {
        unsigned int index;
        memset(frame->errors, 0, sizeof(frame->errors));
        if (fvm_render_jobs_run(frame->work_count, DVM_EDITOR_RASTER_WORKERS, render_tile, frame) != 0)
        {
            return FVM_ERROR;
        }
        for (index = 0; index < frame->columns * frame->rows; ++index)
        {
            if (frame->errors[index] != FVM_OK)
            {
                return frame->errors[index];
            }
        }
        return FVM_OK;
    }
    if (!frame->background)
    {
        memset(frame->surface.pixels, 255, frame->surface.stride * frame->surface.height);
    }
    for (entry = frame->first; entry; entry = entry->next)
    {
        FVM_Result result = fvm_path_draw(entry->path, &frame->surface, entry->color, 0, 0);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    return FVM_OK;
}


/* Reclaim only exclusively owned native resources; safe before or after rasterization. */
void dvm_editor_frame_destroy(void *value)
{
    DVM_EditorFrame *frame = value;
    if (frame)
    {
        FramePath *entry = frame->first;
        while (entry)
        {
            FramePath *next = entry->next;
            fvm_path_destroy(entry->path);
            free(entry);
            entry = next;
        }
        free(frame->surface.pixels);
        free(frame);
    }
}
