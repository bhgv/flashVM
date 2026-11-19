/* Copyright (c) 2026 bhgv
 * EN: FlashVM/SDL creation and clipboard serialization; included by the owner translation unit.
 * RU: Создание FlashVM/SDL и сериализация clipboard; включается в translation unit владельца.
 */
/* Load an explicitly selected bounded font and create a pinned uniform TextField.
 * Returns one on success, zero on any actual backend/SDK/file failure. */
int dvm_editor_create(void)
{
    FILE *font;
    long size;
    void *bytes;
    FVM_Config config;
    FVM_Value value;
    FVM_Result result;
    dvm_editor_idle_reset();
    if (!font_path)
    {
        fprintf(stderr, "editor bundled font resource is unavailable\n");
        return 0;
    }
    font = fopen(font_path, "rb");
    if (!font)
    {
        perror("editor font");
        return 0;
    }
    if (fseek(font, 0, SEEK_END) || (size = ftell(font)) <= 0 || size > 16777216L || fseek(font, 0, SEEK_SET))
    {
        fclose(font);
        return 0;
    }
    bytes = malloc((size_t)size);
    if (!bytes || fread(bytes, 1, (size_t)size, font) != (size_t)size)
    {
        free(bytes);
        fclose(font);
        return 0;
    }
    fclose(font);
    font_bytes = bytes;
    font_byte_count = (size_t)size;
    result = fvm_config_init(&config);
    if (result == FVM_OK)
    {
        result = fvm_runtime_create(&config, &runtime);
    }
    if (result == FVM_OK)
    {
        result = fvm_context_create(runtime, &context);
    }
    if (result == FVM_OK)
    {
        result = fvm_load_memory(context, ide_view_swf, sizeof(ide_view_swf));
        if (result == FVM_OK)
        {
            result = fvm_async_install(context);
        }
    }
    if (result == FVM_OK)
    {
        result = fvm_standard_text_install(context);
    }
    if (result == FVM_OK)
    {
        result = fvm_standard_text_register_font(context, "Editor", bytes, (size_t)size, 0);
    }
    if (result == FVM_OK)
    {
        result = fvm_font_read(bytes, (size_t)size, 0, font_size, &layout_font);
    }
    if (result != FVM_OK || fvm_textfield_create(context, "Editor", canvas_width, canvas_height, &field) != FVM_OK ||
        fvm_gc_pin(context, field) != FVM_OK ||
        fvm_textfield_create(context, "Editor", canvas_width - 16, 44, &panel_field) != FVM_OK ||
        fvm_gc_pin(context, panel_field) != FVM_OK)
    {
        fprintf(stderr, "editor FlashVM/font initialization failed\n");
        return 0;
    }
    {
        FVM_Object *format = 0;
        FVM_Value argument;
        FVM_Value ignored;
        if (fvm_construct(context, "TextFormat", 0, 0, &format) != FVM_OK || fvm_gc_pin(context, format) != FVM_OK)
        {
            return 0;
        }
        fvm_value_number(&value, font_size);
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
            if (result == FVM_OK)
            {
                result = fvm_object_call(context, panel_field, "setNewTextFormat", 1, &argument, &ignored);
            }
        }
        fvm_gc_unpin(context, format);
        if (result != FVM_OK)
        {
            return 0;
        }
    }
    fvm_value_boolean(&value, 1);
    if (fvm_object_set(context, panel_field, "multiline", &value) != FVM_OK ||
        fvm_object_set(context, field, "multiline", &value) != FVM_OK)
    {
        return 0;
    }
    fvm_value_boolean(&value, 0);
    if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK)
    {
        return 0;
    }
    pixels_valid = 0;
    pixels = malloc(canvas_width * canvas_height * 4U);
    snapshot = malloc(EDITOR_SOURCE_MAX + 1U);
    if (!pixels || !snapshot ||
        fvm_render_queue_create(1, dvm_editor_frame_render, dvm_editor_frame_destroy, &raster_queue) != FVM_OK)
    {
        return 0;
    }
    if (event_path)
    {
        events = fopen(event_path, "rb");
        if (!events)
        {
            return 0;
        }
    }
    if (!event_path || window_replay)
    {
#ifdef DVM_EDITOR_SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        {
            fprintf(stderr, "SDL: %s\n", SDL_GetError());
            return 0;
        }
        if (!dvm_editor_idle_initialize())
        {
            fprintf(stderr, "SDL wake registration failed\n");
            return 0;
        }
        window = SDL_CreateWindow("FlashVM / Dis IDE",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  canvas_width,
                                  canvas_height,
                                  SDL_WINDOW_RESIZABLE);
        if (window)
        {
            SDL_SetWindowMinimumSize(window, EDITOR_WIDTH_MIN, EDITOR_HEIGHT_MIN);
            SDL_SetWindowMaximumSize(window, EDITOR_EXTENT_MAX, EDITOR_EXTENT_MAX);
        }
        renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : 0;
        if (window && !renderer)
        {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
        texture = renderer
                      ? SDL_CreateTexture(
                            renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, canvas_width, canvas_height)
                      : 0;
        if (!texture)
        {
            fprintf(stderr, "SDL window: %s\n", SDL_GetError());
            return 0;
        }
        SDL_StartTextInput();
#else
        fprintf(stderr, "window support unavailable: build with DVM_EDITOR_SDL or provide --events\n");
        return 0;
#endif
    }
    return 1;
}

