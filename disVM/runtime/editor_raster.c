/* Rasterize detached native editor geometry without accessing either interpreter. */
#include "editor_raster.h"
#include "flashvm/render_jobs.h"
#include <math.h>
#include <pthread.h>
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

/* Refcount metadata alone is shared across owner/queue-release threads. Pixels
 * and local commands are immutable once the rendering worker publishes a frame. */
static pthread_mutex_t image_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned int live_images; /* Protected physical image count, independent of frame references. */

/* One independently owned straight-alpha local object bitmap. */
typedef struct SceneImage
{
    unsigned int references;   /* Protected by image_mutex, never a pixel payload lock. */
    FVM_RasterSurface surface; /* Transparent local image including a two-pixel border. */
    FramePath *first;          /* Owned immutable local commands, retained with their associated pixels. */
    FramePath *last;           /* Tail of that private command chain. */
    size_t paths;              /* Copied command count for snapshot admission. */
    size_t verbs;              /* Copied contour verb count for snapshot admission. */
} SceneImage;

/* Retained object metadata; its own ink is cached independently of descendants. */
typedef struct SceneNode
{
    unsigned int id;      /* Stable copied identity, zero means unused. */
    unsigned int parent;  /* Stable ancestor identity, root0 owns canvas composition. */
    unsigned int flags;   /* Content, transform and descendant-composition dirty bits. */
    unsigned int width;   /* Logical bitmap width excluding border. */
    unsigned int height;  /* Logical bitmap height excluding border. */
    unsigned int opacity; /*0..1000 applied at composition, never baked into cached ink. */
    FVM_Affine world;     /* Actual composed native object-to-canvas placement. */
    FramePath *first;     /* Borrowed local commands held by this frame or its associated image. */
    FramePath *last;      /* Borrowed last command, limiting this object's range. */
    SceneImage *base;     /* Immutable old image used only for partial local repaint. */
    double damage[4];     /* Local left/top/right/bottom ink damage, including antialias border. */
    size_t raster_pixels; /* Actual local pixels cleared/repainted on this build. */
    SceneImage *image;    /* Shared immutable completed image or exclusively built candidate. */
    unsigned char *key;   /* Optional owner-supplied copied content identity, never a pointer wire. */
    size_t key_size;      /* Bounded initialized key bytes. */
    int local;            /* Replayed commands are already canonical node-local geometry. */
} SceneNode;

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
    SceneNode nodes[DVM_EDITOR_SCENE_NODES];         /* Stable retained layers in this complete snapshot. */
    unsigned int order[DVM_EDITOR_SCENE_NODES];      /* Actual painter order, independent of parent lookup. */
    int scene_budget;              /* Associated bitmap budget exceeded; caller may retry vector capture. */
    unsigned int node_count;       /* Number of admitted object layers. */
    unsigned int recording_node;   /* Owner-only active command range. */
    FramePath *recording_previous; /* Tail before the active object's first command. */
    unsigned int images_built;     /* Successful content rasterizations in this worker job. */
    unsigned int images_reused;    /* Retained images shared from the displayed baseline. */
    unsigned int nodes_dirty;      /* Admitted changed nodes before successful composition. */
    FVM_Result errors[DVM_EDITOR_FRAME_MAX_TILES]; /* One worker-owned status slot per tile. */
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

/* Retain/release immutable native image ownership across owner and queue threads.
 * Null is accepted; no pixel work is performed while the metadata lock is held. */
static void image_retain(SceneImage *image)
{
    if (image)
    {
        pthread_mutex_lock(&image_mutex);
        ++image->references;
        pthread_mutex_unlock(&image_mutex);
    }
}

/* Release one image reference; the last releaser owns and frees its pixels. */
static void image_release(SceneImage *image)
{
    unsigned int remaining;
    if (!image)
    {
        return;
    }
    pthread_mutex_lock(&image_mutex);
    remaining = --image->references;
    if (!remaining)
    {
        --live_images;
    }
    pthread_mutex_unlock(&image_mutex);
    if (!remaining)
    {
        FramePath *entry = image->first;
        while (entry)
        {
            FramePath *next = entry->next;
            fvm_path_destroy(entry->path);
            free(entry);
            entry = next;
        }
        free(image->surface.pixels);
        free(image);
    }
}

/* Start a bounded, nonnested owner-recorded command range; logical refusal is atomic. */
int dvm_editor_frame_node_begin(DVM_EditorFrame *frame, unsigned int id, unsigned int parent)
{
    if (!frame || !id || id >= DVM_EDITOR_SCENE_NODES || parent >= DVM_EDITOR_SCENE_NODES || parent == id ||
        frame->recording_node || frame->nodes[id].id || frame->node_count == DVM_EDITOR_SCENE_NODES - 1)
    {
        return 0;
    }
    frame->nodes[id].id = id;
    frame->nodes[id].parent = parent;
    frame->recording_node = id;
    frame->recording_previous = frame->last;
    frame->order[frame->node_count++] = id;
    return 1;
}

