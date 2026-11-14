/* Copyright (c) 2026 bhgv
 * EN: Explicit unsupported anchor backend until native relative HANDLE opens exist.
 * RU: Явный отказ anchor backend до реализации native relative HANDLE opens.
 */


/* EN/RU: Refuse without creating a path-reopening substitute; *out is cleared.
 * Отказать без подмены повторным path-open; *out очищается. */
FVM_Result fvm_port_anchor_open(const char *path, FVM_PortAnchor **out)
{
    (void)path;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = NULL;
    return FVM_UNSUPPORTED;
}


/* EN/RU: No owned Windows anchor can be created yet; clear output and refuse.
 * Windows anchor пока не создаётся; очистить output и отказать. */
FVM_Result fvm_port_anchor_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortAnchor **out)
{
    (void)parent;
    return fvm_port_anchor_open(relative, out);
}


/* EN/RU: Constructor always refuses; NULL cleanup is harmless.
 * Constructor всегда отказывает; cleanup NULL безопасен. */
void fvm_port_anchor_close(FVM_PortAnchor *anchor)
{
    (void)anchor;
}


/* EN/RU: Preserve borrowed output; relative HANDLE stat is not implemented.
 * Сохранить borrowed output; relative HANDLE stat не реализован. */
FVM_Result fvm_port_anchor_info_at(FVM_PortAnchor *parent, const char *relative, FVM_FileInfo *output)
{
    (void)parent;
    (void)relative;
    (void)output;
    return FVM_UNSUPPORTED;
}


/* EN/RU: Return no stream; never fall back to concatenating the parent pathname.
 * Не возвращать stream; не подменять конкатенацией parent pathname. */
void *fvm_fs_native_open_at(FVM_PortAnchor *parent, const char *relative, const char *mode)
{
    (void)parent;
    (void)relative;
    (void)mode;
    errno = ENOSYS;
    return NULL;
}


/* EN/RU: Clear cursor output and refuse; no path-based fallback.
 * Очистить cursor output и отказать; без path-based fallback. */
FVM_Result fvm_port_directory_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortDirectory **out)
{
    (void)parent;
    (void)relative;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = NULL;
    return FVM_UNSUPPORTED;
}
