/* Copyright (c) 2026 bhgv
 * EN: Vector capture, scene decoding and world transforms; included by the owner translation unit.
 * RU: Захват векторов, декодирование сцены и преобразования; включается в translation unit владельца.
 */
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