/* Finish copied placement/extent. Reject nonfinite/singular or excessive transforms
 * before later integer bounds conversion. Caller discards an incomplete frame. */
int dvm_editor_frame_node_end(
    DVM_EditorFrame *frame, const FVM_Affine *world, unsigned int width, unsigned int height, unsigned int opacity)
{
    SceneNode *node;
    double determinant;
    if (!frame || !frame->recording_node || !world || !width || !height || width > DVM_EDITOR_FRAME_EXTENT ||
        height > DVM_EDITOR_FRAME_EXTENT || opacity > DVM_EDITOR_NODE_OPACITY || !isfinite(world->xx) ||
        !isfinite(world->xy) || !isfinite(world->yx) || !isfinite(world->yy) || !isfinite(world->x0) ||
        !isfinite(world->y0) || fabs(world->x0) > 1048576 || fabs(world->y0) > 1048576 || fabs(world->xx) > 32 ||
        fabs(world->xy) > 32 || fabs(world->yx) > 32 || fabs(world->yy) > 32)
    {
        return 0;
    }
    determinant = world->xx * world->yy - world->xy * world->yx;
    if (!isfinite(determinant) || fabs(determinant) < 0.000001)
    {
        return 0;
    }
    node = &frame->nodes[frame->recording_node];
    if (node->image && (node->image->surface.width != width + 4 || node->image->surface.height != height + 4))
    {
        return 0;
    }
    node->world = *world;
    node->width = width;
    node->height = height;
    node->opacity = opacity;
    if (frame->last != frame->recording_previous)
    {
        node->first = frame->recording_previous ? frame->recording_previous->next : frame->first;
        node->last = frame->last;
    }
    frame->recording_node = 0;
    frame->recording_previous = NULL;
    return 1;
}

/* Copy a bounded initialized content key into this owner-private active node.
 * Caller bytes are borrowed for this call only; failure preserves the old key. */
int dvm_editor_frame_node_key(DVM_EditorFrame *frame, const void *key, size_t size)
{
    SceneNode *node;
    unsigned char *copy;
    if (!frame || !frame->recording_node || !key || !size || size > 4096)
    {
        return 0;
    }
    copy = malloc(size);
    if (!copy)
    {
        return 0;
    }
    memcpy(copy, key, size);
    node = &frame->nodes[frame->recording_node];
    free(node->key);
    node->key = copy;
    node->key_size = size;
    return 1;
}

/* Test an immutable displayed frame's owner-supplied key; no lossy hash/VM access.
 * Return zero for absent/unrendered identities or a different copied key. */
int dvm_editor_frame_node_matches(const DVM_EditorFrame *prior, unsigned int id, const void *key, size_t size)
{
    const SceneNode *node;
    if (!prior || !id || id >= DVM_EDITOR_SCENE_NODES || !key || !size)
    {
        return 0;
    }
    node = &prior->nodes[id];
    return node->id && node->image && node->key_size == size && !memcmp(node->key, key, size);
}

/* Replay an immutable image and its local commands into an empty active node.
 * No managed display object, outline clone or pixel payload copy is needed. */
int dvm_editor_frame_node_replay(DVM_EditorFrame *frame, const DVM_EditorFrame *prior, unsigned int id)
{
    const SceneNode *old;
    SceneNode *node;
    if (!frame || !prior || frame == prior || frame->recording_node != id || !id || id >= DVM_EDITOR_SCENE_NODES ||
        frame->last != frame->recording_previous)
    {
        return 0;
    }
    old = &prior->nodes[id];
    node = &frame->nodes[id];
    if (!old->id || !old->image || node->image || node->local ||
        old->image->paths > DVM_EDITOR_FRAME_MAX_PATHS - frame->paths ||
        old->image->verbs > DVM_EDITOR_FRAME_MAX_VERBS - frame->verbs)
    {
        return 0;
    }
    /* No outline cloning or comparison is needed after the exact owner content
     * key matched. Image ownership keeps both local commands and pixels alive. */
    node->image = old->image;
    image_retain(node->image);
    node->first = node->image->first;
    node->last = node->image->last;
    node->local = 1;
    frame->paths += node->image->paths;
    frame->verbs += node->image->verbs;
    return 1;
}

