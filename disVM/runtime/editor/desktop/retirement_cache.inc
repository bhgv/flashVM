/* Copyright (c) 2026 bhgv
 * EN: Widget retirement, drag images and retained keys; included by the owner translation unit.
 * RU: Удаление виджетов, drag-изображения и ключи кэша; включается в translation unit владельца.
 */
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
