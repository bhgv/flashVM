/* Copyright (c) 2026 bhgv
 * EN: MovieClip widget painting on owner snapshots; included by the owner translation unit.
 * RU: Рисование MovieClip по снимкам владельца; включается в translation unit владельца.
 */
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