/* Invert a previously admitted copied affine placement; no allocation or VM access. */
static FVM_Affine scene_inverse(const FVM_Affine *world)
{
    double determinant = world->xx * world->yy - world->xy * world->yx;
    FVM_Affine inverse;
    inverse.xx = world->yy / determinant;
    inverse.xy = -world->xy / determinant;
    inverse.yx = -world->yx / determinant;
    inverse.yy = world->xx / determinant;
    inverse.x0 = -(inverse.xx * world->x0 + inverse.xy * world->y0);
    inverse.y0 = -(inverse.yx * world->x0 + inverse.yy * world->y0);
    return inverse;
}

/* Convert one native world command range into canonical local coordinates.
 * The1/4096-pixel grid removes floating cancellation noise during translation.
 * Clip bounds remain conservative; the root also clips composition to its canvas. */
static int scene_local(SceneNode *node)
{
    FVM_Affine inverse = scene_inverse(&node->world);
    FramePath *entry;
    double scale = sqrt(fabs(node->world.xx * node->world.yy - node->world.xy * node->world.yx));
    for (entry = node->first; entry; entry = entry == node->last ? NULL : entry->next)
    {
        FVM_Path *local = NULL;
        size_t index;
        double left = 1e30;
        double top = 1e30;
        double right = -1e30;
        double bottom = -1e30;
        unsigned int corner;
        if (entry->group || fvm_path_create(&local) != FVM_OK)
        {
            return 0;
        }
        for (index = 0; index < fvm_path_count(entry->path); ++index)
        {
            FVM_PathCommand command;
            unsigned int point;
            unsigned int count;
            if (fvm_path_get(entry->path, index, &command) != FVM_OK)
            {
                fvm_path_destroy(local);
                return 0;
            }
            count = command.verb == FVM_PATH_CUBIC       ? 3
                    : command.verb == FVM_PATH_QUADRATIC ? 2
                    : command.verb == FVM_PATH_CLOSE     ? 0
                                                         : 1;
            for (point = 0; point < count; ++point)
            {
                double x = command.x[point];
                double y = command.y[point];
                command.x[point] = floor((inverse.xx * x + inverse.xy * y + inverse.x0) * 4096 + 0.5) / 4096;
                command.y[point] = floor((inverse.yx * x + inverse.yy * y + inverse.y0) * 4096 + 0.5) / 4096;
            }
            if (fvm_path_add(local, &command) != FVM_OK)
            {
                fvm_path_destroy(local);
                return 0;
            }
        }
        for (corner = 0; corner < 4; ++corner)
        {
            double x = corner & 1 ? entry->right : entry->left;
            double y = corner & 2 ? entry->bottom : entry->top;
            double local_x = inverse.xx * x + inverse.xy * y + inverse.x0;
            double local_y = inverse.yx * x + inverse.yy * y + inverse.y0;
            if (local_x < left)
            {
                left = local_x;
            }
            if (local_x > right)
            {
                right = local_x;
            }
            if (local_y < top)
            {
                top = local_y;
            }
            if (local_y > bottom)
            {
                bottom = local_y;
            }
        }
        fvm_path_destroy(entry->path);
        entry->path = local;
        entry->width = floor(entry->width / scale * 4096 + 0.5) / 4096;
        if (left < -2)
        {
            left = -2;
        }
        if (top < -2)
        {
            top = -2;
        }
        if (right < -2)
        {
            right = -2;
        }
        if (bottom < -2)
        {
            bottom = -2;
        }
        if (left > node->width + 2.0)
        {
            left = node->width + 2.0;
        }
        if (top > node->height + 2.0)
        {
            top = node->height + 2.0;
        }
        if (right > node->width + 2.0)
        {
            right = node->width + 2.0;
        }
        if (bottom > node->height + 2.0)
        {
            bottom = node->height + 2.0;
        }
        entry->left = (int)floor(left + 0.0000001);
        entry->top = (int)floor(top + 0.0000001);
        entry->right = (int)ceil(right - 0.0000001);
        entry->bottom = (int)ceil(bottom - 0.0000001);
        if (entry->left < -2)
        {
            entry->left = -2;
        }
        if (entry->top < -2)
        {
            entry->top = -2;
        }
        if (entry->right > (int)node->width + 2)
        {
            entry->right = (int)node->width + 2;
        }
        if (entry->bottom > (int)node->height + 2)
        {
            entry->bottom = (int)node->height + 2;
        }
    }
    node->local = 1;
    return 1;
}

/* Exact comparison of quantized local content; placement and opacity are separate. */
static int scene_same(const SceneNode *a, const SceneNode *b)
{
    const FramePath *x = a->first;
    const FramePath *y = b->first;
    if (a->width != b->width || a->height != b->height)
    {
        return 0;
    }
    while (x && y)
    {
        if (!same_command(x, y))
        {
            return 0;
        }
        x = x == a->last ? NULL : x->next;
        y = y == b->last ? NULL : y->next;
    }
    return !x && !y;
}

/* Extend copied local damage by complete changed contours. Parameters are borrowed;
 * fallback to the whole path on allocation/read failure, preserving correctness. */
