/* Copyright (c) 2026 bhgv
 * EN: Text layout, hit testing and selection geometry; included by the owner translation unit.
 * RU: Раскладка текста, hit testing и геометрия выделения; включается в translation unit владельца.
 */
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
