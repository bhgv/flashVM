/* Copyright (c) 2026 bhgv
 * EN: Gutter painting and retained syntax colors; included by the owner translation unit.
 * RU: Рисование gutter и сохранённые синтаксические цвета; включается в translation unit владельца.
 */
/* Draw logical source numbers beside the copied viewport. content/world are
 * borrowed owner values; no document is read. Wrapped continuations are blank,
 * and clipping suppresses labels crossing the window header or lower edge. */
static int gutter(const DeskItem *content, const FVM_Affine *world, FVM_RasterSurface *surface)
{
    FVM_TextLayout *layout = NULL;
    FVM_TextLayoutOptions options;
    FVM_TextLayoutMetrics metrics;
    FVM_FontMetrics font;
    FVM_RasterColor ink = {100, 111, 126, 255};
    size_t index;
    size_t previous = 0;
    unsigned long number_value = content->line;
    unsigned long last_number = 0;
    int ok = 1;
    if (!number_value)
    {
        return 1;
    }
    if (!text_layout(content, &layout, &font, &options, &metrics))
    {
        return 0;
    }
    for (index = 0; index < metrics.line_count && ok; ++index)
    {
        FVM_TextLayoutLine line;
        FVM_TextRun *run = NULL;
        FVM_Path *path = NULL;
        FVM_Affine placement = *world;
        FVM_Result result;
        double advance = 0;
        double vertical = 0;
        char label[32];
        size_t byte;
        if (fvm_text_layout_line(layout, index, &line) != FVM_OK)
        {
            ok = 0;
            break;
        }
        if (!index)
        {
            previous = line.byte_end;
            continue; /* The first copied line is the caption, never source. */
        }
        for (byte = previous; byte < line.byte_start; ++byte)
        {
            if (content->text[byte] == '\n' && index > 1)
            {
                ++number_value;
            }
        }
        previous = line.byte_start;
        if (number_value == last_number)
        {
            continue;
        }
        last_number = number_value;
        placement.x0 += world->xx * 8;
        placement.y0 += world->yy * (line.baseline + 8);
        if (placement.y0 - font.ascent < 48 || placement.y0 > surface->height ||
            placement.y0 > world->y0 + world->yy * content->height - 4)
        {
            continue;
        }
        snprintf(label, sizeof(label), "%lu", number_value);
        result =
            fvm_font_shape_configured(face, label, strlen(label), FVM_TEXT_LTR, 0, FVM_FONT_SHAPE_NO_KERNING, &run);
        if (result == FVM_OK)
        {
            result = fvm_text_run_path(face, run, &path, &advance, &vertical);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, &placement);
        }
        if (result == FVM_OK)
        {
            result = prepared_frame ? capture_path(NULL, path, ink, 0, 0) : fvm_path_draw(path, surface, ink, 0, 0);
        }
        fvm_path_destroy(path);
        fvm_text_run_destroy(run);
        ok = result == FVM_OK;
    }
    fvm_text_layout_destroy(layout);
    return ok;
}

/* Atomically replace detached colour contours using the same owner-held TextField.
 * content is copied scalar data; label/id belong to this VM owner. No worker sees
 * a VM/font handle. Failure retains the old cache and destroys every candidate. */
static int syntax_cache(FVM_Object *label, const DeskItem *content, unsigned long id)
{
    FVM_Path *paths[5] = {NULL, NULL, NULL, NULL, NULL};
    FVM_TextPathRange ranges[DESK_TEXT];
    size_t boundaries[DESK_TEXT + 1];
    size_t scalars = 0;
    size_t byte;
    unsigned int group;
    for (byte = 0; byte < content->size; ++byte)
    {
        if (((unsigned char)content->text[byte] & 0xc0U) != 0x80U)
        {
            boundaries[scalars++] = byte;
        }
    }
    boundaries[scalars] = content->size;
    for (group = 0; group < 5; ++group)
    {
        size_t count = 0;
        size_t scalar = 0;
        while (scalar < scalars)
        {
            unsigned int kind = content->colors[scalar];
            unsigned int selected = kind < 2 ? 0 : kind - 1;
            if (selected != group)
            {
                ++scalar;
                continue;
            }
            ranges[count].first = boundaries[scalar++];
            while (scalar < scalars)
            {
                kind = content->colors[scalar];
                selected = kind < 2 ? 0 : kind - 1;
                if (selected != group)
                {
                    break;
                }
                ++scalar;
            }
            ranges[count++].last = boundaries[scalar];
        }
        if (fvm_textfield_path_ranges(owner, label, ranges, count, &paths[group]) != FVM_OK)
        {
            unsigned int discard;
            for (discard = 0; discard < 5; ++discard)
            {
                fvm_path_destroy(paths[discard]);
            }
            return 0;
        }
    }
    fvm_path_destroy(text_paths[id]);
    text_paths[id] = paths[0];
    for (group = 0; group < 4; ++group)
    {
        fvm_path_destroy(syntax_paths[id][group]);
        syntax_paths[id][group] = paths[group + 1];
    }
    text_keys[id] = *content;
    return 1;
}