static void scene_range_damage(SceneNode *node, const FramePath *entry, size_t first, size_t after)
{
    FVM_Path *subset = NULL;
    FVM_PathCommand command;
    FVM_PathBounds bounds;
    size_t index;
    size_t count = fvm_path_count(entry->path);
    int ok = 1;
    if (first >= after || !count)
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
        ok = 0;
    }
    for (index = first; ok && index < after; ++index)
    {
        ok = fvm_path_get(entry->path, index, &command) == FVM_OK && fvm_path_add(subset, &command) == FVM_OK;
    }
    if (!ok || fvm_path_bounds(subset, &bounds) != FVM_OK)
    {
        ok = fvm_path_bounds(entry->path, &bounds) == FVM_OK;
    }
    fvm_path_destroy(subset);
    if (ok)
    {
        double margin = entry->width / 2 + 2;
        if (bounds.x_min - margin < node->damage[0])
        {
            node->damage[0] = bounds.x_min - margin;
        }
        if (bounds.y_min - margin < node->damage[1])
        {
            node->damage[1] = bounds.y_min - margin;
        }
        if (bounds.x_max + margin > node->damage[2])
        {
            node->damage[2] = bounds.x_max + margin;
        }
        if (bounds.y_max + margin > node->damage[3])
        {
            node->damage[3] = bounds.y_max + margin;
        }
    }
}

/* Compare old/new local paint ranges, narrowing glyph changes to complete contours.
 * Retain the old immutable image for worker-side patching; no old commands escape. */
static void scene_content_damage(SceneNode *node, const SceneNode *old)
{
    const FramePath *a = node->first;
    const FramePath *b = old->first;
    node->damage[0] = node->width + 2.0;
    node->damage[1] = node->height + 2.0;
    node->damage[2] = node->damage[3] = -2;
    while (a || b)
    {
        if (!(a && b && same_command(a, b)))
        {
            size_t ac = a ? fvm_path_count(a->path) : 0;
            size_t bc = b ? fvm_path_count(b->path) : 0;
            size_t first = 0, suffix = 0;
            int same_paint = a && b && a->width == b->width && a->even_odd == b->even_odd &&
                             !memcmp(&a->color, &b->color, sizeof(a->color)) && a->left == b->left &&
                             a->top == b->top && a->right == b->right && a->bottom == b->bottom;
            if (same_paint)
            {
                while (first < ac && first < bc && same_verb(a->path, first, b->path, first))
                {
                    first++;
                }
                while (suffix < ac - first && suffix < bc - first &&
                       same_verb(a->path, ac - suffix - 1, b->path, bc - suffix - 1))
                {
                    suffix++;
                }
            }
            if (a)
            {
                scene_range_damage(node, a, first, ac - suffix);
            }
            if (b)
            {
                scene_range_damage(node, b, first, bc - suffix);
            }
        }
        a = a && a != node->last ? a->next : NULL;
        b = b && b != old->last ? b->next : NULL;
    }
    if (node->damage[0] < -2)
    {
        node->damage[0] = -2;
    }
    if (node->damage[1] < -2)
    {
        node->damage[1] = -2;
    }
    if (node->damage[2] > node->width + 2.0)
    {
        node->damage[2] = node->width + 2.0;
    }
    if (node->damage[3] > node->height + 2.0)
    {
        node->damage[3] = node->height + 2.0;
    }
    node->base = old->image;
    image_retain(node->base);
}

/* Mark conservative transformed old/new object bounds in root tiles. */
static void scene_damage(DVM_EditorFrame *frame, const SceneNode *node)
{
    FramePath bounds;
    unsigned int corner;
    memset(&bounds, 0, sizeof(bounds));
    bounds.bounds.x_min = bounds.bounds.y_min = 1e30;
    bounds.bounds.x_max = bounds.bounds.y_max = -1e30;
    bounds.right = (int)frame->surface.width;
    bounds.bottom = (int)frame->surface.height;
    for (corner = 0; corner < 4; ++corner)
    {
        double x = corner & 1 ? node->width + 2.0 : -2.0;
        double y = corner & 2 ? node->height + 2.0 : -2.0;
        if (node->base && !(node->flags & DVM_EDITOR_NODE_TRANSFORM))
        {
            x = node->damage[corner & 1 ? 2 : 0];
            y = node->damage[corner & 2 ? 3 : 1];
        }
        double px = node->world.xx * x + node->world.xy * y + node->world.x0;
        double py = node->world.yx * x + node->world.yy * y + node->world.y0;
        if (px < bounds.bounds.x_min)
        {
            bounds.bounds.x_min = px;
        }
        if (px > bounds.bounds.x_max)
        {
            bounds.bounds.x_max = px;
        }
        if (py < bounds.bounds.y_min)
        {
            bounds.bounds.y_min = py;
        }
        if (py > bounds.bounds.y_max)
        {
            bounds.bounds.y_max = py;
        }
    }
    damage_command(frame, &bounds);
}

