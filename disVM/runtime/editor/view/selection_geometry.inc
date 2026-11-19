/* Copyright (c) 2026 bhgv
 * EN: Clipboard delivery and selection/glyph geometry; included by the owner translation unit.
 * RU: Доставка clipboard и геометрия выделения/глифов; включается в translation unit владельца.
 */
/* Copy at most one acknowledged result or Unicode-aligned read fragment into message.
 * message is caller-owned. Returns one for a pending result; zero leaves ordinary input available.
 * Bytes are copied before native storage is freed; a later document edit is solely Dis policy. */
static int clipboard_event(DVM_UIMessage *message)
{
    char identity[9];
    size_t count;
    if (!clipboard_ack && !clipboard_transfer)
    {
        return 0;
    }
    message->kind = DVM_UI_REQUEST;
    message->id = 1;
    message->flags = clipboard_revision;
    snprintf(identity, sizeof(identity), "%08lx", clipboard_ticket);
    memcpy(message->text, identity, EDITOR_CLIP_PREFIX);
    message->size = EDITOR_CLIP_PREFIX;
    if (clipboard_ack)
    {
        message->code = EDITOR_CLIP_ACK;
        message->first = clipboard_status;
        clipboard_ack = 0;
        return 1;
    }
    count = clipboard_total - clipboard_offset;
    if (count > DVM_UI_TEXT_MAX - EDITOR_CLIP_PREFIX)
    {
        count = DVM_UI_TEXT_MAX - EDITOR_CLIP_PREFIX;
        while (count && ((unsigned char)clipboard_transfer[clipboard_offset + count] & 0xc0U) == 0x80U)
        {
            count--;
        }
    }
    message->code = EDITOR_CLIP_DATA;
    message->first = (long)clipboard_offset;
    message->second = (long)clipboard_total;
    memcpy(message->text + EDITOR_CLIP_PREFIX, clipboard_transfer + clipboard_offset, count);
    message->size += count;
    clipboard_offset += count;
    if (clipboard_offset == clipboard_total)
    {
        free(clipboard_transfer);
        clipboard_transfer = 0;
    }
    return 1;
}

/* Paint acknowledged caret/selection using the same face/size and FlashVM layout.
 * surface is borrowed; shown bounds the preview. Returns zero on layout/raster failure.
 * Combining/bidi clusters highlight as glyph clusters; no native editing policy is inferred. */
