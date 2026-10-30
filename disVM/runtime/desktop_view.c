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
    unsigned long id;                /* Stable controller slot. */
    unsigned long parent;            /* Layer group 0..4. */
    long x;                          /* Signed parent-local pixels; windows may pass under the left rail. */
    unsigned long y;                 /* Parent-local pixels. */
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
static FVM_Object *groups[13];             /* [GC:] Borrowed children of the retained root. */
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
static unsigned long offset;              /* Table scroll magnitude. */
static unsigned int width;                /* Committed canvas pixels. */
static unsigned int height;               /* Committed canvas pixels. */
static int pending;                       /* A declared scene is being assembled. */
static int trace; /* Explicit replay diagnostic mode; ordinary windows do not dump source text. */
static FVM_Path *text_paths[DESK_SLOTS];       /* Owner-local untransformed glyph outlines. */
static FVM_Path *syntax_paths[DESK_SLOTS][4];  /* Detached colour groups2..5, sharing whole-field shaping. */
static DeskItem text_keys[DESK_SLOTS];         /* Immutable layout keys; position is deliberately excluded. */
static unsigned char *drag_pixels[DESK_SLOTS]; /* Owned bounded complete window snapshots. */
static DeskItem drag_keys[DESK_SLOTS];         /* Content/geometry keys for copied drag pixels. */
static unsigned int glass_width[2];            /* Cached immutable rail contour widths. */
static unsigned int glass_height[2];           /* Cached immutable rail contour heights. */
static int active;                             /* A desktop scene has been admitted. */


#define VECTOR_BYTES 22000U /* Complete bounded ASCII command stream, never SVG/script input. */
#define VECTOR_SHAPES 32U  /* Matching portable document path bound. */
#define VECTOR_NODES 512U  /* Complete document command bound. */


/* Detached scalar command. Managed/native object addresses never enter the stream. */
typedef struct VectorNode
{
    unsigned long verb; /* Move0, line1, quadratic2, close3. */
    unsigned long x;    /* Logical destination X,0..1000. */
    unsigned long y;    /* Logical destination Y,0..600. */
    unsigned long cx;   /* Quadratic control X, zero otherwise. */
    unsigned long cy;   /* Quadratic control Y, zero otherwise. */
} VectorNode;


/* Immutable path style and range into its pane-owned detached node array. */
typedef struct VectorShape
{
    unsigned long color; /* RGB24; common fill/stroke colour. */
    unsigned long width; /* Logical width0..32, zero disables stroke. */
    unsigned long fill;  /* Boolean fill admission. */
    unsigned int first; /* First node in the detached array. */
    unsigned int count; /* Path command count2..64. */
} VectorShape;


/* One staged vector snapshot. Only the FlashVM owner reads/writes it; raster
 * workers receive copied contours through the existing immutable frame pipeline. */
typedef struct VectorScene
{
    char bytes[VECTOR_BYTES];              /* Owned bounded hex assembly buffer. */
    size_t expected;                       /* Exact declared stream bytes, zero for text panes. */
    size_t used;                           /* Contiguous bytes received. */
    unsigned long tool;                   /* Copied selected authoring tool1..4. */
    unsigned long color;                  /* Copied current RGB24. */
    unsigned long width;                  /* Copied current logical width1..32. */
    unsigned long fill;                   /* Copied fill toggle0/1. */
    unsigned long undo;                   /* Copied Undo availability0..16. */
    unsigned long redo;                   /* Copied Redo availability0..16. */
    unsigned int count;                    /* Parsed complete path count. */
    unsigned int complete;                 /* Only one allows painting/End commit. */
    VectorShape shapes[VECTOR_SHAPES];     /* Copied style/range records. */
    VectorNode nodes[VECTOR_NODES];        /* Copied scalar commands. */
} VectorScene;