/* Record a clipped rectangle in the owner-held frame without document mutation.
 * surface supplies only pixel bounds; color is copied. Return one on admitted
 * geometry (including offscreen no-op), zero on contour allocation/budget failure. */
static int rectangle(FVM_RasterSurface *surface, double x, double y, double width, double height, FVM_RasterColor color)
{
    if (width < 0)
    {
        x += width;
        width = -width;
    }
    if (x + width <= 0 || y + height <= 0 || x >= surface->width || y >= surface->height)
    {
        return 1;
    }
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > surface->width)
    {
        width = surface->width - x;
    }
    if (y + height > surface->height)
    {
        height = surface->height - y;
    }
    FVM_RasterPoint points[4];
    points[0].x = x;
    points[0].y = y;
    points[1].x = x + width;
    points[1].y = y;
    points[2].x = x + width;
    points[2].y = y + height;
    points[3].x = x;
    points[3].y = y + height;
    {
        FVM_Path *path = NULL;
        FVM_PathCommand command;
        unsigned int index;
        FVM_Result result = fvm_path_create(&path);
        memset(&command, 0, sizeof(command));
        for (index = 0; index < 4 && result == FVM_OK; index++)
        {
            command.verb = index ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = points[index].x;
            command.y[0] = points[index].y;
            result = fvm_path_add(path, &command);
        }
        command.verb = FVM_PATH_CLOSE;
        if (result == FVM_OK)
        {
            result = fvm_path_add(path, &command);
        }
        if (result == FVM_OK && dvm_editor_frame_path(recording_frame, path, color))
        {
            return 1;
        }
        fvm_path_destroy(path);
        return 0;
    }
}

/* Map borrowed valid UTF-8 scalar positions to bounded preview byte offsets.
 * Offscreen positions return shown+1; bytes and document authority stay unchanged. */
static size_t scalar_byte(size_t shown, long position)
{
    size_t byte = 0;
    long scalar = view_first;
    const char *text = snapshot + display_start;
    if (position < view_first)
    {
        return shown + 1;
    }
    while (byte < shown && scalar < position)
    {
        byte++;
        while (byte < shown && ((unsigned char)text[byte] & 0xc0U) == 0x80U)
        {
            byte++;
        }
        scalar++;
    }
    return scalar == position ? byte : shown + 1;
}

/* Translate an admitted scalar insertion coordinate in borrowed valid UTF-8.
 * text/length are borrowed bytes; position is nonnegative. Returns a bounded byte count without mutation. */
static size_t text_byte(const char *text, size_t length, long position)
{
    size_t byte = 0;
    long scalar = 0;
    while (byte < length && scalar < position)
    {
        byte++;
        while (byte < length && ((unsigned char)text[byte] & 0xc0U) == 0x80U)
        {
            byte++;
        }
        scalar++;
    }
    return byte;
}

