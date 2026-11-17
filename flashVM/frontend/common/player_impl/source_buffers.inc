/* Copyright (c) 2026 bhgv
 * EN: Shared player source_buffers implementation; included in owner-thread dependency order.
 * RU: Shared player source_buffers; подключается в порядке owner-thread зависимостей.
 */


/* Validate shared scalar settings without requiring a filesystem playlist. */
static int
memory_options(const FVM_DesktopOptions *options, const char *name, FVM_DesktopOptions *value, const char **error)
{
    if (!options)
    {
        if (error)
        {
            *error = "null player options";
        }
        return 0;
    }
    *value = *options;
    value->files = &name;
    value->file_count = 1;
    if (!fvm_desktop_options_validate(value, error))
    {
        return 0;
    }
    /* The temporary validation path array must never escape this helper. */
    value->files = 0;
    return 1;
}


/* Borrow exact binary bytes, including NUL, for the duration of synchronous playback. */
int fvm_desktop_run_memory(
    const FVM_DesktopOptions *options, const void *bytes, size_t length, const char *name, const char **error)
{
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    source.kind = FVM_DESKTOP_SOURCE_MEMORY;
    source.bytes = bytes;
    source.size = length;
    source.location = name;
    fvm_desktop_loader_init(&loader);
    if (options)
    {
        loader.max_loads = options->max_loads;
    }
#ifdef FVM_DESKTOP_HTTP
    loader.fetch = fvm_desktop_http_fetch;
#elif defined(FVM_DESKTOP_SOCKET_HTTP)
    loader.fetch = fvm_desktop_socket_fetch;
#endif
    return fvm_desktop_run_source(options, &source, &loader, error);
}


/* Accumulator owns only transport bytes; sink errors remain latched even if ignored by a provider.
 */
typedef struct FVM_DesktopDownload
{
    unsigned char *bytes;  /* Owned contiguous binary input. */
    size_t size, capacity; /* Used bytes and actual allocation, both bounded. */
    FVM_Result status;     /* First write failure. */
} FVM_DesktopDownload;


/* Consume chunks with checked arithmetic; a failing transport never exposes a partial movie. */
static FVM_Result collect_movie(const void *bytes, size_t size, void *userdata)
{
    FVM_DesktopDownload *download = (FVM_DesktopDownload *)userdata;
    size_t needed, capacity;
    unsigned char *grown;
    if (download->status != FVM_OK)
    {
        return download->status;
    }
    if ((!bytes && size) || size > FVM_SWF_DEFAULT_LIMIT - download->size)
    {
        return download->status = FVM_INVALID_ARGUMENT;
    }
    needed = download->size + size;
    if (needed > download->capacity)
    {
        capacity = download->capacity ? download->capacity : FVM_DESKTOP_FETCH_INITIAL;
        while (capacity < needed)
        {
            capacity = capacity > FVM_SWF_DEFAULT_LIMIT / 2 ? FVM_SWF_DEFAULT_LIMIT : capacity * 2;
        }
        grown = (unsigned char *)realloc(download->bytes, capacity);
        if (!grown)
        {
            return download->status = FVM_OUT_OF_MEMORY;
        }
        download->bytes = grown;
        download->capacity = capacity;
    }
    if (size)
    {
        memcpy(download->bytes + download->size, bytes, size);
    }
    download->size = needed;
    return FVM_OK;
}


/* Count only accessible, terminated location text within the explicit protocol bound. */
static size_t location_size(const char *text)
{
    size_t length = 0;
    if (!text)
    {
        return 0;
    }
    while (length <= FVM_DESKTOP_MAX_LOCATION && text[length])
    {
        ++length;
    }
    return length <= FVM_DESKTOP_MAX_LOCATION ? length : 0;
}


/* Default to startup only; the host explicitly opts in to deferred SWF root loading. */
int fvm_desktop_loader_init(FVM_DesktopLoader *out)
{
    if (!out)
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->max_loads = 1;
    return 1;
}


/* Join local-file references. URL resolution belongs to the supplied transport. */
static int local_location(const char *reference, const char *base, char *out)
{
    size_t length = location_size(reference), prefix = 0, i;
    if (!length)
    {
        return 0;
    }
    if (base && reference[0] != '/' && reference[0] != '\\' && !(length > 1 && reference[1] == ':'))
    {
        for (i = 0; base[i]; ++i)
        {
            if (base[i] == '/' || base[i] == '\\')
            {
                prefix = i + 1;
            }
        }
    }
    if (prefix > FVM_DESKTOP_MAX_LOCATION - length)
    {
        return 0;
    }
    if (prefix)
    {
        memcpy(out, base, prefix);
    }
    memcpy(out + prefix, reference, length + 1);
    return 1;
}