/* Resolve immutable displayed-node caches and propagate composition flags upward.
 * Invalid forests/recording ranges fail before worker publication. */
static int scene_prepare(DVM_EditorFrame *frame, const DVM_EditorFrame *prior)
{
    unsigned int index;
    size_t bytes = 0;
    int compatible = prior && prior->node_count && frame->surface.width == prior->surface.width &&
                     frame->surface.height == prior->surface.height;
    for (index = 0; index < frame->node_count; ++index)
    {
        SceneNode *node = &frame->nodes[frame->order[index]];
        const SceneNode *old = compatible ? &prior->nodes[node->id] : NULL;
        unsigned int parent = node->parent;
        unsigned int depth = 0;
        bytes += (size_t)(node->width + 4) * (node->height + 4) * 4;
        if (bytes > DVM_EDITOR_SCENE_BYTES)
        {
            frame->scene_budget = 1;
            return 0;
        }
        if (!node->local && !scene_local(node))
        {
            return 0;
        }
        while (parent)
        {
            if (!frame->nodes[parent].id || ++depth >= DVM_EDITOR_SCENE_NODES)
            {
                return 0;
            }
            parent = frame->nodes[parent].parent;
        }
        if (node->image)
        {
            frame->images_reused++;
        }
        else if (old && old->id && old->image && scene_same(node, old))
        {
            node->image = old->image;
            image_retain(node->image);
            frame->images_reused++;
        }
        else
        {
            node->flags |= DVM_EDITOR_NODE_CONTENT;
            if (old && old->image && node->width == old->width && node->height == old->height)
            {
                scene_content_damage(node, old);
            }
        }
        if (!old || !old->id || node->parent != old->parent || node->opacity != old->opacity ||
            memcmp(&node->world, &old->world, sizeof(node->world)) || index >= prior->node_count ||
            prior->order[index] != node->id)
        {
            node->flags |= DVM_EDITOR_NODE_TRANSFORM;
        }
        if (node->flags)
        {
            frame->nodes_dirty++;
            scene_damage(frame, node);
            if (old && old->id && (node->flags & DVM_EDITOR_NODE_TRANSFORM))
            {
                scene_damage(frame, old);
            }
        }
    }
    if (compatible)
    {
        for (index = 0; index < prior->node_count; ++index)
        {
            const SceneNode *old = &prior->nodes[prior->order[index]];
            if (!frame->nodes[old->id].id)
            {
                unsigned int parent = old->parent;
                scene_damage(frame, old);
                while (parent)
                {
                    if (frame->nodes[parent].id)
                    {
                        frame->nodes[parent].flags |= DVM_EDITOR_NODE_COMPOSITE;
                    }
                    parent = prior->nodes[parent].parent;
                }
            }
        }
    }
    for (index = 0; index < frame->node_count; ++index)
    {
        SceneNode *node = &frame->nodes[frame->order[index]];
        unsigned int parent = node->parent;
        if (node->flags)
        {
            while (parent)
            {
                frame->nodes[parent].flags |= DVM_EDITOR_NODE_COMPOSITE;
                parent = frame->nodes[parent].parent;
            }
        }
    }
    if (!compatible)
    {
        memset(frame->dirty, 1, frame->columns * frame->rows);
    }
    pack_damage(frame);
    return 1;
}

/* Copy native local geometry once when constructing a new associated image.
 * The image owns this chain independently of frame ranges and future queues. */
static int scene_image_commands(SceneImage *image, const SceneNode *node)
{
    const FramePath *entry;
    for (entry = node->first; entry; entry = entry == node->last ? NULL : entry->next)
    {
        FramePath *copy = malloc(sizeof(*copy));
        FVM_Path *path = NULL;
        if (!copy || fvm_path_create(&path) != FVM_OK || fvm_path_append(path, entry->path, NULL) != FVM_OK)
        {
            free(copy);
            fvm_path_destroy(path);
            return 0;
        }
        *copy = *entry;
        copy->path = path;
        copy->next = NULL;
        if (image->last)
        {
            image->last->next = copy;
        }
        else
        {
            image->first = copy;
        }
        image->last = copy;
        image->paths++;
        image->verbs += fvm_path_count(path);
    }
    return 1;
}

/* Build one associated transparent local image, with clips and no VM/font handles.
 * Candidate image is attached only after every command succeeds. */