/* Translate a prevalidated source scalar through its immutable complete UTF-8 snapshot.
 * position is copied; returns a byte count and performs no VM, allocation or I/O operation. */
static size_t document_byte(long position)
{
    return text_byte(snapshot, extent, position);
}

/* Queue one copied clipboard result for the next ordinary input rendezvous.
 * status is an Idecore-compatible scalar; no source edit or channel operation occurs here. */
static void clipboard_result(int status)
{
    clipboard_status = status;
    clipboard_ack = 1;
}

/* Execute a Dis-authorized clipboard command against the previous complete immutable snapshot.
 * message is borrowed. Returns zero for a framing/identity error, one for a queued value outcome.
 * Native buffers remain owner-local; failed writes never authorize a Cut source mutation.
 * Platform clipboard access may block this native UI owner; no Dis heap pointer escapes. */
static int clipboard_command(const DVM_UIMessage *message)
{
    unsigned long ticket;
    char *candidate;
    const char *source;
    size_t length;
    size_t first;
    size_t last;
    size_t scalars;
    if (assembling || clipboard_ack || clipboard_transfer || message->size != EDITOR_CLIP_PREFIX ||
        message->flags != revision || message->flags != view_revision || !hexword(message->text, &ticket) ||
        ticket != view_ticket)
    {
        return 0;
    }
    clipboard_ticket = ticket;
    clipboard_revision = message->flags;
    if (message->code == EDITOR_CLIP_WRITE || message->code == EDITOR_CLIP_FIELD_WRITE)
    {
        source = snapshot;
        length = extent;
        scalars = view_total;
        if (message->code == EDITOR_CLIP_FIELD_WRITE)
        {
            if (!panel_mode || panel_revision != revision ||
                fvm_utf8_validate(panel_text, panel_size, &scalars) != FVM_UTF8_OK)
            {
                clipboard_result(EDITOR_CLIP_INVALID);
                return 1;
            }
            source = panel_text;
            length = panel_size;
        }
        if (message->first < 0 || message->second < message->first || (unsigned long)message->second > scalars)
        {
            return 0;
        }
        first = text_byte(source, length, message->first);
        last = text_byte(source, length, message->second);
        length = last - first;
        if (length > EDITOR_CLIP_MAX)
        {
            clipboard_result(EDITOR_CLIP_TOO_LARGE);
            return 1;
        }
        if (memchr(source + first, 0, length))
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        candidate = malloc(length + 1);
        if (!candidate)
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        memcpy(candidate, source + first, length);
        candidate[length] = 0;
#ifdef DVM_EDITOR_SDL
        if ((!events || system_clipboard_events) && SDL_SetClipboardText(candidate) != 0)
        {
            free(candidate);
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
#endif
        free(clipboard_text);
        clipboard_text = candidate;
        clipboard_result(EDITOR_CLIP_OK);
        return 1;
    }
    if (message->first || message->second)
    {
        return 0;
    }
    source = clipboard_text ? clipboard_text : "";
#ifdef DVM_EDITOR_SDL
    candidate = !events || system_clipboard_events ? SDL_GetClipboardText() : 0;
    if (!events || system_clipboard_events)
    {
        if (!candidate)
        {
            clipboard_result(EDITOR_CLIP_INVALID);
            return 1;
        }
        source = candidate;
    }
#endif
    length = 0;
    while (length <= EDITOR_CLIP_MAX && source[length])
    {
        length++;
    }
    clipboard_status = length > EDITOR_CLIP_MAX                                     ? 3
                       : fvm_utf8_validate(source, length, &scalars) == FVM_UTF8_OK ? 0
                                                                                    : 4;
    clipboard_transfer = clipboard_status ? 0 : malloc(length + 1);
    if (!clipboard_status && !clipboard_transfer)
    {
        clipboard_status = EDITOR_CLIP_INVALID;
    }
    if (!clipboard_status)
    {
        memcpy(clipboard_transfer, source, length + 1);
        clipboard_total = length;
        clipboard_offset = 0;
    }
#ifdef DVM_EDITOR_SDL
    if (!events || system_clipboard_events)
    {
        SDL_free(candidate);
    }
#endif
    if (clipboard_status)
    {
        clipboard_result(clipboard_status);
    }
    return 1;
}