static FVM_Object *vector_buttons[DESK_SLOTS][16]; /* [GC:] Borrowed child MovieClips retained by each pane/thumbnail. */
static VectorScene vectors[9]; /* Stable pane identities1..8, reset at scene Begin. */


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
static void motion_local(const DeskMotion *motion, double *x, double *y, double *scale)
{
    double zoom;
    double pan;
    double scroll;
    motion_sample(motion, x, y, scale);
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
            double zoom = tablezoom / 100.0;
            double small = fmin((double)thumbnail->width / window->width, (double)thumbnail->height / window->height);
            double x = thumbnail->x + (thumbnail->width - window->width * small) / 2;
            double y = thumbnail->y + (thumbnail->height - window->height * small) / 2;
            int hiding = window->x == 65536;
            double from_x = x;
            double from_y = y;
            double from_scale = small;
            FVM_Affine world;
            if (hiding && clips[id] && world_matrix(clips[id], 1, &world))
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
            motion->to_x = hiding ? x : 84 + (window->x - 84) * zoom + tablepan;
            motion->to_y = hiding ? y : 48 + (window->y - 48) * zoom - offset;
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


/* Draw a60-percent frosted-glass imitation using genuine alpha MovieClip fills.
 * Borrow clip; wide/high are copied panel pixels and right chooses its inner
 * edge. Smoothstep fades the inner32 pixels; deterministic low-contrast grain
 * varies tint rather than accumulating opacity. No backdrop/VM pointer escapes.
 * Return zero on a drawing failure; detached workers later composite the paths. */
static int glass(FVM_Object *clip, double wide, double high, int right)
{
    FVM_Value args[3];
    FVM_Value ignored;
    int x;
    int y;
    if (glass_width[right] == (unsigned int)wide && glass_height[right] == (unsigned int)high)
    {
        return 1;
    }
    if (fvm_object_call(owner, clip, "clear", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    if (fvm_object_call(owner, clip, "lineStyle", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (x = 0; x < (int)wide; x += 4)
    {
        double edge = right ? x + 2 : wide - x - 2;
        double t = fmax(0, fmin(1, edge / 32));
        double alpha = 60 * t * t * (3 - 2 * t);
        for (y = 0; y < (int)high; y += 24)
        {
            unsigned long grain = ((unsigned int)x * 13U + (unsigned int)y * 7U) % 7U;
            unsigned long tint = ((47UL + grain) << 16) | ((62UL + grain) << 8) | (73UL + grain);
            double xs[5] = {x, fmin(x + 4, wide), fmin(x + 4, wide), x, x};
            double ys[5] = {y, y, fmin(y + 24, high), fmin(y + 24, high), y};
            unsigned int point;
            fvm_value_number(&args[0], tint);
            fvm_value_number(&args[1], alpha);
            if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
            {
                return 0;
            }
            for (point = 0; point < 5; ++point)
            {
                fvm_value_number(&args[0], xs[point]);
                fvm_value_number(&args[1], ys[point]);
                if (fvm_object_call(owner, clip, point ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
                {
                    return 0;
                }
            }
            if (fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) != FVM_OK)
            {
                return 0;
            }
        }
    }
    glass_width[right] = (unsigned int)wide;
    glass_height[right] = (unsigned int)high;
    return 1;
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


/* Parse a complete bounded private command stream. Every extent is checked
 * before dereference; no partial stream is paintable and no SVG/XML is parsed. */
static int vector_parse(VectorScene *scene)
{
    size_t at = 56;
    unsigned long shapes;
    unsigned int total = 0;
    unsigned int i;
    if (scene->used < 56 || !word(scene->bytes, &scene->tool) ||
        !word(scene->bytes + 8, &scene->color) || !word(scene->bytes + 16, &scene->width) ||
        !word(scene->bytes + 24, &scene->fill) || !word(scene->bytes + 32, &scene->undo) ||
        !word(scene->bytes + 40, &scene->redo) || !word(scene->bytes + 48, &shapes) ||
        scene->tool < 1 || scene->tool > 4 || scene->color > 0xffffffUL || scene->width < 1 ||
        scene->width > 32 || scene->fill > 1 || scene->undo > 16 || scene->redo > 16 || shapes > VECTOR_SHAPES)
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
            shape->fill > 1 || (!shape->width && !shape->fill) || count < 2 || count > 64 ||
            count > VECTOR_NODES - total)
        {
            return 0;
        }
        at += 32;
        if (count > (scene->used - at) / 40)
        {
            return 0;
        }
        shape->first = total;
        shape->count = (unsigned int)count;
        for (j = 0; j < count; ++j)
        {
            VectorNode *node = &scene->nodes[total++];
            if (!word(scene->bytes + at, &node->verb) || !word(scene->bytes + at + 8, &node->x) ||
                !word(scene->bytes + at + 16, &node->y) || !word(scene->bytes + at + 24, &node->cx) ||
                !word(scene->bytes + at + 32, &node->cy) || node->verb > 3 || node->x > 1000 ||
                node->y > 600 || node->cx > 1000 || node->cy > 600 || (j == 0) != (node->verb == 0) ||
                (node->verb != 2 && (node->cx || node->cy)) ||
                (node->verb == 3 && (j + 1 != count || node->x || node->y)))
            {
                return 0;
            }
            at += 40;
        }
    }
    if (at != scene->used)
    {
        return 0;
    }
    scene->count = (unsigned int)shapes;
    scene->complete = 1;
    return 1;
}


/* Uniform logical canvas scale; page stays inside the pane beneath its toolbar.
 * Nonpositive means the current very small pane cannot display a drawing. */
static double vector_scale(const DeskItem *item)
{
    if (item->width <= 12 || item->height <= 86)
    {
        return 0;
    }
    return fmin((item->width - 12) / 1040.0, (item->height - 86) / 640.0);
}


/* Draw one small copied line/curve icon through ordinary MovieClip methods.
 * cell selects presentation only; all control admission/semantics remain on Dis.
 * Caller owns the context and child clip; zero propagates a VM drawing error. */
static int vector_icon(FVM_Object *clip, unsigned int cell, double wide, unsigned long color)
{
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
    if (cell == 0)
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
        fvm_value_number(args, wide / 2 + x[i] * scale);
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


/* Render one copied decimal digit as seven small solid line segments. Value is
 *0..9; geometry/color are caller-owned values, no font or document is accessed. */
static int vector_digit(FVM_Object *clip, unsigned int value, double cx, double cy, double scale, unsigned long color)
{
    static const unsigned int masks[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};
    static const double coordinates[7][4] = {
        {-2, -4, 2, -4}, {2, -4, 2, 0}, {2, 0, 2, 4}, {-2, 4, 2, 4},
        {-2, 0, -2, 4}, {-2, -4, -2, 0}, {-2, 0, 2, 0}};
    unsigned int i;
    FVM_Value args[3];
    FVM_Value ignored;
    fvm_value_number(args, 1.2);
    fvm_value_number(args + 1, color);
    fvm_value_number(args + 2, 100);
    if (value > 9 || fvm_object_call(owner, clip, "lineStyle", 3, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (i = 0; i < 7; ++i)
    {
        if (!(masks[value] & (1U << i)))
        {
            continue;
        }
        fvm_value_number(args, cx + coordinates[i][0] * scale);
        fvm_value_number(args + 1, cy + coordinates[i][1] * scale);
        if (fvm_object_call(owner, clip, "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
        fvm_value_number(args, cx + coordinates[i][2] * scale);
        fvm_value_number(args + 1, cy + coordinates[i][3] * scale);
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
    double cell = content->width > 12 ? (content->width - 12) / 8.0 : 0;
    for (i = 0; i < 16; ++i)
    {
        FVM_Object *button = vector_buttons[slot][i];
        if (!scene || cell < 12)
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
        unsigned long border = 0x8799a8UL;
        unsigned long ink = 0x26343fUL;
        int selected = i < 4 && scene->tool == i + 1;
        if (i >= 8 && i < 14)
        {
            fill = palette[i - 8];
            selected = scene->color == fill;
        }
        if (i == 14)
        {
            selected = scene->fill != 0;
        }
        if (i == 15)
        {
            fill = scene->color;
            unsigned long luminance = ((fill >> 16) & 255) * 299 + ((fill >> 8) & 255) * 587 + (fill & 255) * 114;
            ink = luminance < 128000 ? 0xffffffUL : 0x222222UL;
        }
        if (selected)
        {
            border = 0x2684dbUL;
            if (i < 8 || i == 14)
            {
                fill = 0xb8d9f5UL;
            }
        }
        if ((i == 4 && !scene->undo) || (i == 5 && !scene->redo) ||
            (i == 6 && scene->width == 1) || (i == 7 && scene->width == 32))
        {
            ink = 0xa4afb7UL;
        }
        if (!number(button, "_visible", 1) || !number(button, "_x", 6 + (i % 8) * cell) ||
            !number(button, "_y", i < 8 ? 26 : 50) || !box(button, cell - 3, 20, fill, border) ||
            !vector_icon(button, i, cell - 3, ink))
        {
            return 0;
        }
        if (i == 15)
        {
            double center = (cell - 3) / 2;
            double scale = fmin(1.4, (cell - 7) / 12);
            if ((scene->width >= 10 && !vector_digit(button, (unsigned int)scene->width / 10, center - 4 * scale, 10, scale, ink)) ||
                !vector_digit(button, (unsigned int)scene->width % 10, center + (scene->width >= 10 ? 4 * scale : 0), 10, scale, ink))
            {
                return 0;
            }
        }
    }
    if (trace && scene)
    {
        printf("VECTOR_TOOLBAR %lu %lu %lu %lu %lu %lu %lu %lu\n", ticket, slot,
               scene->tool, scene->color, scene->width, scene->fill, scene->undo, scene->redo);
    }
    return 1;
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
    for (i = 0; i < scene->count; ++i)
    {
        const VectorShape *shape = &scene->shapes[i];
        unsigned int j;
        double start_x = 0;
        double start_y = 0;
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
            double x = 6 + (20 + node->x) * scale;
            double y = 80 + (20 + node->y) * scale;
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
            fvm_value_number(args, x);
            fvm_value_number(args + 1, y);
            if (node->verb == 2)
            {
                fvm_value_number(args, 6 + (20 + node->cx) * scale);
                fvm_value_number(args + 1, 80 + (20 + node->cy) * scale);
                fvm_value_number(args + 2, x);
                fvm_value_number(args + 3, y);
            }
            if (fvm_object_call(owner, clip, node->verb == 0 ? "moveTo" : (node->verb == 2 ? "curveTo" : "lineTo"),
                                node->verb == 2 ? 4 : 2, args, &ignored) != FVM_OK)
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
        if (!item || item->line || item->mirror || (item->parent != 1 && item->parent < 5))
        {
            return 0;
        }
        scene = &vectors[message->first];
        if (message->code == DVM_DESK_VECTOR_BEGIN)
        {
            unsigned long revision;
            unsigned long version;
            if (scene->expected || message->size != 16 || !word(message->text, &revision) ||
                !word(message->text + 8, &version) || version != DVM_DESK_VECTOR_VERSION ||
                revision > 0x7fffffffUL || message->second < 56 || message->second > VECTOR_BYTES)
            {
                return 0;
            }
            scene->expected = (size_t)message->second;
            return 1;
        }
        if (!scene->expected || scene->complete || message->second < 0 ||
            (size_t)message->second != scene->used || !message->size || message->size > 1024 ||
            message->size > scene->expected - scene->used)
        {
            return 0;
        }
        memcpy(scene->bytes + scene->used, message->text, message->size);
        scene->used += message->size;
        return scene->used == scene->expected ? vector_parse(scene) : 1;
    }
    if (message->code == DVM_DESK_END)
    {
        for (index = 1; index <= 8; ++index)
        {
            if (vectors[index].expected && !vectors[index].complete)
            {
                return 0;
            }
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
                if (items[root_index].id == root_id && items[root_index].parent == 1 && !items[root_index].mirror)
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
        memset(items + received, 0, (DESK_SLOTS - received) * sizeof(*items));
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
    if (words[0] > 12 || (words[0] >= 5 && ((unsigned long)message->first > 8 || words[7])) || (words[1] > 65536 && words[1] < 0xffff8000UL) || words[2] > 65536 || !words[3] ||
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


/* Compose actual local/parent matrices into caller-owned world output. parent
 * is a validated slot0..12; MDI parents recurse once through the table. Return
 * zero on failed observations. Owner-thread only; no managed reference escapes. */
static int world_matrix(FVM_Object *clip, unsigned long parent, FVM_Affine *output)
{
    FVM_Affine local;
    FVM_Affine group;
    if (fvm_movieclip_get_matrix(owner, clip, &local) != FVM_OK ||
        (parent >= 5 ? !world_matrix(groups[parent], 1, &group) :
         fvm_movieclip_get_matrix(owner, groups[parent], &group) != FVM_OK))
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
        x >= width - 104 || y < 48 || y >= height)
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
            double cell = item->width > 12 ? (item->width - 12) / 8.0 : 0;
            if (cell >= 12 && localx >= 6 && localx < item->width - 6 &&
                ((localy >= 26 && localy < 46) || (localy >= 50 && localy < 70)))
            {
                int column = (int)((localx - 6) / cell);
                int index = column + (localy >= 50 ? 8 : 0);
                if (index < 15)
                {
                    *widget = item->id;
                    *position = -2 - index;
                    return 1;
                }
                return 0;
            }
            double scale = vector_scale(item);
            double vx = scale > 0 ? (localx - 6) / scale - 20 : -1;
            double vy = scale > 0 ? (localy - 80) / scale - 20 : -1;
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
            ? !glass(clip, content->width, content->height, item->id == 12)
            : !box(clip,
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
    GLOBAL_NUMBER("ideRightX", width - 104);
    double widget_x = item->x;
    double widget_y = item->y;
    if (item->id <= 8 && motions[item->id].active)
    {
        motion_local(&motions[item->id], &widget_x, &widget_y, &scale);
    }
    GLOBAL_NUMBER("ideWidgetX", widget_x);
    GLOBAL_NUMBER("ideWidgetY", widget_y);
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
        if (item->id <= 8 && item->parent == 1 && item->x == 65536 && item->y == 65536 && !motions[item->id].active)
        {
            return 1;
        }
        result = draw_widget_clip(item, surface);
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
    if ((item->id <= 8 && vectors[item->id].complete) || tablezoom != 100 || tablepan != 0 || camera_motion.active || item->id > 8 || motions[item->id].active ||
        item->parent != 1 || !pixels ||
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
    if ((item->id <= 8 && vectors[item->id].complete) || tablezoom != 100 || tablepan != 0 || camera_motion.active || item->id > 8 || item->parent != 1 || top < 48 ||
        item->x < 0 || motions[item->id].active || item->x + item->width > surface->width ||
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
        if (item->id >= 30 && item->id <= 37 && motions[item->id - 29].active)
        {
            continue;
        }
        int translucent = item->id <= 8 && (motions[item->id].active || item->style == 5 || item->style == 6);
        FVM_Affine world;
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        if (translucent)
        {
            double zoom;
            double pan;
            double scroll;
            double x;
            double y;
            double scale = 1;
            camera_view(&zoom, &pan, &scroll);
            x = item->x * zoom + 84 * (1 - zoom) + pan;
            y = item->y * zoom + 48 * (1 - zoom) - scroll;
            if (motions[item->id].active)
            {
                motion_sample(&motions[item->id], &x, &y, &scale);
                zoom = scale;
            }
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