static FVM_Result scene_build(SceneNode *node)
{
    SceneImage *image = calloc(1, sizeof(*image));
    FramePath *entry;
    if (!image)
    {
        return FVM_OUT_OF_MEMORY;
    }
    image->references = 1;
    pthread_mutex_lock(&image_mutex);
    ++live_images;
    pthread_mutex_unlock(&image_mutex);
    image->surface.width = node->width + 4;
    image->surface.height = node->height + 4;
    image->surface.stride = (size_t)image->surface.width * 4;
    image->surface.pixels = calloc(image->surface.height, image->surface.stride);
    if (!image->surface.pixels)
    {
        image_release(image);
        return FVM_OUT_OF_MEMORY;
    }
    if (node->base)
    {
        unsigned int row;
        int left = (int)floor(node->damage[0]) + 2;
        int top = (int)floor(node->damage[1]) + 2;
        int right = (int)ceil(node->damage[2]) + 2;
        int bottom = (int)ceil(node->damage[3]) + 2;
        memcpy(image->surface.pixels, node->base->surface.pixels, image->surface.stride * image->surface.height);
        if (left < right && top < bottom)
        {
            for (row = (unsigned int)top; row < (unsigned int)bottom; ++row)
            {
                memset(image->surface.pixels + row * image->surface.stride + left * 4, 0, (size_t)(right - left) * 4);
            }
            node->raster_pixels = (size_t)(right - left) * (bottom - top);
        }
    }
    else
    {
        node->raster_pixels = (size_t)image->surface.width * image->surface.height;
    }
    for (entry = node->first; entry; entry = entry == node->last ? NULL : entry->next)
    {
        FVM_Path *path = NULL;
        FVM_RasterSurface clip;
        FVM_Affine shift = {1, 0, 0, 1, 0, 0};
        FVM_Result result;
        int left = entry->left + 2;
        int top = entry->top + 2;
        int right = entry->right + 2;
        int bottom = entry->bottom + 2;
        if (node->base)
        {
            int damage_left = (int)floor(node->damage[0]) + 2;
            int damage_top = (int)floor(node->damage[1]) + 2;
            int damage_right = (int)ceil(node->damage[2]) + 2;
            int damage_bottom = (int)ceil(node->damage[3]) + 2;
            if (left < damage_left)
            {
                left = damage_left;
            }
            if (top < damage_top)
            {
                top = damage_top;
            }
            if (right > damage_right)
            {
                right = damage_right;
            }
            if (bottom > damage_bottom)
            {
                bottom = damage_bottom;
            }
        }
        if (left >= right || top >= bottom)
        {
            continue;
        }
        clip = image->surface;
        clip.pixels += (size_t)top * clip.stride + (size_t)left * 4;
        clip.width = (unsigned int)(right - left);
        clip.height = (unsigned int)(bottom - top);
        shift.x0 = 2 - left;
        shift.y0 = 2 - top;
        result = fvm_path_create(&path);
        if (result == FVM_OK)
        {
            result = fvm_path_append(path, entry->path, &shift);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_draw(path, &clip, entry->color, entry->width, entry->even_odd);
        }
        fvm_path_destroy(path);
        if (result != FVM_OK)
        {
            image_release(image);
            return result;
        }
    }
    if (!scene_image_commands(image, node))
    {
        image_release(image);
        return FVM_OUT_OF_MEMORY;
    }
    node->image = image;
    image_release(node->base);
    node->base = NULL;
    return FVM_OK;
}

/* Sample an immutable image at pixel-center coordinates with premultiplied
 * bilinear weights. Output is caller-owned straight RGBA; transparent edge taps
 * prevent color fringes. Integer translation uses the direct-copy path instead. */
static void scene_sample(const FVM_RasterSurface *image, double sx, double sy, unsigned char *rgba)
{
    double x = sx - 0.5;
    double y = sy - 0.5;
    int ix = (int)floor(x);
    int iy = (int)floor(y);
    double fx = x - ix;
    double fy = y - iy;
    double alpha = 0;
    double colors[3] = {0, 0, 0};
    unsigned int tap;
    unsigned int channel;
    for (tap = 0; tap < 4; ++tap)
    {
        int px = ix + (int)(tap & 1);
        int py = iy + (int)((tap >> 1) & 1);
        double weight = (tap & 1 ? fx : 1 - fx) * (tap & 2 ? fy : 1 - fy);
        if (px >= 0 && py >= 0 && px < (int)image->width && py < (int)image->height)
        {
            const unsigned char *pixel = image->pixels + (size_t)py * image->stride + (size_t)px * 4;
            double contribution = pixel[3] * weight;
            alpha += contribution;
            for (channel = 0; channel < 3; ++channel)
            {
                colors[channel] += pixel[channel] * contribution;
            }
        }
    }
    rgba[3] = (unsigned char)floor(alpha + 0.5);
    for (channel = 0; channel < 3; ++channel)
    {
        double value = alpha > 0 ? floor(colors[channel] / alpha + 0.5) : 0;
        rgba[channel] = (unsigned char)(value > 255 ? 255 : value);
    }
}

