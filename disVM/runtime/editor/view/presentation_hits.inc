/* Copyright (c) 2026 bhgv
 * EN: Presentation messages and text hit positions; included by the owner translation unit.
 * RU: Сообщения представления и позиции попадания в текст; включается в translation unit владельца.
 */
/* Assemble ordered Dis snapshot fragments, publishing TextField only at selection boundary.
 * message is borrowed, never retained. Returns zero on malformed/stale/inconsistent framing. */
int dvm_editor_present(const DVM_UIMessage *message)
{
    unsigned long offset;
    unsigned long total;
    if (message->kind != DVM_UI_VIEW)
    {
        return 0;
    }
    if ((message->code >= DVM_DESK_BEGIN && message->code <= DVM_DESK_END) || message->code == DVM_DESK_COLORS ||
        message->code == DVM_DESK_VECTOR_BEGIN || message->code == DVM_DESK_VECTOR_DATA)
    {
        if (!desktop_bound)
        {
            if (!dvm_desktop_bind(context, font_bytes, font_byte_count, events != NULL))
            {
                return 0;
            }
            desktop_bound = 1;
        }
        if (message->code == DVM_DESK_BEGIN &&
            !apply_canvas((unsigned int)message->first, (unsigned int)message->second))
        {
            return 0;
        }
        int desktop_status = dvm_desktop_message(message);
        if (!desktop_status)
        {
            fprintf(stderr,
                    "desktop wire code=%lu first=%ld size=%lu\n",
                    message->code,
                    message->first,
                    (unsigned long)message->size);
            return 0;
        }
        if (message->code == DVM_DESK_BEGIN)
        {
            complete = 0; /* Staged geometry is not an input baseline, even during native capture. */
        }
        if (desktop_status == 3)
        {
            return 1; /* End accepted; separate geometry brokers have not completed. */
        }
        if (message->code == DVM_DESK_END || desktop_status == 2 || desktop_status == 4)
        {
            if (message->code == DVM_DESK_END || desktop_status == 4)
            {
                complete = 1;
            }
#ifdef DVM_EDITOR_SDL
            if (!events && !capture_path)
            {
                return request_paint();
            }
#endif
            return render_desktop();
        }
        return 1;
    }
    if (message->code == EDITOR_CLIP_CONTINUE)
    {
        unsigned long ticket;
        if (complete || assembling || !clipboard_transfer || message->size != EDITOR_CLIP_PREFIX ||
            message->flags != clipboard_revision || message->first != (long)clipboard_offset ||
            message->second != (long)clipboard_total || !hexword(message->text, &ticket) || ticket != clipboard_ticket)
        {
            return 0;
        }
        complete = 1;
        return 1;
    }
    if (message->code == EDITOR_CLIP_WRITE || message->code == EDITOR_CLIP_FIELD_WRITE ||
        message->code == EDITOR_CLIP_READ)
    {
        return clipboard_command(message);
    }
    if (message->code == EDITOR_CANVAS)
    {
        if (assembling || message->first < EDITOR_WIDTH_MIN || message->second < EDITOR_HEIGHT_MIN ||
            message->first > EDITOR_EXTENT_MAX || message->second > EDITOR_EXTENT_MAX || message->size ||
            !apply_canvas((unsigned int)message->first, (unsigned int)message->second))
        {
            return 0;
        }
        canvas_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_GUTTER)
    {
        if (assembling || message->flags != style_revision || message->size || message->second || message->first < 0 ||
            message->first > 1)
        {
            return 0;
        }
        line_numbers = message->first != 0;
        gutter_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_STYLE)
    {
        if (assembling || message->flags != canvas_revision || message->size || message->first < EDITOR_FONT_MIN ||
            message->first > EDITOR_FONT_MAX || message->second < 1 || message->second > 128 ||
            !apply_font_size((int)message->first) || message->second != dvm_editor_rows())
        {
            return 0;
        }
        style_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_PANEL)
    {
        if (assembling || message->first < 0 || message->first > EDITOR_PANEL_EXPORT_PATH || message->second < 0 ||
            message->second > 1 || message->size > sizeof(panel_text))
        {
            return 0;
        }
        panel_mode = (unsigned long)message->first;
        panel_selected = (int)message->second;
        panel_revision = message->flags;
        panel_size = message->size;
        memcpy(panel_text, message->text, panel_size);
        return 1;
    }
    if (message->code == EDITOR_PANEL_SELECTION)
    {
        size_t scalars;
        if (assembling || message->size || message->flags != panel_revision || message->first < 0 ||
            message->second < 0 || fvm_utf8_validate(panel_text, panel_size, &scalars) != FVM_UTF8_OK ||
            (unsigned long)message->first > scalars || (unsigned long)message->second > scalars ||
            (!panel_mode && (message->first || message->second)))
        {
            return 0;
        }
        panel_caret = message->first;
        panel_anchor = message->second;
        panel_selection_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_VIEWPORT)
    {
        unsigned long top;
        unsigned long scalars;
        unsigned long ticket;
        if (assembling || message->size != 24 || message->first < 0 || message->second < message->first ||
            !hexword(message->text, &top) || !hexword(message->text + 8, &scalars) ||
            !hexword(message->text + 16, &ticket) || scalars > 1048576UL || (unsigned long)message->second > scalars ||
            ticket == 0 || ticket > 2147483647UL || ticket <= view_ticket)
        {
            return 0;
        }
        view_first = message->first;
        view_last = message->second;
        view_total = scalars;
        view_top = top;
        view_ticket = ticket;
        view_revision = message->flags;
        return 1;
    }
    if (message->code == EDITOR_SELECTION)
    {
        if (!assembling || assembled != extent || message->flags != revision || message->first < 0 ||
            message->second < 0)
        {
            return 0;
        }
        size_t scalars;
        snapshot[extent] = 0;
        if (fvm_utf8_validate(snapshot, extent, &scalars) != FVM_UTF8_OK || (unsigned long)message->first > scalars ||
            (unsigned long)message->second > scalars || scalars != view_total || revision != view_revision ||
            revision != canvas_revision || revision != style_revision || revision != panel_revision ||
            revision != panel_selection_revision || revision != gutter_revision || !view_ticket)
        {
            return 0;
        }
        caret = message->first;
        anchor = message->second;
#ifdef DVM_EDITOR_SDL
        if (!events && !capture_path)
        {
            if (!request_paint())
            {
                return 0;
            }
        }
        else
#endif
            if (!render())
        {
            return 0;
        }
        complete = 1;
        assembling = 0;
        if (events)
        {
            size_t index;
            printf("VIEW %lu %lu %ld %ld %ld ", revision, status, saved, message->first, message->second);
            for (index = 0; index < extent; index++)
            {
                printf("%02x", (unsigned char)snapshot[index]);
            }
            putchar('\n');
            printf("WINDOW %lu %ld %ld %lu\n", view_ticket, view_first, view_last, view_top);
            printf("GUTTER %d %u %lu\n", line_numbers, gutter_width, view_top + 1);
            printf("STYLE %d %d\n", font_size, dvm_editor_rows());
            printf("CANVAS %u %u\n", canvas_width, canvas_height);
            printf("FIELD %ld %ld %.0f\n", panel_caret, panel_anchor, panel_horizontal);
            printf("PANEL %lu %d ", panel_mode, panel_selected);
            for (index = 0; index < panel_size; index++)
            {
                printf("%02x", (unsigned char)panel_text[index]);
            }
            putchar('\n');
        }
        return 1;
    }
    if (message->size < EDITOR_REPLY_META || !hexword(message->text + 40, &offset) ||
        !hexword(message->text + 48, &total) || total > EDITOR_SOURCE_MAX || offset > total ||
        message->size - EDITOR_REPLY_META > total - offset)
    {
        return 0;
    }
    if (!assembling)
    {
        if (offset != 0)
        {
            return 0;
        }
        revision = message->flags;
        status = message->code;
        saved = message->second;
        extent = total;
        assembled = 0;
        assembling = 1;
    }
    if (message->flags != revision || message->code != status || message->second != saved || total != extent ||
        offset != assembled)
    {
        return 0;
    }
    memcpy(snapshot + offset, message->text + EDITOR_REPLY_META, message->size - EDITOR_REPLY_META);
    assembled += message->size - EDITOR_REPLY_META;
    return 1;
}

