/* Copyright (c) 2026 bhgv
 * EN: Managed acquisition entry points reuse the ordinary owner-thread player lifecycle.
 * RU: Managed acquisition использует обычный owner-thread lifecycle player.
 */


/* EN/RU: See managed_source.h; initialization performs no transport or SDL operation.
 * См. managed_source.h; initialization не выполняет transport/SDL operations. */
int fvm_desktop_managed_loader_init(FVM_DesktopManagedLoader *out)
{
    if (!out)
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    out->max_loads = 1;
    return 1;
}


/* EN/RU: See managed_source.h; snapshot the bridge policy before any callback or movie operation.
 * См. managed_source.h; копировать bridge policy до callbacks/movie operations. */
int fvm_desktop_run_source_managed(const FVM_DesktopOptions *options, const FVM_DesktopSource *source,
    const FVM_DesktopManagedLoader *loader, const char **error)
{
    FVM_DesktopManagedLoader snapshot;
    FVM_DesktopLoader budget;
    if (error)
    {
        *error = "invalid managed acquisition bridge";
    }
    if (!loader || loader->struct_size != sizeof(*loader) || !loader->fetch ||
        !loader->max_loads || loader->max_loads > FVM_DESKTOP_MAX_LOADS)
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    snapshot = *loader;
    fvm_desktop_loader_init(&budget);
    budget.max_loads = snapshot.max_loads;
    return run_source(options, source, &budget, error, 0, 0, 0, 0, &snapshot);
}
