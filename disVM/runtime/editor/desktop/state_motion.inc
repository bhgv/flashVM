/* Copyright (c) 2026 bhgv
 * EN: Scene fields, interpolation and captured shapes; included by the owner translation unit.
 * RU: Поля сцены, интерполяция и захваченные контуры; включается в translation unit владельца.
 */
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
