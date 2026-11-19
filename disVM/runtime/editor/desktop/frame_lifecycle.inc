/* Copyright (c) 2026 bhgv
 * EN: Frame composition, hit queries and owner teardown; included by the owner translation unit.
 * RU: Композиция кадра, hit-запросы и завершение владельца; включается в translation unit владельца.
 */
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

#include "../../desktop_vector_local.c"