static int selection_geometry(FVM_RasterSurface *surface, size_t shown)
{
    FVM_TextLayoutOptions options;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_TextFieldScrollInfo scrolling;
    FVM_Value value;
    FVM_RasterColor selected = {204, 222, 255, 255};
    FVM_RasterColor cursor = {35, 85, 170, 255};
    size_t first = scalar_byte(shown, caret < anchor ? caret : anchor);
    size_t last = scalar_byte(shown, caret > anchor ? caret : anchor);
    size_t current = scalar_byte(shown, caret);
    size_t index;
    double cursor_x = 0;
    double cursor_y = 0;
    int cursor_visible = 0;
    int ok = 1;
    if ((caret < anchor ? caret : anchor) < view_first)
    {
        first = 0;
    }
    if ((caret > anchor ? caret : anchor) > view_last)
    {
        last = shown;
    }
    memset(&options, 0, sizeof(options));
    options.alignment = FVM_TEXT_ALIGN_LEFT;
    if (fvm_font_metrics(layout_font, &font) != FVM_OK)
    {
        return 0;
    }
    options.line_height = line_step(&font);
    {
        FVM_Result result;
        if (memchr(snapshot + display_start, '\t', shown))
        {
            double average;
            result = fvm_font_average_width(layout_font, &average);
            if (result == FVM_OK)
            { /* Match AS2 TextField's implicit tab interval: four average character widths. */
                result = fvm_text_layout_create_periodic(layout_font,
                                                         snapshot + display_start,
                                                         shown,
                                                         &options,
                                                         FVM_FONT_SHAPE_NO_KERNING,
                                                         0,
                                                         0,
                                                         average * 4,
                                                         &layout);
            }
        }
        else
        {
            result = fvm_text_layout_create_configured(
                layout_font, snapshot + display_start, shown, &options, FVM_FONT_SHAPE_NO_KERNING, &layout);
        }
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    if (fvm_text_layout_metrics(layout, &metrics) != FVM_OK)
    {
        fvm_text_layout_destroy(layout);
        return 0;
    }
    for (index = 0; index < metrics.line_count && current <= shown && ok; index++)
    {
        FVM_TextLayoutLine line;
        size_t glyph_index;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (current < line.byte_start || (current >= line.byte_next && index + 1 < metrics.line_count))
        {
            continue;
        }
        cursor_x = line.x + line.advance;
        cursor_y = line.baseline - font.ascent;
        for (glyph_index = 0; glyph_index < line.glyph_count; glyph_index++)
        {
            FVM_TextLayoutGlyph glyph;
            if (fvm_text_layout_glyph(layout, line.glyph_start + glyph_index, &glyph) != FVM_OK)
            {
                ok = 0;
                break;
            }
            if (glyph.shaped.cluster >= current)
            {
                cursor_x = glyph.x;
                break;
            }
        }
        cursor_visible = ok;
        break;
    }
    if (cursor_visible)
    {
        if (cursor_x - horizontal < 0)
        {
            horizontal = floor(cursor_x);
        }
        if (cursor_x - horizontal > canvas_width - gutter_width - 12)
        {
            horizontal = ceil(cursor_x - (canvas_width - gutter_width - 12));
        }
    }
    if (horizontal < 0)
    {
        horizontal = 0;
    }
    fvm_value_number(&value, horizontal);
    scrolling.struct_size = sizeof(scrolling);
    if (fvm_object_set(context, field, "hscroll", &value) != FVM_OK ||
        fvm_textfield_scroll_info(context, field, &scrolling) != FVM_OK)
    {
        ok = 0;
    }
    if (ok)
    {
        horizontal = (double)scrolling.hscroll;
    }
    for (index = 0; index < metrics.glyph_count && ok; index++)
    {
        FVM_TextLayoutGlyph glyph;
        if (fvm_text_layout_glyph(layout, index, &glyph) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (glyph.shaped.cluster >= first && glyph.shaped.cluster < last)
        {
            ok = rectangle(surface,
                           glyph.x + 2 + gutter_width - horizontal,
                           glyph.y - font.ascent + 2,
                           glyph.shaped.x_advance,
                           line_step(&font),
                           selected);
        }
    }
    if (ok && cursor_visible && !panel_mode)
    {
        ok = rectangle(surface, cursor_x + 2 + gutter_width - horizontal, cursor_y + 2, 1, line_step(&font), cursor);
    }
    if (ok)
    {
        fvm_text_layout_destroy(hit_layout);
        hit_layout = layout;
    }
    else
    {
        fvm_text_layout_destroy(layout);
    }
    return ok;
}

/* Cull entire offscreen font contours before the bounded public raster API.
 * path/surface are borrowed, color is copied. Retained curves/control points stay exact;
 * this is conservative contour culling, never point clamping or glyph distortion. */
static FVM_Result draw_visible(const FVM_Path *path, FVM_RasterSurface *surface, FVM_RasterColor color)
{
    FVM_Path *visible = 0;
    FVM_Result result = fvm_path_create(&visible);
    size_t count = fvm_path_count(path);
    size_t first = 0;
    while (first < count && result == FVM_OK)
    {
        size_t last = first;
        double left = 1e100;
        double right = -1e100;
        double top = 1e100;
        double bottom = -1e100;
        while (last < count)
        {
            FVM_PathCommand command;
            unsigned int points;
            unsigned int index;
            result = fvm_path_get(path, last, &command);
            if (result != FVM_OK || (last != first && command.verb == FVM_PATH_MOVE))
            {
                break;
            }
            points = command.verb == FVM_PATH_CLOSE       ? 0U
                     : command.verb == FVM_PATH_CUBIC     ? 3U
                     : command.verb == FVM_PATH_QUADRATIC ? 2U
                                                          : 1U;
            for (index = 0; index < points; index++)
            {
                if (command.x[index] < left)
                {
                    left = command.x[index];
                }
                if (command.x[index] > right)
                {
                    right = command.x[index];
                }
                if (command.y[index] < top)
                {
                    top = command.y[index];
                }
                if (command.y[index] > bottom)
                {
                    bottom = command.y[index];
                }
            }
            last++;
        }
        if (last == first || result != FVM_OK)
        {
            result = FVM_INVALID_ARGUMENT;
            break;
        }
        if (right >= 0 && bottom >= 0 && left <= surface->width && top <= surface->height)
        {
            size_t index;
            for (index = first; index < last && result == FVM_OK; index++)
            {
                FVM_PathCommand command;
                result = fvm_path_get(path, index, &command);
                if (result == FVM_OK)
                {
                    result = fvm_path_add(visible, &command);
                }
            }
        }
        first = last;
    }
    if (result == FVM_OK)
    {
        if (dvm_editor_frame_path(recording_frame, visible, color))
        {
            visible = NULL;
        }
        else
        {
            result = FVM_OUT_OF_MEMORY;
        }
    }
    fvm_path_destroy(visible);
    return result;
}

/* Resolve an admitted byte insertion coordinate in one owned single-line field layout.
 * layout is borrowed; out receives native pixels. Returns zero on invalid geometry, without policy changes. */
static int field_position(const FVM_TextLayout *layout, size_t byte, double *out)
{
    FVM_TextLayoutLine line;
    size_t index;
    if (fvm_text_layout_line(layout, 0, &line) != FVM_OK)
    {
        return 0;
    }
    *out = line.x + line.advance;
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        if (fvm_text_layout_glyph(layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return 0;
        }
        if (glyph.shaped.cluster >= byte)
        {
            *out = glyph.x;
            break;
        }
    }
    return 1;
}
