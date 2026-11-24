/* Copyright (c) 2026 bhgv
 * EN: Paint owner-local TextField contours at a MovieClip depth, clipped to the transformed viewport.
 * RU: Отрисовка контуров TextField на глубине MovieClip с отсечением по преобразованной области.
 */

/* EN: Write the five commands of one closed viewport rectangle into the borrowed path.
 * RU: Записать пять команд замкнутой области просмотра в заимствованный путь. */
FVM_Result text_viewport_path(FVM_Path *path, const FVM_TextFieldScrollInfo &viewport)
{
    const double x = viewport.clip_x, y = viewport.clip_y;
    const double right = x + viewport.clip_width, bottom = y + viewport.clip_height;
    const double points[4][2] = {{x, y}, {right, y}, {right, bottom}, {x, bottom}};
    FVM_Result result = FVM_OK;
    for (unsigned int index = 0; index < 5 && result == FVM_OK; ++index)
    {
        FVM_PathCommand command = {FVM_PATH_CLOSE, {0, 0, 0}, {0, 0, 0}};
        if (index < 4)
        {
            command.verb = index ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = points[index][0];
            command.y[0] = points[index][1];
        }
        result = fvm_path_add(path, &command);
    }
    return result;
}


/* EN: Paint one copied glyph path into the visible viewport tile. No script can run during compositing.
 * RU: Отрисовать контуры в видимой части области текста. Композиция не вызывает скрипт. */
FVM_Result paint_text_tile(FVM_Context *c, const FVM_Path *glyphs, const FVM_Affine &world,
                           const FVM_TextFieldScrollInfo &viewport, FVM_RasterColor ink,
                           FVM_RasterSurface *surface)
{
    if (!surface || !surface->pixels || !surface->width || !surface->height || surface->width > 4096 ||
        surface->height > 4096 || surface->stride < (size_t)surface->width * 4 ||
        surface->stride > (size_t)INT_MAX / surface->height)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Path *clip = 0;
    FVM_Result result = fvm_path_create(&clip);
    OwnedPath clip_owner(clip);
    FVM_PathBounds bounds;
    if (result == FVM_OK)
    {
        result = text_viewport_path(clip, viewport);
    }
    if (result == FVM_OK)
    {
        result = fvm_path_transform(clip, &world);
    }
    if (result == FVM_OK)
    {
        result = fvm_path_bounds(clip, &bounds);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    const double left = bounds.x_min < 0 ? 0 : std::floor(bounds.x_min);
    const double top = bounds.y_min < 0 ? 0 : std::floor(bounds.y_min);
    const double right = bounds.x_max > surface->width ? surface->width : std::ceil(bounds.x_max);
    const double bottom = bounds.y_max > surface->height ? surface->height : std::ceil(bounds.y_max);
    if (right <= left || bottom <= top)
    {
        return FVM_OK;
    }
    FVM_RasterSurface tile = {surface->pixels + (size_t)top * surface->stride + (size_t)left * 4,
        (unsigned int)(right - left), (unsigned int)(bottom - top), surface->stride};
    FVM_Affine projection = world;
    projection.x0 -= left;
    projection.y0 -= top;
    FVM_Path *placed = 0;
    result = fvm_path_create(&placed);
    OwnedPath placed_owner(placed);
    if (result == FVM_OK)
    {
        result = fvm_path_append(placed, glyphs, &projection);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    const bool antialias = c->stage_quality != FVM_STAGE_QUALITY_LOW;
    const bool pixel_aligned = !world.xy && !world.yx &&
        bounds.x_min == std::floor(bounds.x_min) && bounds.x_max == std::floor(bounds.x_max) &&
        bounds.y_min == std::floor(bounds.y_min) && bounds.y_max == std::floor(bounds.y_max);
    if (pixel_aligned)
    {
        return fvm_path_draw_quality(placed, &tile, ink, 0, 0, antialias);
    }
    /* The tile is the screen intersection. Allocate coverage only for its bounded area.
     * Временный слой занимает только часть области, пересекающую экран. */
    const size_t bytes = (size_t)tile.width * tile.height * 4;
    const unsigned long memory_limit = c->runtime->config.memory_limit_bytes;
    if (memory_limit && bytes > memory_limit / 2)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_detail::vector<unsigned char> pixels(bytes);
    FVM_RasterSurface layer = {pixels.begin(), tile.width, tile.height, (size_t)tile.width * 4};
    const FVM_Affine offset = {1, 0, 0, 1, -left, -top};
    result = fvm_path_transform(clip, &offset);
    if (result == FVM_OK)
    {
        result = fvm_path_draw_quality(placed, &layer, ink, 0, 0, antialias);
    }
    return result == FVM_OK ? fvm_path_mask_quality(clip, &tile, &layer, 0, antialias) : result;
}


/* [GC:] EN: Export one rooted field into temporary paths and paint through its parent's transform.
 * RU: Скопировать контуры удерживаемого TextField и отрисовать с преобразованием родителя. */
FVM_Result render_text(FVM_Context *c, FVM_Object *field, const FVM_Affine &parent_world,
                       const ColorAffine &transform, FVM_RasterSurface *surface, FVM_RasterSurface *coverage)
{
    double x, y, rgb;
    FVM_Result result = numeric(c, field, "_x", x);
    if (result == FVM_OK)
    {
        result = numeric(c, field, "_y", y);
    }
    if (result == FVM_OK)
    {
        result = numeric(c, field, "textColor", rgb);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_TextFieldScrollInfo viewport;
    viewport.struct_size = sizeof(viewport);
    result = fvm_textfield_scroll_info(c, field, &viewport);
    if (result != FVM_OK || !viewport.clip_width || !viewport.clip_height)
    {
        return result;
    }
    const FVM_Affine offset = {1, 0, 0, 1, x, y};
    const FVM_Affine world = multiply(parent_world, offset);
    FVM_Path *selection = 0, *caret = 0;
    double selection_rgb = 0, caret_rgb = 0;
    result = fvm_textfield_preview_paths(c, field, &selection, &caret, selection_rgb, caret_rgb);
    OwnedPath selection_owner(selection), caret_owner(caret);
    if (result == FVM_OK && selection && fvm_path_count(selection))
    {
        result = paint_text_tile(c, selection, world, viewport, color(selection_rgb, 100, transform), surface);
    }
    FVM_Path *glyphs = 0;
    if (result == FVM_OK)
    {
        result = fvm_textfield_path(c, field, &glyphs);
    }
    OwnedPath owner(glyphs);
    if (result == FVM_OK && fvm_path_count(glyphs))
    {
        result = paint_text_tile(c, glyphs, world, viewport, color(rgb, 100, transform), surface);
        if (result == FVM_OK && coverage)
        {
            const FVM_RasterColor opaque = {255, 255, 255, 255};
            result = paint_text_tile(c, glyphs, world, viewport, opaque, coverage);
        }
    }
    if (result == FVM_OK && caret && fvm_path_count(caret))
    {
        result = paint_text_tile(c, caret, world, viewport, color(caret_rgb, 100, transform), surface);
    }
    return result;
}