/* Compose cached layers into one dirty root tile. Transform/opacity only affect
 * sampling/blitting, never their associated local bitmap. Painter order remains
 * explicit, so parent bodies cannot accidentally capture their children twice. */
static void scene_tile(void *value, unsigned int work_index)
{
    DVM_EditorFrame *frame = value;
    unsigned int index = frame->work[work_index];
    unsigned int x, y, width, height, px, py, ordinal;
    if (!dvm_editor_frame_damage(frame, index, &x, &y, &width, &height))
    {
        return;
    }
    for (py = y; py < y + height; ++py)
    {
        memset(frame->surface.pixels + (size_t)py * frame->surface.stride + x * 4, 255, width * 4);
    }
    for (ordinal = 0; ordinal < frame->node_count; ++ordinal)
    {
        const SceneNode *node = &frame->nodes[frame->order[ordinal]];
        const FVM_RasterSurface *image = &node->image->surface;
        FVM_Affine inverse = scene_inverse(&node->world);
        unsigned int corner;
        double min_x = 1e30, min_y = 1e30, max_x = -1e30, max_y = -1e30;
        unsigned int first_x, first_y, after_x, after_y;
        int translated = node->world.xx == 1 && node->world.yy == 1 && node->world.xy == 0 && node->world.yx == 0 &&
                         floor(node->world.x0) == node->world.x0 && floor(node->world.y0) == node->world.y0;
        if (!node->opacity)
        {
            continue;
        }
        for (corner = 0; corner < 4; ++corner)
        {
            double lx = corner & 1 ? node->width + 2.0 : -2.0;
            double ly = corner & 2 ? node->height + 2.0 : -2.0;
            double wx = node->world.xx * lx + node->world.xy * ly + node->world.x0;
            double wy = node->world.yx * lx + node->world.yy * ly + node->world.y0;
            if (wx < min_x)
            {
                min_x = wx;
            }
            if (wy < min_y)
            {
                min_y = wy;
            }
            if (wx > max_x)
            {
                max_x = wx;
            }
            if (wy > max_y)
            {
                max_y = wy;
            }
        }
        if (max_x <= x || max_y <= y || min_x >= x + width || min_y >= y + height)
        {
            continue;
        }
        first_x = min_x > x ? (unsigned int)floor(min_x) : x;
        first_y = min_y > y ? (unsigned int)floor(min_y) : y;
        after_x = max_x < x + width ? (unsigned int)ceil(max_x) : x + width;
        after_y = max_y < y + height ? (unsigned int)ceil(max_y) : y + height;
        for (py = first_y; py < after_y; ++py)
        {
            for (px = first_x; px < after_x; ++px)
            {
                double sx = translated ? (double)px - node->world.x0 + 2.5
                                       : inverse.xx * (px + 0.5) + inverse.xy * (py + 0.5) + inverse.x0 + 2;
                double sy = translated ? (double)py - node->world.y0 + 2.5
                                       : inverse.yx * (px + 0.5) + inverse.yy * (py + 0.5) + inverse.y0 + 2;
                const unsigned char *source;
                unsigned char filtered[4];
                unsigned char *target;
                unsigned int alpha, channel;
                if (sx < 0 || sy < 0 || sx >= image->width || sy >= image->height)
                {
                    continue;
                }
                source = image->pixels + (size_t)floor(sy) * image->stride + (size_t)floor(sx) * 4;
                if (!translated)
                {
                    scene_sample(image, sx, sy, filtered);
                    source = filtered;
                }
                alpha = source[3] * node->opacity;
                if (!alpha)
                {
                    continue;
                }
                target = frame->surface.pixels + (size_t)py * frame->surface.stride + px * 4;
                if (alpha == 255U * DVM_EDITOR_NODE_OPACITY)
                {
                    memcpy(target, source, 4);
                    continue;
                }
                for (channel = 0; channel < 3; ++channel)
                {
                    target[channel] = (unsigned char)((source[channel] * alpha +
                                                       target[channel] * (255U * DVM_EDITOR_NODE_OPACITY - alpha) +
                                                       (255U * DVM_EDITOR_NODE_OPACITY) / 2) /
                                                      (255U * DVM_EDITOR_NODE_OPACITY));
                }
                target[3] = 255;
            }
        }
    }
}

/* Build only an uncached local object on a native worker; the copied ordinal
 * selects a disjoint node/status slot. No shared image payload is mutated. */
static void scene_build_job(void *value, unsigned int ordinal)
{
    DVM_EditorFrame *frame = value;
    SceneNode *node = &frame->nodes[frame->order[ordinal]];
    frame->errors[ordinal] = node->image ? FVM_OK : scene_build(node);
}

