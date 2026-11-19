/* Copyright (c) 2026 bhgv
 * EN: CLI options, font resources and canvas geometry; included by the owner translation unit.
 * RU: Опции CLI, шрифты и геометрия canvas; включается в translation unit владельца.
 */
/* Store a known CLI option without copying argv; returns whether it was recognized. */
int dvm_editor_option(const char *name, const char *value)
{
    if (!strcmp(name, "--line-numbers"))
    {
        if (strcmp(value, "0") && strcmp(value, "1"))
        {
            return 0;
        }
        initial_line_numbers = value[0] == '1';
    }
    else if (!strcmp(name, "--font"))
    {
        font_path = value;
    }
    else if (!strcmp(name, "--document"))
    {
        document_path = value;
        new_document = 0;
    }
    else if (!strcmp(name, "--new"))
    {
        document_path = value;
        new_document = 1;
    }
    else if (!strcmp(name, "--events"))
    {
        event_path = value;
    }
    else if (!strcmp(name, "--window-events"))
    {
        event_path = value;
        window_replay = 1;
    }
    else if (!strcmp(name, "--clipboard-events"))
    {
        event_path = value;
        window_replay = 1;
        system_clipboard_events = 1;
    }
    else if (!strcmp(name, "--capture"))
    {
        capture_path = value;
    }
    else
    {
        return 0;
    }
    return 1;
}

/* Expose immutable configuration to the Dis builtin; no I/O or heap pointer escapes. */
const char *dvm_editor_path(void)
{
    return document_path;
}

/* Return copied new-document configuration; no file or VM operation occurs. */
int dvm_editor_fresh(void)
{
    return new_document;
}

/* Choose the bundled font relative to the entry module, preserving the caller's cwd.
 * directory is borrowed. An explicit --font remains an optional override, never a prompt. */
int dvm_editor_default_font(const char *directory)
{
    const char *relative = "/fonts/NotoSansMono-Regular.ttf";
    size_t length;
    if (font_path)
    {
        return 1;
    }
    if (!directory || strlen(directory) > 4000)
    {
        return 0;
    }
    length = strlen(directory) + strlen(relative) + 1;
    default_font_path = malloc(length);
    if (!default_font_path)
    {
        return 0;
    }
    snprintf(default_font_path, length, "%s%s", directory, relative);
    font_path = default_font_path;
    return 1;
}

/* Reserve enough native pixels for a readable prompt at the admitted face size.
 * line_height is an actual finite font metric; returns bounded surface pixels, never text policy. */
static int panel_height(double line_height)
{
    int height = line_height > 300 ? 320 : (int)ceil(line_height) + 12;
    if (height < 48)
    {
        height = 48;
    }
    return height;
}

/* Measure copied multiline panel data for the AS view's bounded native surface.
 * line_height is a trusted font metric. Return pixels; no selection/action policy lives here. */
static int panel_extent(double line_height)
{
    int height = panel_height(line_height);
    if (panel_mode == EDITOR_PANEL_BROWSER)
    {
        size_t index;
        int lines = 1;
        for (index = 0; index < panel_size; index++)
        {
            if (panel_text[index] == '\n')
            {
                lines++;
            }
        }
        height = (int)ceil(lines * line_height) + 12;
        if (height > (int)canvas_height - 48)
        {
            height = (int)canvas_height - 48;
        }
    }
    return height;
}

/* Return the TextField baseline distance for the explicit editor TextFormat.
 * metrics is a borrowed current/proposed font record; no VM or view state changes. */
static double line_step(const FVM_FontMetrics *metrics)
{
    return metrics->ascent + metrics->descent + EDITOR_LEADING;
}

/* Measure bounded visible logical rows from the actual current face metrics.
 * No arguments; returns 1..128, with a conservative single row on an unavailable metric. */
int dvm_editor_rows(void)
{
    FVM_FontMetrics metrics;
    int rows;
    if (!layout_font || fvm_font_metrics(layout_font, &metrics) != FVM_OK || !(line_step(&metrics) > 0))
    {
        return 1;
    }
    rows = (int)floor((canvas_height - panel_height(line_step(&metrics)) - 4) / line_step(&metrics));
    if (rows < 1)
    {
        rows = 1;
    }
    if (rows > 128)
    {
        rows = 128;
    }
    return rows;
}

/* Measure a proposed face size without changing the current view or either VM state.
 * size is bounded 6..48; height is admitted 160..2048 pixels. Returns row capacity or zero on failure. */