/* Convert one actual glyph-layout hit into a source scalar proposal.
 * Coordinates are borrowed pixel values. Returns -1 if no completed layout is available. */
static long hit_position(double x, double y)
{
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_TextLayoutLine line;
    size_t index;
    size_t selected = 0;
    size_t byte;
    long position = view_first;
    double distance;
    if (!hit_layout || fvm_text_layout_metrics(hit_layout, &metrics) != FVM_OK || !metrics.line_count ||
        fvm_font_metrics(layout_font, &font) != FVM_OK)
    {
        return -1;
    }
    if (x < 0 || y < 0 || y >= canvas_height - panel_height(line_step(&font)))
    {
        return -1;
    }
    y -= 2;
    x += horizontal - 2 - gutter_width;
    for (index = 0; index < metrics.line_count; index++)
    {
        if (fvm_text_layout_line(hit_layout, index, &line) != FVM_OK)
        {
            return -1;
        }
        selected = index;
        if (y < line.baseline - font.ascent + line_step(&font))
        {
            break;
        }
    }
    if (fvm_text_layout_line(hit_layout, selected, &line) != FVM_OK)
    {
        return -1;
    }
    byte = line.byte_end;
    distance = fabs(x - (line.x + line.advance));
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        double candidate;
        if (fvm_text_layout_glyph(hit_layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return -1;
        }
        candidate = fabs(x - glyph.x);
        if (candidate < distance)
        {
            distance = candidate;
            byte = glyph.shaped.cluster;
        }
    }
    if (byte > display_size)
    {
        return -1;
    }
    for (index = 0; index < byte; index++)
    {
        if (((unsigned char)snapshot[display_start + index] & 0xc0U) != 0x80U)
        {
            position++;
        }
    }
    return position;
}

