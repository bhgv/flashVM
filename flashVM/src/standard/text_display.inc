/* Copyright (c) 2026 bhgv
 * EN: Share the field's shaped layout between scalar hits, caret and selection preview.
 * RU: Общая раскладка поля для попаданий указателя, каретки и подсветки выделения.
 */

namespace
{
/* EN: Return the left edge of a laid-out advance, including the line's shaping and alignment.
 * RU: Вернуть левую границу шага глифа с учётом раскладки и выравнивания строки. */
static double glyph_left(const FVM_TextLayoutGlyph &glyph)
{
    return glyph.x - glyph.shaped.x_offset;
}


/* EN: Locate the visual leading edge of a glyph within its bidirectional run.
 * RU: Найти начальную визуальную границу глифа с учётом направления текста. */
static double glyph_start(const FVM_TextLayoutGlyph &glyph)
{
    return glyph_left(glyph) + ((glyph.bidi_level & 1) ? glyph.shaped.x_advance : 0);
}


/* EN: Append a plain selection rectangle to a detached path.
 * RU: Добавить прямоугольник выделения к отдельному контуру. */
static FVM_Result preview_box(FVM_Path *path, double x, double y, double width, double height)
{
    if (width <= 0 || height <= 0)
    {
        return FVM_OK;
    }
    const double points[][2] = {{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}};
    FVM_Result result = FVM_OK;
    for (size_t i = 0; i <= 4 && result == FVM_OK; ++i)
    {
        FVM_PathCommand command = {FVM_PATH_CLOSE, {0, 0, 0}, {0, 0, 0}};
        if (i < 4)
        {
            command.verb = i ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = points[i][0];
            command.y[0] = points[i][1];
        }
        result = fvm_path_add(path, &command);
    }
    return result;
}


/* EN: Add selected glyph advances and the current insertion box for the visible line.
 * RU: Добавить шаги выделенных глифов и прямоугольник каретки на видимой строке. */
static FVM_Result preview_line(const Field *field, const FVM_TextFieldScrollInfo &view, size_t i, size_t first,
    size_t last, size_t current, FVM_Path *selection, FVM_Path *caret)
{
    FVM_TextLayoutLine line;
    FVM_Result result = fvm_text_layout_line(field->layout, i, &line);
    if (result != FVM_OK)
    {
        return result;
    }
    const double x = view.clip_x + view.offset_x + field_line_shift(field, line);
    const double y = view.clip_y + view.offset_y + i * view.line_height;
    double cursor = line.x + (line.direction == FVM_TEXT_RTL ? 0 : line.advance);
    size_t cursor_byte = line.byte_end;
    for (size_t g = 0; result == FVM_OK && g < line.glyph_count; ++g)
    {
        FVM_TextLayoutGlyph glyph;
        result = fvm_text_layout_glyph(field->layout, line.glyph_start + g, &glyph);
        if (result != FVM_OK)
        {
            return result;
        }
        const size_t cluster = glyph.shaped.cluster;
        if (cluster >= current && cluster < cursor_byte)
        {
            cursor = glyph_start(glyph);
            cursor_byte = cluster;
        }
        if (first <= cluster && cluster < last)
        {
            result = preview_box(selection, x + glyph_left(glyph), y, glyph.shaped.x_advance, view.line_height);
        }
    }
    if (result == FVM_OK && current >= line.byte_start &&
        (current < line.byte_next || i + 1 == view.line_count))
    {
        result = preview_box(caret, x + cursor, y, 1, view.line_height);
    }
    return result;
}
} // namespace


/* EN/RU: See text_display.hpp. / См. text_display.hpp. */
FVM_Result fvm_textfield_scalar_at(FVM_Context *c, FVM_Object *object, double x, double y, long &index)
{
    Field *field = (Field *)object->resource;
    index = -1;
    if (!(x >= 0 && x < field->width && y >= 0 && y < field->height))
    {
        return FVM_OK;
    }
    FVM_TextFieldScrollInfo view;
    FVM_Result result = scroll_geometry(c, field, view);
    if (result != FVM_OK)
    {
        return result;
    }
    const double row = (y - view.clip_y - view.offset_y) / view.line_height;
    const size_t i = row <= 0 ? 0 : row >= view.line_count ? view.line_count - 1 : (size_t)row;
    FVM_TextLayoutLine line;
    result = fvm_text_layout_line(field->layout, i, &line);
    if (result != FVM_OK)
    {
        return result;
    }
    x -= view.clip_x + view.offset_x + field_line_shift(field, line);
    size_t byte = line.byte_end;
    double distance = std::fabs(x - line.x - (line.direction == FVM_TEXT_RTL ? 0 : line.advance));
    for (size_t g = 0; g < line.glyph_count; ++g)
    {
        FVM_TextLayoutGlyph glyph;
        result = fvm_text_layout_glyph(field->layout, line.glyph_start + g, &glyph);
        if (result != FVM_OK)
        {
            return result;
        }
        const double candidate = std::fabs(x - glyph_start(glyph));
        if (candidate < distance)
        {
            distance = candidate;
            byte = glyph.shaped.cluster;
        }
    }
    size_t count;
    if (fvm_utf8_validate(field->text, byte, &count) != FVM_UTF8_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    index = (long)count;
    return FVM_OK;
}


/* EN/RU: See text_display.hpp. / См. text_display.hpp. */
FVM_Result fvm_textfield_preview_paths(FVM_Context *c, FVM_Object *object, FVM_Path **selection,
    FVM_Path **caret, double &selection_rgb, double &caret_rgb)
{
    Field *field = (Field *)object->resource;
    *selection = *caret = 0;
    if (!field->preview)
    {
        return FVM_OK;
    }
    size_t anchor, current;
    if (fvm_utf8_offset(field->text, field->text_size, field->preview_anchor, &anchor) != FVM_UTF8_OK ||
        fvm_utf8_offset(field->text, field->text_size, field->preview_caret, &current) != FVM_UTF8_OK)
    {
        return FVM_OK; /* The application replaced the text since publishing the preview. */
    }
    FVM_TextFieldScrollInfo view;
    FVM_Result result = scroll_geometry(c, field, view);
    if (result == FVM_OK)
    {
        result = fvm_path_create(selection);
    }
    if (result == FVM_OK)
    {
        result = fvm_path_create(caret);
    }
    const size_t first = anchor < current ? anchor : current;
    const size_t last = anchor < current ? current : anchor;
    if (result == FVM_OK)
    {
        const double visible_rows = std::ceil(view.clip_height / view.line_height);
        const size_t start = view.scroll - 1;
        const size_t end = visible_rows < view.line_count - start ? start + (size_t)visible_rows : view.line_count;
        for (size_t i = start; i < end && result == FVM_OK; ++i)
        {
            result = preview_line(field, view, i, first, last, current, *selection, *caret);
        }
    }
    selection_rgb = field->preview_fill;
    caret_rgb = field->preview_ink;
    return result;
}