static int rows_for_canvas(int size, unsigned int height)
{
    FVM_Font *font = 0;
    FVM_FontMetrics metrics;
    int rows;
    if (size == font_size && height == canvas_height)
    {
        return dvm_editor_rows();
    }
    if (size < EDITOR_FONT_MIN || size > EDITOR_FONT_MAX ||
        fvm_font_read(font_bytes, font_byte_count, 0, size, &font) != FVM_OK)
    {
        return 0;
    }
    if (fvm_font_metrics(font, &metrics) != FVM_OK || !(line_step(&metrics) > 0))
    {
        fvm_font_destroy(font);
        return 0;
    }
    rows = (int)floor((height - panel_height(line_step(&metrics)) - 4) / line_step(&metrics));
    fvm_font_destroy(font);
    if (rows < 1)
    {
        rows = 1;
    }
    if (rows > 128)
    {
        rows = 128;
    }
    return rows;
}

/* Admit a bounded Dis-selected surface with fresh storage before retiring the old one.
 * width/height are copied pixels. Returns zero on allocation/field/SDL failure; no document access.
 * A field failure is terminal to this view; successful allocation ownership changes only here. */
static int apply_canvas(unsigned int width, unsigned int height)
{
    unsigned char *candidate;
    FVM_Value value;
    FVM_Result result;
#ifdef DVM_EDITOR_SDL
    SDL_Texture *replacement = 0;
#endif
    if (width < EDITOR_WIDTH_MIN || height < EDITOR_HEIGHT_MIN || width > EDITOR_EXTENT_MAX ||
        height > EDITOR_EXTENT_MAX)
    {
        return 0;
    }
    if (width == canvas_width && height == canvas_height)
    {
        return 1;
    }
    candidate = malloc((size_t)width * height * 4U);
    if (!candidate)
    {
        return 0;
    }
#ifdef DVM_EDITOR_SDL
    if (renderer)
    {
        replacement = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!replacement)
        {
            free(candidate);
            return 0;
        }
    }
#endif
    fvm_value_number(&value, width);
    result = fvm_object_set(context, field, "_width", &value);
    if (result == FVM_OK)
    {
        fvm_value_number(&value, height);
        result = fvm_object_set(context, field, "_height", &value);
    }
    if (result == FVM_OK)
    {
        fvm_value_number(&value, width - 16);
        result = fvm_object_set(context, panel_field, "_width", &value);
    }
    if (result != FVM_OK)
    {
#ifdef DVM_EDITOR_SDL
        if (replacement)
        {
            SDL_DestroyTexture(replacement);
        }
#endif
        free(candidate);
        return 0;
    }
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
    texture = replacement;
    if (window)
    {
        int current_width;
        int current_height;
        SDL_GetWindowSize(window, &current_width, &current_height);
        if (current_width != (int)width || current_height != (int)height)
        {
            SDL_SetWindowSize(window, width, height);
        }
    }
#endif
    free(pixels);
    pixels = candidate;
    pixels_valid = 0;
    canvas_width = width;
    canvas_height = height;
    return 1;
}

/* Apply a Dis-admitted size to both real TextFields and the glyph-hit face.
 * size is bounded; returns one on actual matching font/layout setup, zero on failure. */
static int apply_font_size(int size)
{
    FVM_Object *format = 0;
    FVM_Font *font = 0;
    FVM_Value value;
    FVM_Value argument;
    FVM_Value ignored;
    FVM_Result result;
    if (size == font_size)
    {
        return 1;
    }
    if (size < EDITOR_FONT_MIN || size > EDITOR_FONT_MAX ||
        fvm_font_read(font_bytes, font_byte_count, 0, size, &font) != FVM_OK ||
        fvm_construct(context, "TextFormat", 0, 0, &format) != FVM_OK || fvm_gc_pin(context, format) != FVM_OK)
    {
        fvm_font_destroy(font);
        return 0;
    }
    fvm_value_number(&value, size);
    result = fvm_object_set(context, format, "size", &value);
    if (result == FVM_OK)
    {
        fvm_value_number(&value, EDITOR_LEADING);
        result = fvm_object_set(context, format, "leading", &value);
    }
    if (result == FVM_OK)
    {
        fvm_value_boolean(&value, 0);
        result = fvm_object_set(context, format, "kerning", &value);
    }
    fvm_value_undefined(&argument);
    argument.type = FVM_VALUE_OBJECT;
    argument.object = format;
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, field, "setNewTextFormat", 1, &argument, &ignored);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, panel_field, "setNewTextFormat", 1, &argument, &ignored);
    }
    /* A loaded SWF retains current text runs independently of future insertion format.
     * Update both domains so the glyph raster and admitted caret metrics scale together. */
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, field, "setTextFormat", 1, &argument, &ignored);
    }
    if (result == FVM_OK)
    {
        result = fvm_object_call(context, panel_field, "setTextFormat", 1, &argument, &ignored);
    }
    fvm_gc_unpin(context, format);
    if (result != FVM_OK)
    {
        fvm_font_destroy(font);
        return 0;
    }
    fvm_font_destroy(layout_font);
    layout_font = font;
    font_size = size;
    return 1;
}
