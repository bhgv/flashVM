/* Copyright (c) 2026 bhgv
 * EN: Vector icons, toolbars and live ink painting; included by the owner translation unit.
 * RU: Векторные значки, инструменты и рисование ink; включается в translation unit владельца.
 */
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