/* Render dirty local content then compose root damage in parallel. Only successful
 * full composition clears this snapshot's propagated flags; prior remains immutable. */
static FVM_Result scene_render(DVM_EditorFrame *frame)
{
    unsigned int index;
    unsigned int missing = 0;
    for (index = 0; index < frame->node_count; ++index)
    {
        missing += frame->nodes[frame->order[index]].image == NULL;
    }
    if (missing)
    {
        if (fvm_render_jobs_run(frame->node_count, DVM_EDITOR_RASTER_WORKERS, scene_build_job, frame) != 0)
        {
            return FVM_ERROR;
        }
        for (index = 0; index < frame->node_count; ++index)
        {
            if (frame->errors[index] != FVM_OK)
            {
                return frame->errors[index];
            }
        }
        frame->images_built = missing;
    }
    if (fvm_render_jobs_run(frame->work_count, DVM_EDITOR_RASTER_WORKERS, scene_tile, frame) != 0)
    {
        return FVM_ERROR;
    }
    for (index = 0; index < frame->node_count; ++index)
    {
        frame->nodes[frame->order[index]].flags = 0;
    }
    return FVM_OK;
}

/* Return completed-frame diagnostics without touching either VM or a running worker. */
void dvm_editor_frame_scene_stats(const DVM_EditorFrame *frame,
                                  unsigned int *built,
                                  unsigned int *reused,
                                  unsigned int *dirty)
{
    if (built)
    {
        *built = frame ? frame->images_built : 0;
    }
    if (reused)
    {
        *reused = frame ? frame->images_reused : 0;
    }
    if (dirty)
    {
        *dirty = frame ? frame->nodes_dirty : 0;
    }
}

/* Sum actual cleared local bitmap pixels after rendering. Shared unchanged images
 * report zero; this counter does not count root composition or frame copies. */
size_t dvm_editor_frame_scene_pixels(const DVM_EditorFrame *frame)
{
    unsigned int index;
    size_t pixels = 0;
    if (frame)
    {
        for (index = 0; index < frame->node_count; ++index)
        {
            pixels += frame->nodes[frame->order[index]].raster_pixels;
        }
    }
    return pixels;
}

/* Return the process-wide count of physical associated images under the short
 * metadata lock. Intended for diagnostics; it transfers no resource ownership. */
unsigned int dvm_editor_scene_live_images(void)
{
    unsigned int count;
    pthread_mutex_lock(&image_mutex);
    count = live_images;
    pthread_mutex_unlock(&image_mutex);
    return count;
}

/* Return1 only for associated-image budget exhaustion, not malformed scene data. */
int dvm_editor_frame_scene_budget(const DVM_EditorFrame *frame)
{
    return frame && frame->scene_budget;
}

/* Borrow one owner-held frame and return this node's pending dirty bits; zero
 * means absent/clean. Never inspect a frame while a raster worker owns it. */
unsigned int dvm_editor_frame_node_flags(const DVM_EditorFrame *frame, unsigned int id)
{
    return frame && id < DVM_EDITOR_SCENE_NODES ? frame->nodes[id].flags : 0;
}

/* Establish a self-contained baseline from the last actually displayed frame.
 * Intermediate queued frames never become a baseline, so supersession cannot lose damage. */
int dvm_editor_frame_incremental(DVM_EditorFrame *frame, const DVM_EditorFrame *prior)
{
    const FramePath *a;
    const FramePath *b;
    if (!frame || frame == prior || frame->background || frame->incremental)
    {
        return 0;
    }
    if (frame->recording_node)
    {
        return 0;
    }
    frame->incremental = 1;
    frame->columns = (frame->surface.width + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE;
    frame->rows = (frame->surface.height + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE;
    memset(frame->dirty, 0, sizeof(frame->dirty));
    if (frame->node_count)
    {
        if (prior && prior->incremental && frame->surface.width == prior->surface.width &&
            frame->surface.height == prior->surface.height)
        {
            memcpy(frame->surface.pixels, prior->surface.pixels, frame->surface.stride * frame->surface.height);
        }
        return scene_prepare(frame, prior);
    }
    if (prior && prior->node_count)
    {
        prior = NULL; /* Retained local commands cannot serve as a world-vector diff baseline. */
    }
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
    if (frame->node_count)
    {
        return frame->incremental ? scene_render(frame) : FVM_INVALID_ARGUMENT;
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
        {
            unsigned int id;
            for (id = 1; id < DVM_EDITOR_SCENE_NODES; ++id)
            {
                image_release(frame->nodes[id].image);
                image_release(frame->nodes[id].base);
                free(frame->nodes[id].key);
            }
        }
        free(frame->surface.pixels);
        free(frame);
    }
}
