/* Copyright (c) 2026 bhgv
 * EN: Panel/gutter painting and immutable frame capture; included by the owner translation unit.
 * RU: Рисование панелей/gutter и захват неизменяемого кадра; включается в translation unit владельца.
 */
/* Draw a channel-published prompt/status field over the document surface.
 * surface is borrowed; mode/text/selection are copied Dis values. Returns one only on real rendering. */
static int render_panel(FVM_RasterSurface *surface)
{
    static const char *labels[] = {"File: ",
                                   "Find [Enter/Esc]: ",
                                   "Open [Enter/Esc]: ",
                                   "Save As [Enter/Esc]: ",
                                   "Replace needle: ",
                                   "Replacement: ",
                                   "Go to line[:column]: ",
                                   "Inspect [Esc]: ",
                                   "Unsaved [S save / D discard / Esc cancel]: ",
                                   "",
                                   "Directory path [Enter/Esc]: ",
                                   "Export NEW file [Enter/Esc]: "};
    char text[1100];
    size_t prefix = strlen(labels[panel_mode]);
    FVM_Value value;
    FVM_Path *path = 0;
    FVM_Affine placement;
    FVM_TextFieldScrollInfo scrolling;
    FVM_RasterColor background = {232, 237, 247, 255};
    FVM_RasterColor active = {197, 217, 252, 255};
    FVM_RasterColor ink = {24, 40, 72, 255};
    FVM_Result result;
    FVM_FontMetrics metrics;
    int height;
    double cursor_x = 0;
    size_t first = 0;
    size_t last = 0;
    size_t index;
    if (fvm_font_metrics(layout_font, &metrics) != FVM_OK)
    {
        return 0;
    }
    height = panel_extent(line_step(&metrics));
    if (!rectangle(surface, 0, canvas_height - height, canvas_width, height, background))
    {
        return 0;
    }
    memcpy(text, labels[panel_mode], prefix);
    memcpy(text + prefix, panel_text, panel_size);
    if (fvm_value_string_bytes(&value, text, prefix + panel_size) != FVM_OK ||
        fvm_set_global(context, "idePanelText", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = panel_field;
    if (fvm_set_global(context, "idePanel", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, canvas_width);
    if (fvm_set_global(context, "ideWidth", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, canvas_height);
    if (fvm_set_global(context, "ideHeight", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, height);
    if (fvm_set_global(context, "idePanelHeight", &value) != FVM_OK || fvm_swf_execute_actions(context) != FVM_OK)
    {
        return 0;
    }
    scrolling.struct_size = sizeof(scrolling);
    if (fvm_textfield_scroll_info(context, panel_field, &scrolling) != FVM_OK)
    {
        return 0;
    }
    if (panel_mode)
    {
        FVM_TextLayoutOptions options;
        FVM_TextLayout *layout = 0;
        memset(&options, 0, sizeof(options));
        options.alignment = FVM_TEXT_ALIGN_LEFT;
        options.line_height = line_step(&metrics);
        if (fvm_text_layout_create_configured(
                layout_font, text, prefix + panel_size, &options, FVM_FONT_SHAPE_NO_KERNING, &layout) != FVM_OK)
        {
            return 0;
        }
        fvm_text_layout_destroy(panel_layout);
        panel_layout = layout;
        panel_prefix = prefix;
        first = prefix + text_byte(panel_text, panel_size, panel_caret < panel_anchor ? panel_caret : panel_anchor);
        last = prefix + text_byte(panel_text, panel_size, panel_caret > panel_anchor ? panel_caret : panel_anchor);
        if (panel_mode == EDITOR_PANEL_BROWSER)
        {
            panel_horizontal = 0;
        }
        else if (!field_position(panel_layout, prefix + text_byte(panel_text, panel_size, panel_caret), &cursor_x))
        {
            return 0;
        }
        if (cursor_x - panel_horizontal < 0)
        {
            panel_horizontal = floor(cursor_x);
        }
        if (cursor_x - panel_horizontal > canvas_width - 24)
        {
            panel_horizontal = ceil(cursor_x - (canvas_width - 24));
        }
    }
    else
    {
        fvm_text_layout_destroy(panel_layout);
        panel_layout = 0;
        panel_horizontal = 0;
    }
    fvm_value_number(&value, panel_horizontal);
    if (fvm_object_set(context, panel_field, "hscroll", &value) != FVM_OK ||
        fvm_textfield_scroll_info(context, panel_field, &scrolling) != FVM_OK)
    {
        return 0;
    }
    panel_horizontal = scrolling.hscroll;
    memset(&placement, 0, sizeof(placement));
    placement.xx = 1;
    placement.yy = 1;
    if (fvm_object_get(context, panel_field, "_x", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER)
    {
        return 0;
    }
    placement.x0 = value.number;
    if (fvm_object_get(context, panel_field, "_y", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER)
    {
        return 0;
    }
    placement.y0 = value.number;
    if (panel_layout)
    {
        FVM_TextLayoutMetrics layout_metrics;
        if (fvm_text_layout_metrics(panel_layout, &layout_metrics) != FVM_OK)
        {
            return 0;
        }
        for (index = 0; index < layout_metrics.glyph_count; index++)
        {
            FVM_TextLayoutGlyph glyph;
            if (fvm_text_layout_glyph(panel_layout, index, &glyph) != FVM_OK)
            {
                return 0;
            }
            if (glyph.shaped.cluster >= first && glyph.shaped.cluster < last &&
                !rectangle(surface,
                           placement.x0 + 2 + glyph.x - panel_horizontal,
                           placement.y0 + 2 + glyph.y - metrics.ascent,
                           glyph.shaped.x_advance,
                           line_step(&metrics),
                           active))
            {
                return 0;
            }
        }
    }
    result = fvm_textfield_path(context, panel_field, &path);
    if (result == FVM_OK)
    {
        result = fvm_path_transform(path, &placement);
    }
    if (result == FVM_OK)
    {
        result = draw_visible(path, surface, ink);
    }
    fvm_path_destroy(path);
    if (result == FVM_OK && panel_mode &&
        (panel_mode < 7 || panel_mode == EDITOR_PANEL_BROWSE_PATH || panel_mode == EDITOR_PANEL_EXPORT_PATH))
    {
        FVM_RasterColor cursor = {35, 85, 170, 255};
        if (!rectangle(surface,
                       placement.x0 + 2 + cursor_x - panel_horizontal,
                       placement.y0 + 2,
                       1,
                       line_step(&metrics),
                       cursor))
        {
            return 0;
        }
    }
    return result == FVM_OK;
}

/* Reserve a measured strip and update only the native source field's available width.
 * Uses copied viewport/font/visibility data; returns zero on font/field errors, never edits source. */
static int measure_gutter(void)
{
    unsigned long highest = view_top + (unsigned long)dvm_editor_rows() + 1;
    unsigned int digits = 1;
    unsigned int digit;
    double widest = 0;
    FVM_Value value;
    gutter_width = 0;
    if (line_numbers)
    {
        while (highest >= 10)
        {
            highest /= 10;
            digits++;
        }
        for (digit = 0; digit < 10; digit++)
        {
            FVM_Path *path = 0;
            double advance = 0;
            double vertical = 0;
            if (fvm_font_glyph(layout_font, fvm_font_char(layout_font, '0' + digit), &path, &advance, &vertical) !=
                FVM_OK)
            {
                return 0;
            }
            fvm_path_destroy(path);
            if (advance > widest)
            {
                widest = advance;
            }
        }
        gutter_width = (unsigned int)ceil(widest * digits + 12);
        if (gutter_width > canvas_width / 2)
        {
            gutter_width = canvas_width / 2;
        }
    }
    fvm_value_number(&value, canvas_width - gutter_width);
    return fvm_object_set(context, field, "_width", &value) == FVM_OK;
}

/* Paint logical numbers at the actual source layout baselines, covering scrolled source ink.
 * surface is borrowed, layout contains only the acknowledged viewport. No source parsing/mutation. */
static int render_gutter(FVM_RasterSurface *surface)
{
    FVM_RasterColor background = {242, 244, 248, 255};
    FVM_RasterColor ink = {105, 113, 128, 255};
    FVM_FontMetrics font;
    FVM_TextLayoutMetrics metrics;
    unsigned int index;
    double bottom;
    if (!gutter_width)
    {
        return 1;
    }
    if (fvm_font_metrics(layout_font, &font) != FVM_OK || fvm_text_layout_metrics(hit_layout, &metrics) != FVM_OK)
    {
        return 0;
    }
    bottom = canvas_height - panel_height(line_step(&font));
    if (!rectangle(surface, 0, 0, gutter_width, bottom, background))
    {
        return 0;
    }
    for (index = 0; index < metrics.line_count; index++)
    {
        FVM_TextLayoutLine line;
        FVM_TextRun *run = 0;
        FVM_Path *path = 0;
        FVM_Affine placement;
        FVM_Result result;
        double advance = 0;
        double vertical = 0;
        char label[32];
        if (fvm_text_layout_line(hit_layout, index, &line) != FVM_OK)
        {
            return 0;
        }
        if (line.baseline - font.ascent + 2 >= bottom)
        {
            break;
        }
        snprintf(label, sizeof(label), "%lu", view_top + index + 1);
        result = fvm_font_shape_configured(
            layout_font, label, strlen(label), FVM_TEXT_LTR, 0, FVM_FONT_SHAPE_NO_KERNING, &run);
        if (result == FVM_OK)
        {
            result = fvm_text_run_path(layout_font, run, &path, &advance, &vertical);
        }
        memset(&placement, 0, sizeof(placement));
        placement.xx = 1;
        placement.yy = 1;
        placement.x0 = gutter_width - 6 - advance;
        placement.y0 = line.baseline + 2;
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, &placement);
        }
        if (result == FVM_OK)
        {
            result = draw_visible(path, surface, ink);
        }
        fvm_path_destroy(path);
        fvm_text_run_destroy(run);
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}

/* Prepare a complete owner-only document view as detached native paint commands.
 * recording_frame is borrowed and receives owned paths. Return one on success or
 * zero on layout/geometry/allocation failure; no pixels, files or SDL are touched. */
static int prepare_frame(void)
{
    FVM_Value value;
    FVM_Path *path = 0;
    FVM_RasterSurface surface;
    FVM_RasterColor ink = {32, 36, 48, 255};
    FVM_Result result;
    size_t shown;
    display_start = document_byte(view_first);
    shown = document_byte(view_last) - display_start;
    if (shown > EDITOR_VIEW_BYTES)
    {
        return 0;
    }
    display_size = shown;
    if (fvm_value_string_bytes(&value, snapshot + display_start, shown) != FVM_OK ||
        fvm_object_set(context, field, "text", &value) != FVM_OK ||
        fvm_object_get(context, field, "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
        fvm_value_string_size(&value) != shown || memcmp(value.string, snapshot + display_start, shown))
    {
        return 0;
    }
    surface.pixels = NULL; /* Geometry only: worker owns the detached destination. */
    surface.width = canvas_width;
    surface.height = canvas_height;
    surface.stride = canvas_width * 4U;
    if (!measure_gutter() || !selection_geometry(&surface, shown))
    {
        return 0;
    }
    result = fvm_textfield_path(context, field, &path);
    if (result == FVM_OK && gutter_width)
    {
        FVM_Affine shift;
        memset(&shift, 0, sizeof(shift));
        shift.xx = shift.yy = 1;
        shift.x0 = gutter_width;
        result = fvm_path_transform(path, &shift);
    }
    if (result == FVM_OK)
    {
        result = draw_visible(path, &surface, ink);
    }
    fvm_path_destroy(path);
    if (result != FVM_OK)
    {
        fprintf(stderr, "editor TextField raster failed: %d\n", result);
        return 0;
    }
    if (!render_gutter(&surface) || !render_panel(&surface))
    {
        return 0;
    }
    return 1;
}

/* Present only completed native pixels on the UI owner; SDL and capture stay here.
 * No VM state enters the raster worker. Returns zero on SDL/file failure. */
static int present_pixels(void)
{
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        char title[256];
        int changed = upload_frame == NULL; /* Expose forces presentation; identical snapshots do not. */
        snprintf(title,
                 sizeof(title),
                 "Dis IDE | rev %lu saved %ld status %lu | top %lu | Ctrl+S save, Ctrl+O open, Ctrl+G location%s",
                 revision,
                 saved,
                 status,
                 view_top + 1,
                 view_first != 0 || view_last != (long)view_total ? " | bounded window" : "");
        SDL_SetWindowTitle(window, title);
        if (upload_frame)
        {
            unsigned int index;
            unsigned int tiles = ((canvas_width + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE) *
                                 ((canvas_height + DVM_EDITOR_TILE_SIZE - 1) / DVM_EDITOR_TILE_SIZE);
            for (index = 0; index < tiles; ++index)
            {
                unsigned int x;
                unsigned int y;
                unsigned int width;
                unsigned int height;
                if (dvm_editor_frame_damage(upload_frame, index, &x, &y, &width, &height))
                {
                    changed = 1;
                    SDL_Rect rectangle = {(int)x, (int)y, (int)width, (int)height};
                    if (SDL_UpdateTexture(
                            texture, &rectangle, pixels + ((size_t)y * canvas_width + x) * 4, canvas_width * 4U))
                    {
                        return 0;
                    }
                }
            }
        }
        else if (SDL_UpdateTexture(texture, 0, pixels, canvas_width * 4U))
        {
            return 0;
        }
        if (changed)
        {
            if (SDL_RenderClear(renderer) || SDL_RenderCopy(renderer, texture, 0, 0))
            {
                return 0;
            }
            SDL_RenderPresent(renderer);
        }
    }
#endif
    if (capture_path)
    {
        FILE *capture = fopen(capture_path, "wb");
        size_t index;
        int failed = 0;
        if (!capture)
        {
            return 0;
        }
        if (fprintf(capture, "P6\n%u %u\n255\n", canvas_width, canvas_height) < 0)
        {
            failed = 1;
        }
        for (index = 0; index < canvas_width * canvas_height && !failed; index++)
        {
            if (fwrite(pixels + index * 4, 1, 3, capture) != 3)
            {
                failed = 1;
            }
        }
        if (fclose(capture))
        {
            failed = 1;
        }
        if (failed)
        {
            return 0;
        }
    }
    return 1;
}
