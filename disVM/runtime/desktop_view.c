/* Actual MovieClip/AS widget presentation for copied Limbo desktop scenes. */
#include "desktop_view.h"
#include "vector_local.h"
#include "desktop_chrome.h"
#include "editor_ink.h"
#include "editor_vector_layout.h"
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

#define DESK_SLOTS 96U  /* Stable bounded widget identities, independent of VM addresses. */
#define DESK_TEXT 1024U /* Maximum copied UTF-8 payload. */
#define DESK_META 72U   /* parent/x/y/width/height/style/caret/mirror/anchor words. */

/* One immutable copied scene item and its persistent owner-local capabilities. */
typedef struct DeskItem
{
    unsigned long id;                /* Stable controller slot. */
    unsigned long parent;            /* Layer group 0..4. */
    long x;                          /* Signed parent-local pixels; windows may pass under the left rail. */
    long y;                          /* Signed parent-local pixels; scrolled rail items may precede the viewport. */
    unsigned long width;             /* Bounded local extent. */
    unsigned long height;            /* Bounded local extent. */
    unsigned long mirror;            /* Optional source window snapshot in this scene. */
    unsigned long line;              /* One-based logical source line, zero for non-editor panes. */
    unsigned long style;             /* Palette 0..6; line origin is decoded separately. */
    long anchor;                     /* Clipped Unicode selection endpoint, -1 hides selection. */
    long caret;                      /* Unicode position, -1 disables the caret. */
    char text[DESK_TEXT];            /* Owned counted text, not evaluated. */
    size_t size;                     /* Valid bytes in text. */
    unsigned char colors[DESK_TEXT]; /* Numeric palette 0..5 per displayed Unicode scalar. */
} DeskItem;

static FVM_Context *owner;                /* Borrowed FlashVM context; never used on raster workers. */
static FVM_Object *root;                  /* [GC:] Pinned MovieClip root retaining all scene children. */
static FVM_Object *groups[13];            /* [GC:] Borrowed children of the retained root. */
static FVM_Object *clips[DESK_SLOTS];     /* [GC:] Borrowed stable MovieClip children. */
static FVM_Object *marks[DESK_SLOTS];     /* [GC:] Selection MovieClips retained beneath widgets. */
static FVM_Object *cursors[DESK_SLOTS];   /* [GC:] Caret MovieClips retained beneath each widget. */
static FVM_Object *labels[DESK_SLOTS];    /* [GC:] TextFields retained on their MovieClips. */
static unsigned char visible[DESK_SLOTS]; /* Slots admitted in the previous complete scene. */
static unsigned long parents[DESK_SLOTS]; /* Current validated presentation parent; document identity stays stable. */
static FVM_Font *face;                    /* Owner-only native face for caret geometry. */
static DeskItem items[DESK_SLOTS];        /* Copied staged descriptors. */
static unsigned int expected;             /* Exact declared item count. */
static unsigned int received;             /* Complete staged descriptors. */
static unsigned long ticket;              /* Committed view token. */
static long tablepan;                     /* Logical horizontal viewport translation in physical pixels. */
static unsigned long reveal_epoch;        /* Explicit Dis-owned reveal proposal identity. */
static unsigned long tablezoom = 100;     /* Uniform scale of the table MovieClip only. */
static int prepared_retained;             /* Owner-only layer split policy for the current detached frame. */
static const DVM_EditorFrame *prepared_prior; /* Borrowed displayed baseline during owner preparation. */
static int deferred_vector;                   /* Current pane's native ink is captured after its chrome node closes. */
static FVM_Affine deferred_world; /* Copied current source-pane transform, never worker-shared mutable state. */
static unsigned long offset;      /* Table scroll magnitude. */
static unsigned int width;        /* Committed canvas pixels. */
static unsigned int height;       /* Committed canvas pixels. */
static int pending;               /* A declared scene is being assembled. */
static int end_waiting;           /* End metadata validated; private geometry fragments still outstanding. */
static int trace;                 /* Explicit replay diagnostic mode; ordinary windows do not dump source text. */
static FVM_Path *text_paths[DESK_SLOTS];       /* Owner-local untransformed glyph outlines. */
static FVM_Path *syntax_paths[DESK_SLOTS][4];  /* Detached colour groups2..5, sharing whole-field shaping. */
static DeskItem text_keys[DESK_SLOTS];         /* Immutable layout keys; position is deliberately excluded. */
static unsigned char *drag_pixels[DESK_SLOTS]; /* Owned bounded complete window snapshots. */
static DeskItem drag_keys[DESK_SLOTS];         /* Content/geometry keys for copied drag pixels. */
static unsigned int glass_width[2];            /* Cached immutable rail contour widths. */
static unsigned int glass_height[2];           /* Cached immutable rail contour heights. */
static int active;                             /* A desktop scene has been admitted. */


#define VECTOR_BYTES 1230000U /* Complete bounded ASCII command stream, never SVG/script input. */
#define VECTOR_SHAPES 33U    /* 32 source paths plus one transient editing overlay. */
#define VECTOR_NODES 20480U  /* 16384 source commands plus 4096 presentation commands. */

/* Detached scalar command. Managed/native object addresses never enter the stream. */
typedef DVM_VectorCommand VectorNode; /* Copied native geometry, no managed references. */

/* Immutable path style and range into its pane-owned detached node array. */
typedef struct VectorShape
{
    unsigned long color; /* RGB24; common fill/stroke colour. */
    unsigned long width; /* Logical width0..32, zero disables stroke. */
    unsigned long fill;  /* Boolean fill admission. */
    unsigned int first;  /* First node in the detached array. */
    unsigned int count;  /* Path command count2..4096. */
} VectorShape;

/* One staged vector snapshot. Only the FlashVM owner reads/writes it; raster
 * workers receive copied contours through the existing immutable frame pipeline. */
typedef struct VectorScene
{
    char bytes[VECTOR_BYTES];          /* Owned bounded hex assembly buffer. */
    size_t expected;                   /* Exact declared stream bytes, zero for text panes. */
    size_t used;                       /* Contiguous bytes received. */
    unsigned long tool;                /* Copied authoring/editing tool1..7. */
    unsigned long zoom;                /* Copied canvas percent, 25..1600. */
    unsigned long center_x;            /* Copied logical viewport centre X. */
    unsigned long center_y;            /* Copied logical viewport centre Y. */
    unsigned long color;               /* Copied current RGB24. */
    unsigned long width;               /* Copied current logical width1..32. */
    unsigned long fill;                /* Copied fill toggle0/1. */
    unsigned long undo;                /* Copied Undo availability0..16. */
    unsigned long redo;                /* Copied Redo availability0..16. */
    unsigned int count;                /* Parsed complete path count. */
    unsigned int complete;             /* Only one allows painting/End commit. */
    VectorShape shapes[VECTOR_SHAPES]; /* Copied style/range records. */
    DVM_VectorLocalPlan local;         /* Optional owner-issued point modifier plan. */
    VectorNode nodes[VECTOR_NODES];    /* Copied scalar commands. */
} VectorScene;

static FVM_Object
    *vector_buttons[DESK_SLOTS][32]; /* [GC:] Borrowed child MovieClips retained by each pane/thumbnail. */
static VectorScene vectors[9];       /* Stable pane identities1..8, reset at scene Begin. */
static DVM_VectorLocal *local_edit; /* Exclusively owned immutable baseline/scratch. */
static unsigned long local_widget; /* Captured source pane identity, never a VM handle. */
static int local_released; /* Final proposal sent; retire on the next idle canonical plan. */
static unsigned long local_epochs[9]; /* Content-key generations for native source motion. */


/* Reapply native motion after a complete canonical owner snapshot arrives. */
static void vector_local_reconcile(unsigned long widget);


/* Owner-local visual interpolation; no document, VM pointer or worker handle. */
typedef struct DeskMotion
{
    double from_x;        /* Initial canvas origin in pixels. */
    double from_y;        /* Initial canvas origin in pixels. */
    double from_scale;    /* Initial uniform scale relative to normal geometry. */
    double to_x;          /* Final canvas origin in pixels. */
    double to_y;          /* Final canvas origin in pixels. */
    double to_scale;      /* Final uniform scale relative to normal geometry. */
    unsigned int elapsed; /* Bounded elapsed wall-clock milliseconds,0..240. */
    int active;           /* Presentation still needs clock-driven frames. */
} DeskMotion;

static DeskMotion camera_motion;          /* Value-only whole-table pan/scroll/zoom interpolation. */
static DeskMotion motions[9];             /* Value-only interpolation for eight window identities. */
static DeskItem previous_windows[9];      /* Last complete logical window descriptors. */
static unsigned char previous_present[9]; /* Previous scene identity membership. */

/* Evaluate smoothstep with zero endpoint velocity. Borrow motion and write
 * caller-owned table-local position/scale; no allocation or VM access occurs. */
static void motion_sample(const DeskMotion *motion, double *x, double *y, double *scale)
{
    double t = motion->elapsed / 240.0;
    double ease = t * t * (3.0 - 2.0 * t);
    *x = motion->from_x + (motion->to_x - motion->from_x) * ease;
    *y = motion->from_y + (motion->to_y - motion->from_y) * ease;
    *scale = motion->from_scale + (motion->to_scale - motion->from_scale) * ease;
}

/* Read the effective visual viewport; logical target stays Dis-owned. Outputs
 * are caller-owned scale/pan/scroll values. No allocation or VM state changes. */
static void camera_view(double *zoom, double *pan, double *scroll)
{
    *zoom = tablezoom / 100.0;
    *pan = tablepan;
    *scroll = offset;
    if (camera_motion.active)
    {
        motion_sample(&camera_motion, pan, scroll, zoom);
    }
}

/* Convert a copied window trajectory from canvas pixels into the current
 * table matrix. Borrow motion and write local x/y/scale; no VM handles escape. */
static void motion_local(const DeskMotion *motion, double *x, double *y, double *scale, unsigned long parent)
{
    double zoom;
    double pan;
    double scroll;
    motion_sample(motion, x, y, scale);
    if (parent == 4)
    {
        return;
    }
    camera_view(&zoom, &pan, &scroll);
    *x = 84 + (*x - pan - 84) / zoom;
    *y = 48 + (*y + scroll - 48) / zoom;
    *scale /= zoom;
}