/* Propose a scalar in the completed single-line field using its actual label/scroll layout.
 * Coordinates are native pixels. Returns -1 outside the footer or before a valid completed layout. */
static long field_hit(double x, double y)
{
    FVM_FontMetrics font;
    FVM_TextLayoutLine line;
    FVM_TextLayoutMetrics layout_metrics;
    size_t byte;
    size_t index;
    long scalar = 0;
    double distance;
    if (!panel_mode || !panel_layout || fvm_font_metrics(layout_font, &font) != FVM_OK ||
        y < canvas_height - panel_extent(line_step(&font)) || y >= canvas_height ||
        fvm_text_layout_line(panel_layout, 0, &line) != FVM_OK)
    {
        return -1;
    }
    if (panel_mode == EDITOR_PANEL_BROWSER)
    {
        double local_y = y - (canvas_height - panel_extent(line_step(&font)) + 8);
        unsigned int row;
        if (local_y < 0 || fvm_text_layout_metrics(panel_layout, &layout_metrics) != FVM_OK)
        {
            return -1;
        }
        row = (unsigned int)floor(local_y / line_step(&font));
        if (row >= layout_metrics.line_count || fvm_text_layout_line(panel_layout, row, &line) != FVM_OK)
        {
            return -1;
        }
    }
    x += panel_horizontal - 10;
    byte = line.byte_end;
    distance = fabs(x - (line.x + line.advance));
    for (index = 0; index < line.glyph_count; index++)
    {
        FVM_TextLayoutGlyph glyph;
        double candidate;
        if (fvm_text_layout_glyph(panel_layout, line.glyph_start + index, &glyph) != FVM_OK)
        {
            return -1;
        }
        candidate = fabs(x - glyph.x);
        if (candidate < distance)
        {
            distance = candidate;
            byte = glyph.shaped.cluster;
        }
    }
    if (byte <= panel_prefix)
    {
        return 0;
    }
    byte -= panel_prefix;
    if (byte > panel_size)
    {
        return -1;
    }
    for (index = 0; index < byte; index++)
    {
        if (((unsigned char)panel_text[index] & 0xc0U) != 0x80U)
        {
            scalar++;
        }
    }
    return scalar;
}