/* Observe a borrowed owner-local MovieClip's composed native matrix. */
static int world_matrix(FVM_Object *clip, unsigned long parent, FVM_Affine *output);

/* Observe a complete typed scene and derive visual transitions only. Logical
 * visibility, focus, normal geometry and thumbnail ordering stay on Dis.
 * All descriptors are copied; missing/changed-size panes cancel stale motion. */
static void motion_commit(void)
{
    unsigned int id;
    for (id = 1; id <= 8; ++id)
    {
        const DeskItem *window = NULL;
        const DeskItem *thumbnail = NULL;
        unsigned int index;
        for (index = 0; index < received; ++index)
        {
            if (items[index].id == id)
            {
                window = &items[index];
            }
            if (items[index].mirror == id && items[index].parent == 2)
            {
                thumbnail = &items[index];
            }
        }
        if (!window)
        {
            motions[id].active = 0;
            previous_present[id] = 0;
            continue;
        }
        if (previous_present[id] && previous_windows[id].parent != window->parent)
        {
            previous_present[id] = 0;
            motions[id].active = 0;
        }
        if (previous_present[id] && thumbnail && (previous_windows[id].x == 65536) != (window->x == 65536))
        {
            DeskMotion *motion = &motions[id];
            double zoom = window->parent == 4 ? 1.0 : tablezoom / 100.0;
            double small =
                fmin(1, fmin((double)thumbnail->width / window->width, (double)thumbnail->height / window->height));
            double x = thumbnail->x;
            double y = thumbnail->y;
            int hiding = window->x == 65536;
            double from_x = x;
            double from_y = y;
            double from_scale = small;
            FVM_Affine world;
            if (hiding && clips[id] && world_matrix(clips[id], window->parent, &world))
            {
                from_x = world.x0;
                from_y = world.y0;
                from_scale = world.xx;
            }
            if (motion->active)
            {
                motion_sample(motion, &from_x, &from_y, &from_scale);
            }
            motion->from_x = from_x;
            motion->from_y = from_y;
            motion->from_scale = from_scale;
            motion->to_x = hiding ? x : window->parent == 4 ? window->x : 84 + (window->x - 84) * zoom + tablepan;
            motion->to_y = hiding ? y : window->parent == 4 ? window->y : 48 + (window->y - 48) * zoom - offset;
            motion->to_scale = hiding ? small : zoom;
            motion->elapsed = 0;
            motion->active = 1;
        }
        else if (previous_present[id] &&
                 (previous_windows[id].width != window->width || previous_windows[id].height != window->height))
        {
            motions[id].active = 0;
        }
        previous_windows[id] = *window;
        previous_present[id] = 1;
    }
}

/* Advance visual time independently of SWF frames. elapsed is a copied bounded
 * delta in milliseconds; invalid deltas change nothing. Return one if a new
 * immutable raster snapshot is needed. Owner only; no document or event waits. */
int dvm_desktop_advance(unsigned int elapsed)
{
    unsigned int id;
    int changed = 0;
    if (!active || pending || !elapsed || elapsed > 1000)
    {
        return 0;
    }
    if (camera_motion.active)
    {
        unsigned int remaining = 240 - camera_motion.elapsed;
        camera_motion.elapsed += elapsed < remaining ? elapsed : remaining;
        camera_motion.active = camera_motion.elapsed < 240;
        changed = 1;
        if (trace)
        {
            double pan;
            double scroll;
            double zoom;
            motion_sample(&camera_motion, &pan, &scroll, &zoom);
            printf("CAMERA %lu %u %.3f %.3f %.6f\n", ticket, camera_motion.elapsed, pan, scroll, zoom);
        }
    }
    for (id = 1; id <= 8; ++id)
    {
        DeskMotion *motion = &motions[id];
        if (motion->active)
        {
            unsigned int remaining = 240 - motion->elapsed;
            motion->elapsed += elapsed < remaining ? elapsed : remaining;
            motion->active = motion->elapsed < 240;
            changed = 1;
            if (trace)
            {
                double x;
                double y;
                double scale;
                motion_sample(motion, &x, &y, &scale);
                printf("TWEEN %lu %u %u %.3f %.3f %.6f\n", ticket, id, motion->elapsed, x, y, scale);
            }
        }
    }
    return changed;
}

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

/* Capture only a container's own body: its document children are independent
 * scene items and are captured later at their current matrices. item/surface
 * are borrowed on the owner. Saved visibility values are primitive copies.
 * Restore every mask on failure; no task polling or worker VM access occurs. */
static FVM_Result draw_widget_clip(const DeskItem *item, FVM_RasterSurface *surface)
{
    FVM_Value saved[8];
    FVM_Value hidden;
    unsigned char changed[8] = {0};
    unsigned int id;
    unsigned long parent;
    FVM_Result result = FVM_OK;
    if (!((item->id >= 13 && item->id <= 19) || item->id == 28))
    {
        return draw_clip(clips[item->id], surface, NULL);
    }
    parent = item->id == 28 ? 12 : item->id - 8;
    fvm_value_number(&hidden, 0);
    /* [GC:] Hidden display children remain rooted by the container. No managed
     * references enter saved; the owner restores flags before exposing the view. */
    for (id = 1; id <= 8; ++id)
    {
        if (clips[id] && parents[id] == parent)
        {
            result = fvm_object_get(owner, clips[id], "_visible", &saved[id - 1]);
            if (result != FVM_OK)
            {
                break;
            }
            if (saved[id - 1].type != FVM_VALUE_NUMBER && saved[id - 1].type != FVM_VALUE_BOOLEAN)
            {
                result = FVM_INVALID_ARGUMENT;
                break;
            }
            changed[id - 1] = 1;
            result = fvm_object_set(owner, clips[id], "_visible", &hidden);
            if (result != FVM_OK)
            {
                break;
            }
        }
    }
    if (result == FVM_OK)
    {
        result = draw_clip(clips[item->id], surface, NULL);
    }
    for (id = 1; id <= 8; ++id)
    {
        if (changed[id - 1])
        {
            FVM_Result restored = fvm_object_set(owner, clips[id], "_visible", &saved[id - 1]);
            if (result == FVM_OK)
            {
                result = restored;
            }
        }
    }
    return result;
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

/* Read admitted rail geometry from the complete copied scene. A scene without
 * a right rail owns the whole canvas. No theme constant or VM pointer escapes. */
static unsigned long right_extent(void)
{
    unsigned int index;
    for (index = 0; index < received; index++)
    {
        if (items[index].id == 12)
        {
            return items[index].width <= width ? items[index].width : width;
        }
    }
    return 0;
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

/* Parse a complete bounded private command stream. Every extent is checked
 * before dereference; no partial stream is paintable and no SVG/XML is parsed. */
static int vector_parse(VectorScene *scene)
{
    size_t at = 56;
    unsigned long shapes;
    size_t stride;
    int cubic;
    int local;
    unsigned int total = 0;
    unsigned int i;
    if (scene->used < 56 || !word(scene->bytes, &scene->tool) || !word(scene->bytes + 8, &scene->color) ||
        !word(scene->bytes + 16, &scene->width) || !word(scene->bytes + 24, &scene->fill) ||
        !word(scene->bytes + 32, &scene->undo) || !word(scene->bytes + 40, &scene->redo) ||
        scene->color > 0xffffffUL)
    {
        return 0;
    }
    int extended = (scene->tool & 0x200000UL) != 0;
    scene->tool &= ~0x200000UL;
    int compact = (scene->tool & 0x100000UL) != 0;
    scene->tool &= ~0x100000UL;
    int armed = (scene->tool & 0x80000UL) != 0;
    scene->tool &= ~0x80000UL;
    local = (scene->tool & 0x40000UL) != 0;
    if ((armed || compact || extended) && !local)
    {
        return 0;
    }
    scene->tool &= ~0x40000UL;
    memset(&scene->local,0,sizeof(scene->local));
    cubic = (scene->tool & 0x20000UL) != 0;
    scene->tool &= ~0x20000UL;
    stride = cubic ? 56 : 40;
    scene->zoom = 100;
    scene->center_x = 500;
    scene->center_y = 300;
    if (scene->tool & 0x10000UL)
    {
        scene->tool &= ~0x10000UL;
        if (scene->used < 80 || !word(scene->bytes + 48, &scene->zoom) ||
            !word(scene->bytes + 56, &scene->center_x) || !word(scene->bytes + 64, &scene->center_y) ||
            scene->zoom < 25 || scene->zoom > 1600 || scene->center_x > 1000 || scene->center_y > 600)
        {
            return 0;
        }
        at += 24;
    }
    if (local)
    {
        DVM_VectorLocalPlan *plan = &scene->local;
        unsigned long n;
        size_t start = at-8;
        if (scene->used-start < 56 || (scene->tool != 7 && scene->tool != 6) ||
            !word(scene->bytes+start,&plan->source) || !word(scene->bytes+start+8,&plan->revision) ||
            !word(scene->bytes+start+16,&plan->x) || !word(scene->bytes+start+24,&plan->y) ||
            !word(scene->bytes+start+32,&plan->count) || !word(scene->bytes+start+40,&plan->overlay) ||
            plan->source >= 32 || plan->revision > 0x7fffffffUL || plan->x > 1000 || plan->y > 600 ||
            plan->count < 2 || plan->count > DVM_VECTOR_LOCAL_NODES || plan->overlay < plan->count ||
            plan->overlay > DVM_VECTOR_LOCAL_NODES ||
            plan->count+plan->overlay > (scene->used-start-56)/8)
        {
            return 0;
        }
        start += 48;
        if (extended)
        {
            if (scene->used-start < 24 || !word(scene->bytes+start,&plan->mode) ||
                !word(scene->bytes+start+8,&plan->handle))
            {
                return 0;
            }
            start += 16;
        }
        if (scene->used-start < 8 || plan->count+plan->overlay > (scene->used-start-8)/8 ||
            (scene->tool == 6 && (plan->mode < 1 || plan->mode > 3)) ||
            (scene->tool == 7 && plan->mode != 0 && plan->mode != 5))
        {
            return 0;
        }
        for (n = 0; n < plan->count; ++n, start += 8)
        {
            if (!word(scene->bytes+start,&plan->masks[n]))
            {
                return 0;
            }
        }
        for (n = 0; n < plan->overlay; ++n, start += 8)
        {
            if (!word(scene->bytes+start,&plan->bindings[n]))
            {
                return 0;
            }
        }
        at = start+8;
    }
    if (!word(scene->bytes + at - 8, &shapes) || scene->tool < 1 || scene->tool > 8 ||
        scene->width < 1 || scene->width > 32 || scene->fill > 3 ||
        ((scene->fill & 2) && (scene->tool != 4 || shapes == 0)) || scene->undo > 16 || scene->redo > 16 ||
        shapes > VECTOR_SHAPES)
    {
        return 0;
    }
    for (i = 0; i < shapes; ++i)
    {
        VectorShape *shape = &scene->shapes[i];
        unsigned long count;
        unsigned int j;
        if (scene->used - at < 32 || !word(scene->bytes + at, &shape->color) ||
            !word(scene->bytes + at + 8, &shape->width) || !word(scene->bytes + at + 16, &shape->fill) ||
            !word(scene->bytes + at + 24, &count) || shape->color > 0xffffffUL || shape->width > 32 ||
            shape->fill > 1 || (!shape->width && !shape->fill) || count < 2 || count > 4096 ||
            count > VECTOR_NODES - total)
        {
            return 0;
        }
        at += 32;
        shape->first = total;
        shape->count = (unsigned int)count;
        if (compact && i+1 == shapes)
        {
            DVM_VectorLocalPlan *plan = &scene->local;
            if (plan->source >= i || scene->shapes[plan->source].count != plan->count ||
                plan->overlay != count || !dvm_vector_local_decorate(plan,
                    scene->nodes+scene->shapes[plan->source].first,scene->nodes+total))
            {
                return 0;
            }
            total += (unsigned int)count;
            continue;
        }
        if (count > (scene->used - at) / stride)
        {
            return 0;
        }
        for (j = 0; j < count; ++j)
        {
            VectorNode *node = &scene->nodes[total++];
            if (!word(scene->bytes + at, &node->verb) || !word(scene->bytes + at + 8, &node->x) ||
                !word(scene->bytes + at + 16, &node->y) || !word(scene->bytes + at + 24, &node->cx) ||
                !word(scene->bytes + at + 32, &node->cy) || node->verb > (cubic ? 4UL : 3UL) || node->x > 1000 || node->y > 600 ||
                node->cx > 1000 || node->cy > 600 || (j == 0 && node->verb != 0) ||
                (j > 0 && node->verb == 0 && scene->nodes[total - 2].verb != 3 &&
                 !(scene->tool >= 6 && i + 1 == shapes)) ||
                (j > 0 && node->verb != 0 && scene->nodes[total - 2].verb == 3) ||
                (node->verb != 2 && node->verb != 4 && (node->cx || node->cy)) || (node->verb == 3 && (node->x || node->y)))
            {
                return 0;
            }
            node->c2x = node->c2y = 0;
            if (cubic && (!word(scene->bytes + at + 40, &node->c2x) ||
                !word(scene->bytes + at + 48, &node->c2y) || node->c2x > 1000 || node->c2y > 600 ||
                (node->verb != 4 && (node->c2x || node->c2y))))
            {
                return 0;
            }
            at += stride;
        }
    }
    if (at != scene->used)
    {
        return 0;
    }
    if (local)
    {
        const DVM_VectorLocalPlan *plan = &scene->local;
        if (shapes < 2 || plan->source >= shapes-1 || scene->shapes[plan->source].count != plan->count ||
            scene->shapes[shapes-1].count != plan->overlay ||
            !dvm_vector_local_valid(plan,scene->nodes+scene->shapes[plan->source].first,
                scene->nodes+scene->shapes[shapes-1].first))
        {
            return 0;
        }
        scene->local.valid = armed ? 2 : 1;
    }
    scene->count = (unsigned int)shapes;
    scene->complete = 1;
    vector_local_reconcile((unsigned long)(scene-vectors));
    return 1;
}

/* Uniform logical canvas scale; page stays inside the pane beneath its toolbar.
 * Nonpositive means the current very small pane cannot display a drawing. */
static double vector_base(const DeskItem *item)
{
    if (item->width <= 80 || item->height <= 34)
    {
        return 0;
    }
    return fmin((item->width - 80) / 1040.0, (item->height - 34) / 640.0);
}


/* Apply copied owner camera state to page units; no document state is mutated. */
static double vector_scale(const DeskItem *item)
{
    return vector_base(item) * vectors[item->id].zoom / 100.0;
}


/* Project one page coordinate around the independently copied viewport centre. */
static double vector_x(const DeskItem *item, double x)
{
    return 40 + vector_base(item) * 520 + vector_scale(item) * (x - vectors[item->id].center_x);
}


/* Vertical companion to vector_x, in the same containing pane's local pixels. */
static double vector_y(const DeskItem *item, double y)
{
    return 28 + vector_base(item) * 320 + vector_scale(item) * (y - vectors[item->id].center_y);
}


/* Intersect the current native scissor with the fixed canvas viewport. The
 * caller restores all four copied bounds before returning on this owner. */
static void vector_clip(const DeskItem *item, const FVM_Affine *world, int saved[4])
{
    double base = vector_base(item);
    double left = 1e30;
    double top = 1e30;
    double right = -1e30;
    double bottom = -1e30;
    unsigned int corner;
    saved[0] = capture_left;
    saved[1] = capture_top;
    saved[2] = capture_right;
    saved[3] = capture_bottom;
    for (corner = 0; corner < 4; ++corner)
    {
        double x = 40 + base * (corner & 1 ? 1020 : 20);
        double y = 28 + base * (corner & 2 ? 620 : 20);
        double px = world->xx * x + world->xy * y + world->x0;
        double py = world->yx * x + world->yy * y + world->y0;
        left = fmin(left, px);
        top = fmin(top, py);
        right = fmax(right, px);
        bottom = fmax(bottom, py);
    }
    capture_left = (int)fmax(capture_left, floor(left));
    capture_top = (int)fmax(capture_top, floor(top));
    capture_right = (int)fmin(capture_right, ceil(right));
    capture_bottom = (int)fmin(capture_bottom, ceil(bottom));
}


/* Retire one borrowed scissor scope, including empty/failed native drawing. */
static void vector_unclip(const int saved[4])
{
    capture_left = saved[0];
    capture_top = saved[1];
    capture_right = saved[2];
    capture_bottom = saved[3];
}


/* Adapt native stamp/outline preview placement to the copied page camera.
 * Borrow all values; the renderer receives detached geometry through emit. */
static int vector_preview(unsigned long widget, const DeskItem *item, const FVM_Affine *world,
                          DVM_InkEmit emit, void *user)
{
    FVM_Affine camera = *world;
    double dx = vector_x(item, 0) - 40 - 20 * vector_scale(item);
    double dy = vector_y(item, 0) - 28 - 20 * vector_scale(item);
    camera.x0 += world->xx * dx + world->xy * dy;
    camera.y0 += world->yx * dx + world->yy * dy;
    int saved[4];
    int ok;
    vector_clip(item, world, saved);
    ok = dvm_ink_preview(widget, &camera, vector_scale(item), emit, user);
    vector_unclip(saved);
    return ok;
}

#include "desktop_node_icons.h"


/* Draw one small copied line/curve icon through ordinary MovieClip methods.
 * cell selects presentation only; all control admission/semantics remain on Dis.
 * Caller owns the context and child clip; zero propagates a VM drawing error. */
static int vector_icon(FVM_Object *clip, unsigned int cell, double wide, unsigned long color)
{
    if (cell >= 23)
    {
        return dvm_node_icon(owner,clip,cell-23,wide,color);
    }
    FVM_Value args[4];
    FVM_Value ignored;
    double scale = fmin(1.0, (wide - 8) / 18.0);
    double x[18];
    double y[18];
    unsigned int count = 0;
    unsigned int i;
    if (scale <= 0)
    {
        return 1;
    }
    if (cell == 19)
    {
        for (i = 0; i < 13; ++i)
        {
            x[i] = -2 + cos(i * 3.141592653589793 / 6) * 5;
            y[i] = -2 + sin(i * 3.141592653589793 / 6) * 5;
        }
        x[13] = 2;
        y[13] = 2;
        x[14] = 8;
        y[14] = 8;
        count = 15;
    }
    else if (cell == 17 || cell == 18)
    {
        x[0] = -6;
        y[0] = -8;
        x[1] = -6;
        y[1] = 7;
        x[2] = -2;
        y[2] = 3;
        x[3] = 2;
        y[3] = 8;
        x[4] = 5;
        y[4] = 6;
        x[5] = 1;
        y[5] = 1;
        x[6] = 7;
        y[6] = 0;
        x[7] = -6;
        y[7] = -8;
        count = 8;
        if (cell == 18)
        {
            x[8] = 7;
            y[8] = -8;
            x[9] = 7;
            y[9] = -4;
            count = 10;
        }
    }
    else if (cell == 0)
    {
        for (i = 0; i < 9; ++i)
        {
            x[i] = -7 + i * 1.75;
            y[i] = sin(i * 0.8) * 4;
        }
        count = 9;
    }
    else if (cell == 1 || cell == 2 || cell == 14)
    {
        count = cell == 2 ? 17 : 5;
        for (i = 0; i < count; ++i)
        {
            if (cell == 2)
            {
                x[i] = cos(i * 3.141592653589793 / 8) * 7;
                y[i] = sin(i * 3.141592653589793 / 8) * 5;
            }
            else
            {
                x[i] = i == 0 || i >= 3 ? -6 : 6;
                y[i] = i < 2 || i == 4 ? -5 : 5;
            }
        }
    }
    else if (cell == 15)
    {
        x[0] = -12;
        y[0] = 6;
        x[1] = -10;
        y[1] = -3;
        x[2] = -4;
        y[2] = -7;
        x[3] = -2;
        y[3] = -4;
        x[4] = -8;
        y[4] = 4;
        x[5] = -12;
        y[5] = 6;
        count = 6;
    }
    else if (cell == 16)
    {
        x[0] = -7;
        y[0] = 0;
        x[1] = -2;
        y[1] = 5;
        x[2] = 7;
        y[2] = -6;
        count = 3;
    }
    else if (cell == 3)
    {
        x[0] = -7;
        y[0] = 5;
        x[1] = 7;
        y[1] = -5;
        count = 2;
    }
    else if (cell == 4 || cell == 5)
    {
        double sign = cell == 4 ? 1 : -1;
        x[0] = 7 * sign;
        y[0] = 4;
        x[1] = 7 * sign;
        y[1] = -3;
        x[2] = -6 * sign;
        y[2] = -3;
        x[3] = -2 * sign;
        y[3] = -7;
        x[4] = -6 * sign;
        y[4] = -3;
        x[5] = -2 * sign;
        y[5] = 1;
        count = 6;
    }
    else if (cell == 6 || cell == 7)
    {
        x[0] = -6;
        y[0] = 0;
        x[1] = 6;
        y[1] = 0;
        count = 2;
    }
    fvm_value_number(args, 1.6);
    fvm_value_number(args + 1, color);
    fvm_value_number(args + 2, 100);
    if (fvm_object_call(owner, clip, "lineStyle", 3, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (i = 0; i < count; ++i)
    {
        fvm_value_number(args, wide / 2 + (x[i] + (cell == 15 ? 7 : 0)) * scale);
        fvm_value_number(args + 1, 10 + y[i] * scale);
        if (fvm_object_call(owner, clip, i ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    if (cell == 7)
    {
        fvm_value_number(args, wide / 2);
        fvm_value_number(args + 1, 10 - 6 * scale);
        if (fvm_object_call(owner, clip, "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
        fvm_value_number(args + 1, 10 + 6 * scale);
        if (fvm_object_call(owner, clip, "lineTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}

/* Update16 real child MovieClips with copied state. Source and thumbnail each
 * own their children through parent edges; workers receive only detached paths.
 * Null scene hides pooled controls when a slot becomes an ordinary text pane.
 * This function presents values and never chooses/executes a document command. */
static int vector_toolbar(FVM_Object *clip, unsigned long slot, const DeskItem *content, const VectorScene *scene)
{
    static const unsigned long palette[6] = {0x245ac4UL, 0xe04444UL, 0x229955UL, 0xf0b323UL, 0x222222UL, 0x9933bbUL};
    unsigned int i;
    unsigned long key[9];
    static unsigned long keys[DESK_SLOTS][9]; /* Copied toolbar pixels/style identity. */
    key[8] = content->height;
    key[0] = content->width;
    key[1] = scene ? scene->tool : 0;
    key[2] = scene ? scene->color : 0;
    key[3] = scene ? scene->width : 0;
    key[4] = scene ? scene->fill : 0;
    key[5] = scene ? scene->undo : 0;
    key[6] = scene ? scene->redo : 0;
    key[7] = scene != NULL;
    if (vector_buttons[slot][0] && !memcmp(key, keys[slot], sizeof(key)))
    {
        goto report;
    }
    for (i = 0; i < 32; ++i)
    {
        FVM_Object *button = vector_buttons[slot][i];
        if (!scene || content->width < 80 || (i == 16 && scene->tool != 4) ||
            (i >= 20 && i <= 22) || (i >= 23 && scene->tool != 7))
        {
            if (button && !number(button, "_visible", 0))
            {
                return 0;
            }
            continue;
        }
        if (!button)
        {
            char name[32];
            snprintf(name, sizeof(name), "vectorButton%u", i);
            button = vector_buttons[slot][i] = child(clip, name, 100 + i);
            if (!button)
            {
                return 0;
            }
        }
        unsigned long fill = 0xe3e8ebUL;

        unsigned long ink = 0x26343fUL;
        int selected = (i < 4 && scene->tool == i + 1) || (i == 15 && scene->tool == 5) || (i >= 17 && scene->tool == i-11);
        if (i >= 8 && i < 14)
        {
            fill = palette[i - 8];
            selected = scene->color == fill;
        }
        if (i == 14)
        {
            selected = (scene->fill & 1) != 0;
        }
        if (selected)
        {

            if (i < 8 || i == 14 || i == 15)
            {
                fill = 0xb8d9f5UL;
            }
        }
        if ((i == 4 && !scene->undo) || (i == 5 && !scene->redo) || (i == 6 && scene->width == 1) ||
            (i == 7 && scene->width == 32) || (i == 16 && !(scene->fill & 2)))
        {
            ink = 0xa4afb7UL;
        }
        double bx;
        double by;
        if (!dvm_vector_control_rect(i, content->width, &bx, &by) ||
            !number(button, "_visible", 1) || !number(button, "_x", bx) || !number(button, "_y", by) ||
            !dvm_chrome_vector(owner, button, dvm_vector_control_width(i,content->width), 24, fill, selected) ||
            !vector_icon(button, i, dvm_vector_control_width(i,content->width), ink))
        {
            return 0;
        }
    }
    memcpy(keys[slot], key, sizeof(key));
report:
    if (trace && scene)
    {
        printf("VECTORCAMERA %lu %lu %lu %lu %lu\n",ticket,slot,scene->zoom,scene->center_x,scene->center_y);
        printf("VECTOR_TOOLBAR %lu %lu %lu %lu %lu %lu %lu %lu\n",
               ticket,
               slot,
               scene->tool,
               scene->color,
               scene->width,
               scene->fill,
               scene->undo,
               scene->redo);
    }
    return 1;
}

/* Copy an immutable native ink path into the current raster frame. Existing
 * capture bounds clip it to the same admitted canvas; workers own all copies. */
static int ink_emit(void *unused, const FVM_Path *path, FVM_RasterColor color, double wide)
{
    (void)unused;
    return dvm_editor_frame_command(
        prepared_frame, path, color, wide, 0, capture_left, capture_top, capture_right, capture_bottom);
}

/* Only native live ink opts into approximate bitmap stamping. Standard Flash
 * contours and final vector paths continue through exact backend rasterization. */
static int preview_emit(void *unused, const FVM_Path *path, FVM_RasterColor color, double wide)
{
    (void)unused;
    if (wide < 0)
    {
        return dvm_editor_frame_xor(prepared_frame, path, -wide,
                                    capture_left, capture_top, capture_right, capture_bottom);
    }
    return dvm_editor_frame_stamp(
        prepared_frame, path, color, wide, capture_left, capture_top, capture_right, capture_bottom);
}

/* Append copied vector commands to the real owner-local MovieClip. Standard
 * Flash drawing methods produce contours later detached for raster workers.
 * Return zero on a VM failure; no document policy, input or file I/O occurs. */
static int vector_paint(FVM_Object *clip, unsigned long slot, const DeskItem *content)
{
    const VectorScene *scene;
    double scale;
    unsigned int i;
    FVM_Value args[4];
    FVM_Value ignored;
    if (content->id > 8 || !vectors[content->id].complete)
    {
        return vector_toolbar(clip, slot, content, NULL);
    }
    scene = &vectors[content->id];
    if (!vector_toolbar(clip, slot, content, scene))
    {
        return 0;
    }
    scale = vector_scale(content);
    if (scale <= 0)
    {
        return 1;
    }
    if (prepared_frame)
    {
        return 1; /* Native immutable commands are emitted after the card body. */
    }
    for (i = 0; i < scene->count; ++i)
    {
        const VectorShape *shape = &scene->shapes[i];
        unsigned int j;
        double start_x = 0;
        double start_y = 0;
        double previous_x = 0;
        double previous_y = 0;
        fvm_value_number(args, shape->width * scale);
        fvm_value_number(args + 1, shape->color);
        fvm_value_number(args + 2, 100);
        if (fvm_object_call(owner, clip, "lineStyle", shape->width ? 3 : 0, args, &ignored) != FVM_OK)
        {
            return 0;
        }
        if (shape->fill)
        {
            fvm_value_number(args, shape->color);
            fvm_value_number(args + 1, 100);
            if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
            {
                return 0;
            }
        }
        for (j = 0; j < shape->count; ++j)
        {
            const VectorNode *node = &scene->nodes[shape->first + j];
            double x = vector_x(content, node->x);
            double y = vector_y(content, node->y);
            if (node->verb == 0)
            {
                start_x = x;
                start_y = y;
            }
            if (node->verb == 3)
            {
                x = start_x;
                y = start_y;
            }
            if (node->verb == 4)
            {
                unsigned int part;
                for (part = 1; part <= 64; ++part)
                {
                    double t = part / 64.0;
                    double u = 1.0 - t;
                    fvm_value_number(args, u*u*u*previous_x + 3*u*u*t*vector_x(content, node->cx) +
                        3*u*t*t*vector_x(content, node->c2x) + t*t*t*x);
                    fvm_value_number(args + 1, u*u*u*previous_y + 3*u*u*t*vector_y(content, node->cy) +
                        3*u*t*t*vector_y(content, node->c2y) + t*t*t*y);
                    if (fvm_object_call(owner, clip, "lineTo", 2, args, &ignored) != FVM_OK)
                    {
                        return 0;
                    }
                }
                previous_x = x;
                previous_y = y;
                continue;
            }
            previous_x = x;
            previous_y = y;
            fvm_value_number(args, x);
            fvm_value_number(args + 1, y);
            if (node->verb == 2)
            {
                fvm_value_number(args, vector_x(content, node->cx));
                fvm_value_number(args + 1, vector_y(content, node->cy));
                fvm_value_number(args + 2, x);
                fvm_value_number(args + 3, y);
            }
            if (fvm_object_call(owner,
                                clip,
                                node->verb == 0 ? "moveTo" : (node->verb == 2 ? "curveTo" : "lineTo"),
                                node->verb == 2 ? 4 : 2,
                                args,
                                &ignored) != FVM_OK)
            {
                return 0;
            }
        }
        if (fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}

/* Build detached vector paths directly from the already validated copied
 * stream. AS/Dis still owns appearance and document geometry; no per-point
 * AVM1 method invocation or VM object allocation is needed for raster work. */
static int vector_capture(unsigned long widget, const DeskItem *content, const FVM_Affine *world)
{
    const VectorScene *scene = &vectors[content->id];
    unsigned int shape_index;
    int saved[4];
    double scale = vector_scale(content);
    (void)widget;
    if (scale <= 0)
    {
        return 1;
    }
    vector_clip(content, world, saved);
    for (shape_index = 0; shape_index < scene->count; ++shape_index)
    {
        const VectorShape *shape = &scene->shapes[shape_index];
        FVM_Path *path = NULL;
        FVM_Result result = fvm_path_create(&path);
        FVM_RasterColor color = {
            (unsigned char)(shape->color >> 16), (unsigned char)(shape->color >> 8), (unsigned char)shape->color, 255};
        unsigned int index;
        for (index = 0; result == FVM_OK && index < shape->count; ++index)
        {
            const VectorNode *node = &scene->nodes[shape->first + index];
            FVM_PathCommand command;
            memset(&command, 0, sizeof(command));
            command.verb = node->verb == 0   ? FVM_PATH_MOVE
                           : node->verb == 1 ? FVM_PATH_LINE
                           : node->verb == 2 ? FVM_PATH_QUADRATIC
                           : node->verb == 4 ? FVM_PATH_CUBIC
                                             : FVM_PATH_CLOSE;
            command.x[0] = vector_x(content, node->x);
            command.y[0] = vector_y(content, node->y);
            if (node->verb == 2)
            {
                command.x[1] = command.x[0];
                command.y[1] = command.y[0];
                command.x[0] = vector_x(content, node->cx);
                command.y[0] = vector_y(content, node->cy);
            }
            if (node->verb == 4)
            {
                command.x[2] = command.x[0];
                command.y[2] = command.y[0];
                command.x[0] = vector_x(content, node->cx);
                command.y[0] = vector_y(content, node->cy);
                command.x[1] = vector_x(content, node->c2x);
                command.y[1] = vector_y(content, node->c2y);
            }
            result = fvm_path_add(path, &command);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, world);
        }
        if (result != FVM_OK || (shape->fill && !ink_emit(NULL, path, color, 0)) ||
            (shape->width && !ink_emit(NULL, path, color, shape->width * scale * world->xx)))
        {
            fvm_path_destroy(path);
            vector_unclip(saved);
            return 0;
        }
        fvm_path_destroy(path);
    }
    vector_unclip(saved);
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
        unsigned long old_zoom = tablezoom;
        unsigned long old_offset = offset;
        long old_pan = tablepan;
        unsigned long old_reveal = reveal_epoch;
        unsigned long encoded_pan = 0;
        double from_zoom;
        double from_pan;
        double from_scroll;
        camera_view(&from_zoom, &from_pan, &from_scroll);
        if (pending || message->first < 240 || message->first > 2048 || message->second < 160 ||
            message->second > 2048 || (message->size != 32 && message->size != 40 && message->size != 48) ||
            !word(message->text, &ticket) || !ticket || !word(message->text + 8, &count) || count >= DESK_SLOTS ||
            !word(message->text + 16, &offset) || offset > 8192 || !word(message->text + 24, &tablezoom) ||
            tablezoom < 10 || tablezoom > 180)
        {
            return 0;
        }
        tablepan = 0;
        if (message->size >= 40)
        {
            if (!word(message->text + 32, &encoded_pan) || (encoded_pan > 4096 && encoded_pan < 0xfffff000UL))
            {
                return 0;
            }
            tablepan = encoded_pan <= 4096 ? (long)encoded_pan : -1L - (long)(0xffffffffUL - encoded_pan);
        }
        if (message->size == 48 && !word(message->text + 40, &reveal_epoch))
        {
            return 0;
        }
        if (active && (old_zoom != tablezoom || old_offset != offset || old_pan != tablepan ||
                       width != (unsigned int)message->first || height != (unsigned int)message->second))
        {
            memset(motions, 0, sizeof(motions));
            camera_motion.active = 0;
            if (old_reveal != reveal_epoch && width == (unsigned int)message->first &&
                height == (unsigned int)message->second)
            {
                camera_motion.from_x = from_pan;
                camera_motion.from_y = from_scroll;
                camera_motion.from_scale = from_zoom;
                camera_motion.to_x = tablepan;
                camera_motion.to_y = offset;
                camera_motion.to_scale = tablezoom / 100.0;
                camera_motion.elapsed = 0;
                camera_motion.active = 1;
            }
        }
        width = (unsigned int)message->first;
        height = (unsigned int)message->second;
        memset(vectors, 0, sizeof(vectors));
        expected = (unsigned int)count;
        received = 0;
        pending = 1;
        end_waiting = 0;
        if (getenv("DVM_EDITOR_BULK_STATS"))
        {
            printf("BULKFRAME %lu\n", ticket);
        }
        return 1;
    }
    if (message->code == DVM_DESK_COLORS)
    {
        DeskItem *item = NULL;
        unsigned long scalars;
        size_t prefix = 0;
        size_t byte;
        if ((!pending && !active) || message->flags != ticket || message->first < 1 || message->first > 8 ||
            message->second < 0 || message->second > 0x7fffffffL || message->size > 200)
        {
            return 0;
        }
        for (index = 0; index < received; ++index)
        {
            if (items[index].id == (unsigned long)message->first && items[index].line)
            {
                item = &items[index];
            }
        }
        if (!item || fvm_utf8_validate(item->text, item->size, &scalars) != FVM_UTF8_OK)
        {
            return 0;
        }
        for (byte = 0; byte < item->size; ++byte)
        {
            if (((unsigned char)item->text[byte] & 0xc0U) != 0x80U)
            {
                ++prefix;
            }
            if (item->text[byte] == '\n')
            {
                break;
            }
        }
        if (byte == item->size || prefix > scalars || message->size != scalars - prefix)
        {
            return 0;
        }
        for (byte = 0; byte < message->size; ++byte)
        {
            if (message->text[byte] < '0' || message->text[byte] > '5')
            {
                return 0;
            }
        }
        memset(item->colors, 0, sizeof(item->colors));
        for (byte = 0; byte < message->size; ++byte)
        {
            item->colors[prefix + byte] = (unsigned char)(message->text[byte] - '0');
        }
        if (trace)
        {
            printf("COLORS %lu %ld %ld %lu %.*s %s\n",
                   ticket,
                   message->first,
                   message->second,
                   (unsigned long)message->size,
                   (int)message->size,
                   message->text,
                   pending ? "staged" : "paint");
        }
        return pending ? 1 : 2;
    }
    if (!pending || message->flags != ticket)
    {
        return 0;
    }
    if (message->code == DVM_DESK_VECTOR_BEGIN || message->code == DVM_DESK_VECTOR_DATA)
    {
        VectorScene *scene;
        DeskItem *item = NULL;
        if (message->first < 1 || message->first > 8)
        {
            return 0;
        }
        for (index = 0; index < received; ++index)
        {
            if (items[index].id == (unsigned long)message->first)
            {
                item = &items[index];
            }
        }
        if (!item || item->line || item->mirror || (item->parent != 1 && item->parent != 4 && item->parent < 5))
        {
            return 0;
        }
        scene = &vectors[message->first];
        if (message->code == DVM_DESK_VECTOR_BEGIN)
        {
            unsigned long revision;
            unsigned long version;
            if (scene->expected || message->size != 16 || !word(message->text, &revision) ||
                !word(message->text + 8, &version) || version != DVM_DESK_VECTOR_VERSION || revision > 0x7fffffffUL ||
                message->second < 56 || message->second > VECTOR_BYTES)
            {
                return 0;
            }
            scene->expected = (size_t)message->second;
            return 1;
        }
        if (!scene->expected || scene->complete || message->second < 0 || (size_t)message->second != scene->used ||
            !message->size || message->size > 1024 || message->size > scene->expected - scene->used)
        {
            return 0;
        }
        memcpy(scene->bytes + scene->used, message->text, message->size);
        scene->used += message->size;
        if (scene->used == scene->expected)
        {
            int parsed = vector_parse(scene);
            if (parsed && end_waiting)
            {
                unsigned int pane;
                DVM_UIMessage end;
                for (pane = 1; pane <= 8; pane++)
                {
                    if (vectors[pane].expected && !vectors[pane].complete)
                    {
                        return 1;
                    }
                }
                memset(&end, 0, sizeof(end));
                end.kind = DVM_UI_VIEW;
                end.code = DVM_DESK_END;
                end.flags = ticket;
                return dvm_desktop_message(&end) == 1 ? 4 : 0;
            }
            return parsed;
        }
        return 1;
    }
    if (message->code == DVM_DESK_END)
    {
        unsigned int declarations = 0;
        if (message->first < 0 || message->first > 8 || message->second)
        {
            return 0;
        }
        for (index = 1; index <= 8; index++)
        {
            declarations += vectors[index].expected != 0;
        }
        if (message->first && (unsigned long)message->first != declarations)
        {
            return 0;
        }
        /* The only additional hierarchy is document -> MDI root -> table.
         * Validate the entire copied forest before presentation; no cycles or
         * arbitrary native parent addresses can be proposed. */
        for (index = 0; index < received; ++index)
        {
            unsigned int root_index;
            unsigned long root_id;
            int found = 0;
            if (items[index].parent < 5)
            {
                continue;
            }
            root_id = items[index].parent == 12 ? 28 : items[index].parent + 8;
            for (root_index = 0; root_index < index; ++root_index)
            {
                if (items[root_index].id == root_id &&
                    (items[root_index].parent == 1 || items[root_index].parent == 4) && !items[root_index].mirror)
                {
                    found = 1;
                }
            }
            if (!found)
            {
                return 0;
            }
        }
        if (message->size || received != expected)
        {
            return 0;
        }
        for (index = 1; index <= 8; ++index)
        {
            if (vectors[index].expected && !vectors[index].complete)
            {
                if (!message->first)
                {
                    return 0; /* Legacy End still requires all fragments now. */
                }
                end_waiting = 1;
                return 3; /* Accepted End; publication remains pending, no input acknowledgement. */
            }
        }
        end_waiting = 0;
        memset(items + received, 0, (DESK_SLOTS - received) * sizeof(*items));
        for (index = 1; index <= 8; ++index)
        {
            if (vectors[index].complete)
            {
                dvm_ink_commit(index, vectors[index].bytes, vectors[index].used);
            }
            else
            {
                dvm_ink_drop(index);
            }
        }
        pending = 0;
        active = 1;
        motion_commit();
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
    if (words[0] > 12 || (words[0] >= 5 && ((unsigned long)message->first > 8 || words[7])) ||
        (words[1] > 65536 && words[1] < 0xffff8000UL) || (words[2] > 65536 && words[2] < 0xffff8000UL) || !words[3] ||
        words[3] > 32768 || !words[4] || words[4] > 32768 || words[5] > 80000005UL || (words[5] % 8) > 6 ||
        words[7] >= DESK_SLOTS)
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
        memset(item->colors, 0, sizeof(item->colors));
        memset(item->text, 0, sizeof(item->text));
        memcpy(item->text, message->text + DESK_META, item->size);
        if (fvm_utf8_validate(item->text, item->size, &scalars) != FVM_UTF8_OK ||
            (words[6] != 0xffffffffUL && words[6] > scalars) || (words[8] != 0xffffffffUL && words[8] > scalars))
        {
            return 0;
        }
        item->id = (unsigned long)message->first;
        item->parent = words[0];
        item->x = words[1] <= 65536 ? (long)words[1] : -1L - (long)(0xffffffffUL - words[1]);
        item->y = words[2] <= 65536 ? (long)words[2] : -1L - (long)(0xffffffffUL - words[2]);
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

/* Compose actual local/parent matrices into caller-owned world output. parent
 * is a validated slot0..12; MDI parents recurse once through the table. Return
 * zero on failed observations. Owner-thread only; no managed reference escapes. */
static int world_matrix(FVM_Object *clip, unsigned long parent, FVM_Affine *output)
{
    FVM_Affine local;
    FVM_Affine group;
    unsigned long ancestor = 1;
    if (parent >= 5)
    {
        unsigned int index;
        unsigned long identity = parent == 12 ? 28 : parent + 8;
        for (index = 0; index < received; ++index)
        {
            if (items[index].id == identity)
            {
                ancestor = items[index].parent;
                break;
            }
        }
    }
    if (fvm_movieclip_get_matrix(owner, clip, &local) != FVM_OK ||
        (parent >= 5 ? !world_matrix(groups[parent], ancestor, &group)
                     : fvm_movieclip_get_matrix(owner, groups[parent], &group) != FVM_OK))
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
    if (!owner || !active || pending || camera_motion.active || !isfinite(x) || !isfinite(y) || x < 80 ||
        x >= width - right_extent() || y < 48 || y >= height)
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
        if ((item->id <= 8 && motions[item->id].active) ||
            (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active))
        {
            continue;
        }
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
        if (vectors[item->id].complete)
        {
            unsigned int index;
            for (index = 0; index < 32; ++index)
            {
                double bx;
                double by;
                if ((index >= 20 && index <= 22) || (index >= 23 && vectors[item->id].tool != 7) ||
                    (index == 16 && !(vectors[item->id].fill & 2)))
                {
                    continue;
                }
                if (dvm_vector_control_rect(index, item->width, &bx, &by) &&
                    localx >= bx && localx < bx + dvm_vector_control_width(index,item->width) && localy >= by && localy < by + 24)
                {
                    *widget = item->id;
                    *position = -2 - (long)index;
                    return 1;
                }
            }
            double scale = vector_scale(item);
            double vx = scale > 0 ? (localx - vector_x(item, 0)) / scale : -1;
            double vy = scale > 0 ? (localy - vector_y(item, 0)) / scale : -1;
            if (vx < 0 || vx > 1000 || vy < 0 || vy > 600)
            {
                return 0;
            }
            *widget = item->id;
            *position = (long)floor(vy) * 1001 + (long)floor(vx);
            if (trace)
            {
                printf("VECTORHIT %lu %lu %ld\n", ticket, item->id, *position);
            }
            return 1;
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
             dvm_chrome_box(owner, cursor, 1, options.line_height, 0x203747UL, 0);
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
        ok = (!mark || draw_clip(mark, surface, &identity) == FVM_OK) &&
             capture_path(NULL, path, ink, 0, 0) == FVM_OK &&
             (!cursor || draw_clip(cursor, surface, &identity) == FVM_OK);
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
    ok = (!mark || fvm_movieclip_render_view(owner, mark, &tile, &shift) == FVM_OK) &&
         fvm_path_transform(path, &shift) == FVM_OK && fvm_path_draw(path, &tile, ink, 0, 0) == FVM_OK &&
         (!cursor || fvm_movieclip_render_view(owner, cursor, &tile, &shift) == FVM_OK);
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

/* Atomically replace detached colour contours using the same owner-held TextField.
 * content is copied scalar data; label/id belong to this VM owner. No worker sees
 * a VM/font handle. Failure retains the old cache and destroys every candidate. */
static int syntax_cache(FVM_Object *label, const DeskItem *content, unsigned long id)
{
    FVM_Path *paths[5] = {NULL, NULL, NULL, NULL, NULL};
    FVM_TextPathRange ranges[DESK_TEXT];
    size_t boundaries[DESK_TEXT + 1];
    size_t scalars = 0;
    size_t byte;
    unsigned int group;
    for (byte = 0; byte < content->size; ++byte)
    {
        if (((unsigned char)content->text[byte] & 0xc0U) != 0x80U)
        {
            boundaries[scalars++] = byte;
        }
    }
    boundaries[scalars] = content->size;
    for (group = 0; group < 5; ++group)
    {
        size_t count = 0;
        size_t scalar = 0;
        while (scalar < scalars)
        {
            unsigned int kind = content->colors[scalar];
            unsigned int selected = kind < 2 ? 0 : kind - 1;
            if (selected != group)
            {
                ++scalar;
                continue;
            }
            ranges[count].first = boundaries[scalar++];
            while (scalar < scalars)
            {
                kind = content->colors[scalar];
                selected = kind < 2 ? 0 : kind - 1;
                if (selected != group)
                {
                    break;
                }
                ++scalar;
            }
            ranges[count++].last = boundaries[scalar];
        }
        if (fvm_textfield_path_ranges(owner, label, ranges, count, &paths[group]) != FVM_OK)
        {
            unsigned int discard;
            for (discard = 0; discard < 5; ++discard)
            {
                fvm_path_destroy(paths[discard]);
            }
            return 0;
        }
    }
    fvm_path_destroy(text_paths[id]);
    text_paths[id] = paths[0];
    for (group = 0; group < 4; ++group)
    {
        fvm_path_destroy(syntax_paths[id][group]);
        syntax_paths[id][group] = paths[group + 1];
    }
    text_keys[id] = *content;
    return 1;
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
    if (clips[item->id] && parents[item->id] != item->parent)
    {
        FVM_Value ignored;
        FVM_Object *previous = clips[item->id];
        /* [GC:] Source owners do not move. Retire only this presentation subtree;
         * discard every borrowed child handle before an allocation can collect it.
         * Immutable raster paths are independent of these managed objects. */
        if ((item->id >= 13 && item->id <= 19) || item->id == 28)
        {
            unsigned int group = item->id == 28 ? 12 : (unsigned int)item->id - 8;
            unsigned int child_id;
            /* [GC:] Removing a reparented container retires its entire managed
             * subtree. Drop every borrowed child handle before any allocation
             * can collect it, even when a child's numeric parent slot is stable. */
            groups[group] = NULL;
            for (child_id = 0; child_id < DESK_SLOTS; ++child_id)
            {
                if (parents[child_id] == group)
                {
                    clips[child_id] = labels[child_id] = marks[child_id] = cursors[child_id] = NULL;
                    memset(vector_buttons[child_id], 0, sizeof(vector_buttons[child_id]));
                    free(drag_pixels[child_id]);
                    drag_pixels[child_id] = NULL;
                }
            }
        }
        clips[item->id] = NULL;
        labels[item->id] = NULL;
        marks[item->id] = NULL;
        cursors[item->id] = NULL;
        memset(vector_buttons[item->id], 0, sizeof(vector_buttons[item->id]));
        free(drag_pixels[item->id]);
        drag_pixels[item->id] = NULL;
        if (fvm_object_call(owner, previous, "removeMovieClip", 0, NULL, &ignored) != FVM_OK)
        {
            return 0;
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
            ok = number(format, "size", (item->parent == 3 || (item->id >= 72 && item->id <= 88)) ? 8 : 11) &&
                 number(format, "leading", 2);
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
    if ((item->id >= 13 && item->id <= 19) || item->id == 28)
    {
        groups[item->id == 28 ? 12 : item->id - 8] = clip;
    }
    label = labels[item->id];
    {
        FVM_Value ignored;
        if (fvm_object_call(owner, cursors[item->id], "clear", 0, NULL, &ignored) != FVM_OK ||
            fvm_object_call(owner, marks[item->id], "clear", 0, NULL, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    if ((item->id == 11 || item->id == 12)
            ? !dvm_chrome_glass(owner,
                                clip,
                                content->width,
                                content->height,
                                item->id == 12,
                                &glass_width[item->id == 12],
                                &glass_height[item->id == 12])
            : !dvm_chrome_owns_view(item->id, item->mirror) &&
                  !dvm_chrome_box(
                      owner,
                      clip,
                      content->width,
                      content->height,
                      (item->id >= 41 && item->id <= 55 && content->style != 3)
                          ? 0x252b2eUL
                          : (((item->id >= 13 && item->id <= 19) || item->id == 28) ? 0xeff2f1UL
                                                                                    : colors[content->style]),
                      item->id <= 8 && (content->style == 1 || content->style == 5)
                          ? 0x2684dbUL
                          : (item->id <= 8 || (item->id >= 13 && item->id <= 28) ? 0xb9c4caUL : 0x4d575bUL)))
    {
        fprintf(stderr, "desktop box\n");
        return 0;
    }
    DVM_ChromeItem chrome = {item->id,
                             content->id <= 8 && vectors[content->id].complete && !prepared_frame,
                             content->style == 1 || content->style == 5,
                             content->style == 3,
                             scale,
                             item->width,
                             item->height,
                             item->id == 55 && content->style == 2,
                             item->id >= 30 && item->id <= 37 && strcmp(content->text, "x") == 0,
                             item->mirror};
    if (!dvm_chrome_decorate(owner, clip, &chrome, content->width, content->height))
    {
        return 0;
    }
    if (!vector_paint(clip, item->id, content))
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
    GLOBAL_NUMBER("ideRightX", width - right_extent());
    double widget_x = item->x;
    double widget_y = item->y;
    if (item->id <= 8 && motions[item->id].active)
    {
        motion_local(&motions[item->id], &widget_x, &widget_y, &scale, item->parent);
    }
    GLOBAL_NUMBER("ideWidgetX", widget_x);
    GLOBAL_NUMBER("ideWidgetY", widget_y);
    GLOBAL_NUMBER("ideWidgetWidth", content->width);
    GLOBAL_NUMBER("ideWidgetHeight", content->height);
    GLOBAL_NUMBER("ideWidgetScale", scale * 100);
    GLOBAL_NUMBER("ideLabelWidth",
                  content->width > (content->line ? 40U : 8U) ? content->width - (content->line ? 40 : 8) : 1);
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
        double visual_zoom;
        double visual_pan;
        double visual_scroll;
        camera_view(&visual_zoom, &visual_pan, &visual_scroll);
        if (!number(groups[1], "_xscale", visual_zoom * 100) || !number(groups[1], "_yscale", visual_zoom * 100) ||
            !number(groups[1], "_x", 84.0 * (1.0 - visual_zoom) + visual_pan) ||
            !number(groups[1], "_y", 48.0 * (1.0 - visual_zoom) - visual_scroll))
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
        if (item->id <= 8 && (item->parent == 1 || item->parent == 4) && item->x == 65536 && item->y == 65536 &&
            !motions[item->id].active)
        {
            return 1;
        }
        result = draw_widget_clip(item, surface);
        if (result == FVM_OK && prepared_frame && content->id <= 8 && vectors[content->id].complete)
        {
            FVM_Affine ink_world;
            if (!world_matrix(clip, item->parent, &ink_world))
            {
                return 0;
            }
            if (prepared_retained && vectors[content->id].used >= 48 &&
                vectors[content->id].used - 48 + 2 * sizeof(item->width) <= DVM_EDITOR_NODE_KEY_BYTES &&
                !item->mirror && item->style < 5 && !motions[item->id].active && !camera_motion.active &&
                ink_world.xy == 0 && ink_world.yx == 0 && ink_world.xx > 0 && ink_world.yy > 0 && ink_world.x0 >= 0 &&
                ink_world.y0 >= 48 && ink_world.x0 + ink_world.xx * content->width <= width &&
                ink_world.y0 + ink_world.yy * content->height <= height)
            {
                deferred_vector = 1;
                deferred_world = ink_world;
            }
            else if (!vector_capture(content->id, content, &ink_world) ||
                     (!item->mirror &&
                      !vector_preview(content->id, content, &ink_world, preview_emit, NULL)))
            {
                return 0;
            }
        }
        if (result != FVM_OK)
        {
            fprintf(stderr, "desktop movie render: %d\n", result);
            return 0;
        }
        /* Moving a clip must not reshape every glyph. These outlines contain
         * no GC pointers and are invalidated by every text/extent change. */
        if (!text_paths[item->id] || text_keys[item->id].width != content->width ||
            text_keys[item->id].height != content->height || text_keys[item->id].size != content->size ||
            memcmp(text_keys[item->id].text, content->text, content->size) ||
            memcmp(text_keys[item->id].colors, content->colors, sizeof(content->colors)))
        {
            if (!syntax_cache(label, content, item->id))
            {
                return 0;
            }
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
        ok = ok && field_ink(path, NULL, marks[item->id], &world, item, content, surface, ink, 0);
        if (ok && caption_path)
        {
            FVM_RasterColor caption_ink = {176, 36, 48, 255};
            ok = field_ink(caption_path, NULL, NULL, &world, item, content, surface, caption_ink, 1);
        }
        if (ok && text_paths[item->id])
        {
            static const FVM_RasterColor palette[4] = {
                {174, 99, 19, 255}, {31, 126, 65, 255}, {110, 57, 169, 255}, {113, 123, 137, 255}};
            unsigned int group;
            for (group = 0; ok && group < 4; ++group)
            {
                FVM_Path *colored = NULL;
                ok = fvm_path_create(&colored) == FVM_OK &&
                     fvm_path_append(colored, syntax_paths[item->id][group], NULL) == FVM_OK &&
                     fvm_path_transform(colored, &placement) == FVM_OK &&
                     field_ink(colored, NULL, NULL, &world, item, content, surface, palette[group], 0);
                fvm_path_destroy(colored);
            }
        }
        if (ok && content->caret >= 0)
        {
            FVM_Path *empty = NULL;
            ok = fvm_path_create(&empty) == FVM_OK &&
                 field_ink(empty, cursors[item->id], NULL, &world, item, content, surface, ink, 0);
            fvm_path_destroy(empty);
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
                !dvm_chrome_invalidate(owner, clips[id]) ||
                fvm_object_call(owner, clips[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_call(owner, cursors[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_call(owner, marks[id], "clear", 0, NULL, &ignored) != FVM_OK ||
                fvm_object_get(owner, labels[id], "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
                fvm_value_string_size(&value) != 0)
            {
                return 0;
            }
            if (id == 11 || id == 12)
            {
                glass_width[id - 11] = 0;
                glass_height[id - 11] = 0;
            }
            free(drag_pixels[id]);
            drag_pixels[id] = NULL;
            fvm_path_destroy(text_paths[id]);
            text_paths[id] = NULL;
            {
                unsigned int group;
                for (group = 0; group < 4; ++group)
                {
                    fvm_path_destroy(syntax_paths[id][group]);
                    syntax_paths[id][group] = NULL;
                }
            }
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
    if ((item->id <= 8 && vectors[item->id].complete) || tablezoom != 100 || tablepan != 0 || camera_motion.active ||
        item->id > 8 || motions[item->id].active || item->parent != 1 || !pixels ||
        (key->style != item->style &&
         !((item->style == 5 && key->style == 1) || (item->style == 6 && key->style == 0))) ||
        key->width != item->width || key->height != item->height || key->size != item->size ||
        key->caret != item->caret || key->anchor != item->anchor || key->line != item->line ||
        memcmp(key->text, item->text, item->size) || memcmp(key->colors, item->colors, sizeof(item->colors)))
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
            if (item->x + x < 0 || item->x + x >= (long)surface->width || (dx && dy && dx * dx + dy * dy > 49))
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
        printf("MC %lu %lu %lu %ld %d ", ticket, item->id, item->parent, item->x, top);
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
    if ((item->id <= 8 && vectors[item->id].complete) || tablezoom != 100 || tablepan != 0 || camera_motion.active ||
        item->id > 8 || item->parent != 1 || top < 48 || item->x < 0 || motions[item->id].active ||
        item->x + item->width > surface->width || top + item->height > surface->height)
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
        if (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active)
        {
            continue;
        }
        if (drag_image(item, surface))
        {
            continue;
        }
        if (item->id <= 8 && (motions[item->id].active || item->style == 5 || item->style == 6))
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
            if ((item->id <= 8 && motions[item->id].active) ||
                (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active))
            {
                continue;
            }
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

/* Copied owner-side content key; placement and transient opacity are excluded. */
typedef struct DeskRetainedKey
{
    DeskItem item;              /* Exact immutable scalar content and local geometry descriptor. */
    unsigned int canvas_width;  /* Resize invalidates clipped local geometry. */
    unsigned int canvas_height; /* Canvas admission belongs to this capture. */
    int eligible;               /* Ordinary pane or immutable control, never a clipped/mirrored/vector snapshot. */
} DeskRetainedKey;

/* Build a borrowed descriptor's initialized native key using its actual placement.
 * Complete visible ordinary panes and immutable controls can bypass MovieClip reconstruction. */
static void retained_key(DeskRetainedKey *key, const DeskItem *item, const FVM_Affine *world)
{
    memset(key, 0, sizeof(*key));
    key->item = *item;
    key->item.x = 0;
    key->item.y = 0;
    key->item.parent = 0;
    if (key->item.style == 5)
    {
        key->item.style = 1;
    }
    if (key->item.style == 6)
    {
        key->item.style = 0;
    }
    key->canvas_width = width;
    key->canvas_height = height;
    key->eligible = (item->id <= 8 || item->parent == 3 || item->id >= 64) && !item->mirror &&
                    !(item->id <= 8 && (vectors[item->id].complete || motions[item->id].active)) &&
                    !camera_motion.active && world->xy == 0 && world->yx == 0 && world->xx > 0 && world->yy > 0 &&
                    world->x0 >= 2 && world->y0 >= 50 && world->x0 + world->xx * item->width <= width - 2 &&
                    world->y0 + world->yy * item->height <= height - 2;
}

/* Keep committed geometry independent of toolbar state and live C samples.
 * Only fully visible opaque source panes qualify; clipped/mirrored/translucent
 * cases retain the original combined capture. Exact key bytes exclude only the
 * six current tool/history header words. Both baseline/frame remain owner-held.
 */
static int vector_layers(const DeskItem *item)
{
    const VectorScene *scene = &vectors[item->id];
    unsigned int id = 96 + (unsigned int)item->id;
    size_t prefix = 2 * sizeof(item->width)+sizeof(local_epochs[0]);
    size_t size = prefix + scene->used - 48;
    unsigned char *key;
    int reused;
    int ok;
    if (size > DVM_EDITOR_NODE_KEY_BYTES || !(key = malloc(size)))
    {
        return 0;
    }
    memcpy(key, &item->width, sizeof(item->width));
    memcpy(key + sizeof(item->width), &item->height, sizeof(item->height));
    memcpy(key+2*sizeof(item->width),local_epochs+item->id,sizeof(local_epochs[0]));
    memcpy(key + prefix, scene->bytes + 48, scene->used - 48);
    ok = dvm_editor_frame_node_begin(prepared_frame, id, (unsigned int)item->id);
    reused = ok && dvm_editor_frame_node_matches(prepared_prior, id, key, size);
    ok = ok && (reused ? dvm_editor_frame_node_replay(prepared_frame, prepared_prior, id)
                       : vector_capture(item->id, item, &deferred_world));
    ok = ok && dvm_editor_frame_node_key(prepared_frame, key, size) &&
         dvm_editor_frame_node_end(prepared_frame,
                                   &deferred_world,
                                   (unsigned int)item->width,
                                   (unsigned int)item->height,
                                   DVM_EDITOR_NODE_OPACITY);
    free(key);
    if (ok && trace)
    {
        printf("VECTORCACHE %lu %lu %d\n", ticket, item->id, reused);
    }
    if (ok && dvm_ink_has_preview(item->id))
    {
        ok = dvm_editor_frame_node_begin(prepared_frame, 104 + (unsigned int)item->id, (unsigned int)item->id) &&
             vector_preview(item->id, item, &deferred_world, preview_emit, NULL) &&
             dvm_editor_frame_node_end(prepared_frame,
                                       &deferred_world,
                                       (unsigned int)item->width,
                                       (unsigned int)item->height,
                                       DVM_EDITOR_NODE_OPACITY);
    }
    return ok;
}

/* Build detached painter commands from the complete owner-local MovieClip scene.
 * Capture ends before returning, including failures; workers receive no VM state. */
static int prepare_desktop(DVM_EditorFrame *frame, const DVM_EditorFrame *prior, int retained)
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
    prepared_retained = retained;
    prepared_prior = prior;
    capture_left = capture_top = 0;
    capture_right = (int)width;
    capture_bottom = (int)height;
    {
        double zoom, pan, scroll;
        camera_view(&zoom, &pan, &scroll);
        if (!number(groups[1], "_xscale", zoom * 100) || !number(groups[1], "_yscale", zoom * 100) ||
            !number(groups[1], "_x", 84.0 * (1.0 - zoom) + pan) ||
            !number(groups[1], "_y", 48.0 * (1.0 - zoom) - scroll))
        {
            prepared_frame = NULL;
            return 0;
        }
    }
    for (index = 0; index < received && ok; ++index)
    {
        DeskItem *item = &items[index];
        if (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active)
        {
            continue;
        }
        int translucent = item->id <= 8 && (motions[item->id].active || item->style == 5 || item->style == 6);
        FVM_Affine world;
        unsigned int parent = item->parent >= 5 ? (item->parent == 12 ? 28 : (unsigned int)item->parent + 8) : 0;
        unsigned int logical_width = (unsigned int)item->width;
        unsigned int logical_height = (unsigned int)item->height;
        unsigned int source;
        if (item->mirror)
        {
            for (source = 0; source < received; ++source)
            {
                if (items[source].id == item->mirror)
                {
                    logical_width = (unsigned int)items[source].width;
                    logical_height = (unsigned int)items[source].height;
                }
            }
        }
        deferred_vector = 0;
        DeskRetainedKey key;
        int reused = 0;
        if (!retained)
        {
            ok = !translucent || dvm_editor_frame_group(frame, 1, 0, 0, (int)width, (int)height);
            ok = ok && paint(item, &surface);
            ok = ok && (!translucent || dvm_editor_frame_group(frame, 0, 0, 0, (int)width, (int)height));
            continue;
        }
        ok = dvm_editor_frame_node_begin(frame, (unsigned int)item->id, parent);
        if (ok && prior && clips[item->id] && parents[item->id] == item->parent &&
            !(item->id <= 8 && motions[item->id].active) && !camera_motion.active && !item->mirror)
        {
            ok = number(clips[item->id], "_x", item->x) && number(clips[item->id], "_y", item->y) &&
                 number(clips[item->id], "_xscale", 100) && number(clips[item->id], "_yscale", 100) &&
                 world_matrix(clips[item->id], item->parent, &world);
            if (ok)
            {
                retained_key(&key, item, &world);
                if (key.eligible && dvm_editor_frame_node_matches(prior, (unsigned int)item->id, &key, sizeof(key)))
                {
                    ok = dvm_editor_frame_node_replay(frame, prior, (unsigned int)item->id);
                    reused = ok;
                }
            }
        }
        if (ok && !reused)
        {
            ok = paint(item, &surface) && world_matrix(clips[item->id], item->parent, &world);
        }
        if (ok)
        {
            if (item->mirror)
            {
                /* Cache the shelf object in physical card pixels, independently
                 * of its preview's source resolution. Captured paths already
                 * carry the actual MovieClip transform. */
                world.xx = world.yy = 1;
                world.xy = world.yx = 0;
                logical_width = (unsigned int)item->width;
                logical_height = (unsigned int)item->height;
            }
            retained_key(&key, item, &world);
            ok = dvm_editor_frame_node_key(frame, &key, sizeof(key)) &&
                 dvm_editor_frame_node_end(
                     frame, &world, logical_width, logical_height, translucent ? 900 : DVM_EDITOR_NODE_OPACITY);
            if (trace && reused)
            {
                size_t byte;
                printf("NODECACHE %lu %lu\n", ticket, item->id);
                printf("GEOMETRY %lu %lu %lu %lu %lu %lu\n",
                       ticket,
                       item->id,
                       item->width,
                       item->height,
                       item->style,
                       item->line);
                printf("MC %lu %lu %lu %.0f %.0f ", ticket, item->id, item->parent, world.x0, world.y0);
                for (byte = 0; byte < item->size; ++byte)
                {
                    printf("%02x", (unsigned char)item->text[byte]);
                }
                printf("\nSELECTION %lu %lu %ld %ld\n", ticket, item->id, item->caret, item->anchor);
            }
        }
        if (ok && deferred_vector)
        {
            ok = vector_layers(item);
        }
    }
    prepared_retained = 0;
    prepared_prior = NULL;
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

/* Prepare owner-held native layers using only a completed immutable baseline. */
int dvm_desktop_prepare_cached(DVM_EditorFrame *frame, const DVM_EditorFrame *prior)
{
    return prepare_desktop(frame, prior, !dvm_ink_has_xor());
}

/* Resource-budget fallback and development comparison: capture ordinary world
 * vector commands without associated object images, retaining the same UI policy. */
int dvm_desktop_prepare_vector(DVM_EditorFrame *frame)
{
    return prepare_desktop(frame, NULL, 0);
}

/* Prepare a standalone snapshot without a displayed baseline; owner only. */
int dvm_desktop_prepare(DVM_EditorFrame *frame)
{
    return dvm_desktop_prepare_cached(frame, NULL);
}

/* Convert the 20-pixel hit budget using the completed pane/page transforms.
 * Owner only; no geometry walks, allocations or managed references are needed. */
unsigned long dvm_desktop_vector_pick_radius(unsigned long widget)
{
    unsigned int index;
    for (index = 0; index < received; ++index)
    {
        const DeskItem *item = &items[index];
        FVM_Affine world;
        double scale;
        if (item->id != widget || item->mirror || !world_matrix(clips[widget], item->parent, &world))
        {
            continue;
        }
        scale = vector_scale(item) * hypot(world.xx, world.yx);
        if (scale > 0)
        {
            return (unsigned long)fmax(1, fmin(2000000, floor(20000.0 / scale)));
        }
    }
    return 20000;
}


/* Map stable capture against completed owner geometry; input never re-hits a
 * different window/toolbar while drawing. Off-page samples clamp, including Up. */
int dvm_desktop_vector_point(unsigned long widget, double x, double y, long *position)
{
    unsigned int index;
    if (!position || widget < 1 || widget > 8 || pending || !vectors[widget].complete)
    {
        return 0;
    }
    for (index = 0; index < received; ++index)
    {
        const DeskItem *item = &items[index];
        FVM_Affine world;
        double determinant;
        double localx;
        double localy;
        double scale;
        if (item->id != widget || item->mirror)
        {
            continue;
        }
        if (!world_matrix(clips[widget], item->parent, &world))
        {
            return 0;
        }
        determinant = world.xx * world.yy - world.xy * world.yx;
        scale = vector_scale(item);
        if (fabs(determinant) < 0.000001 || scale <= 0)
        {
            return 0;
        }
        localx = ((world.yy * (x - world.x0) - world.xy * (y - world.y0)) / determinant - vector_x(item, 0)) / scale;
        localy = ((-world.yx * (x - world.x0) + world.xx * (y - world.y0)) / determinant - vector_y(item, 0)) / scale;
        localx = fmax(0, fmin(1000, localx));
        localy = fmax(0, fmin(600, localy));
        *position = (long)floor(localy) * 1001 + (long)floor(localx);
        return 1;
    }
    return 0;
}

/* Hit the visible header in actual paint order. Coordinates are copied canvas
 * pixels; no source or touch policy is acquired. Return zero for occlusion. */
int dvm_desktop_header(double x, double y)
{
    unsigned int ordinal;
    if (!active || pending || camera_motion.active || !isfinite(x) || !isfinite(y) || y < 48)
    {
        return 0;
    }
    for (ordinal = received; ordinal > 0; --ordinal)
    {
        DeskItem *item = &items[ordinal - 1];
        FVM_Affine world;
        if ((item->id <= 8 && motions[item->id].active) ||
            (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active))
        {
            continue;
        }
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
    dvm_vector_local_destroy(local_edit);
    local_edit = NULL;
    local_widget = 0;
    local_released = 0;
    memset(local_epochs,0,sizeof(local_epochs));
    memset(vectors, 0, sizeof(vectors));
    memset(vector_buttons, 0, sizeof(vector_buttons));
    unsigned int index;
    for (index = 0; index < DESK_SLOTS; ++index)
    {
        fvm_path_destroy(text_paths[index]);
        text_paths[index] = NULL;
        {
            unsigned int group;
            for (group = 0; group < 4; ++group)
            {
                fvm_path_destroy(syntax_paths[index][group]);
                syntax_paths[index][group] = NULL;
            }
        }
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
    memset(&camera_motion, 0, sizeof(camera_motion));
    tablepan = 0;
    reveal_epoch = 0;
    memset(glass_width, 0, sizeof(glass_width));
    memset(glass_height, 0, sizeof(glass_height));
    memset(motions, 0, sizeof(motions));
    memset(previous_present, 0, sizeof(previous_present));
    memset(previous_windows, 0, sizeof(previous_windows));
}

/* No managed object is exposed: style is read from the complete copied stream. */
int dvm_desktop_ink_style(unsigned long widget, unsigned long *tool, unsigned long *color, unsigned long *width)
{
    if (widget < 1 || widget > 8 || !vectors[widget].complete)
    {
        return 0;
    }
    *tool = vectors[widget].tool;
    *color = vectors[widget].color;
    *width = vectors[widget].width;
    return 1;
}


/* Read the last accepted unfinished Line vertex from copied scene geometry.
 * Native capture borrows values only; Limbo remains the figure owner. */
int dvm_desktop_line_anchor(unsigned long widget, long *point)
{
    const VectorScene *scene;
    const VectorShape *shape;
    const VectorNode *node;
    if (!point || widget < 1 || widget > 8)
    {
        return 0;
    }
    scene = &vectors[widget];
    if (!scene->complete || !(scene->fill & 2) || !scene->count)
    {
        return 0;
    }
    shape = &scene->shapes[scene->count - 1];
    node = &scene->nodes[shape->first + shape->count - 1];
    *point = node->y * 1001 + node->x;
    return 1;
}


/* Classify one completed child control without changing document/camera state. */
int dvm_desktop_zoom_button(double x, double y)
{
    unsigned long widget = 0;
    long position = -1;
    return dvm_desktop_hit(x,y,&widget,&position) && widget >= 1 && widget <= 8 && position == -21;
}

#include "desktop_vector_local.c"
