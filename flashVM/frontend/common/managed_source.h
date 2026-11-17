/* Copyright (c) 2026 bhgv
 * EN: Managed player resource acquisition delegates code/data bytes to the owning DisVM bridge.
 * RU: Managed player получает code/data bytes только через мост владеющей DisVM.
 */
#ifndef FVM_DESKTOP_MANAGED_SOURCE_H
#define FVM_DESKTOP_MANAGED_SOURCE_H /* Guard the independent C89 host acquisition API. */
#include "source.h"
#define FVM_DESKTOP_FETCH_CODE 1U /* Execution-capable movie resource; adapter uses provider-first lookup. */
#define FVM_DESKTOP_FETCH_DATA 2U /* Ordinary variable/form data; adapter uses ordinary data lookup. */
#ifdef __cplusplus
extern "C" {
#endif


/* EN/RU: Synchronous native-only resource bridge, same borrowing/sink rules as DesktopFetch.
 * Purpose is a native operation identifier, not an S-record tag or permission grant.
 * Resolve reference/base in the delegated VFS, stream bounded bytes through write, return actual status.
 * No VM/SDL entry, recursive playback, hidden local I/O or retained borrowed pointers.
 * Native-only bridge: reference/base разрешает VFS, bytes идут в write; вернуть actual status.
 * Purpose не wire tag/permission; inputs borrowed, без VM/SDL entry и retained pointers. */
typedef FVM_Result (*FVM_DesktopManagedFetch)(unsigned int purpose, const char *reference,
    const char *base, FVM_DesktopWrite write, void *write_userdata, char *resolved,
    size_t capacity, void *userdata);


/* EN/RU: Caller-owned bridge policy; copied once for the synchronous source run.
 * Caller-owned policy моста; копируется один раз для synchronous run. */
typedef struct FVM_DesktopManagedLoader
{
    size_t struct_size; /* Exact descriptor size, initialized by loader_init. */
    FVM_DesktopManagedFetch fetch; /* Required explicit bridge, no native default provider. */
    void *userdata; /* Borrowed host/transport state until run returns. */
    unsigned long max_loads; /* Existing total resource budget including startup; default1. */
} FVM_DesktopManagedLoader;


/* EN/RU: Initialize exact descriptor with no bridge and one startup resource;1/0 for output/NULL.
 * No allocation, callback or I/O. Инициализировать descriptor;1/0, без allocation/callback/I/O. */
FVM_DESKTOP_API int fvm_desktop_managed_loader_init(FVM_DesktopManagedLoader *out);


/* EN/RU: Run with every FILE/URL acquisition delegated, including nested movies/variables.
 * MEMORY startup bytes are already supplied by the bridge; location is its explicit VFS provenance.
 * Return existing desktop0/1/2 statuses; optional static error, no local/HTTP fallback on any failure.
 * Owner thread only; source/options/state borrowed through return; callback may block like legacy fetch.
 * Выполнить все FILE/URL acquisitions через bridge; MEMORY уже передан, location — VFS provenance.
 * Owner-thread run, borrowed inputs; existing0/1/2, optional static error, без local/HTTP fallback. */
FVM_DESKTOP_API int fvm_desktop_run_source_managed(const FVM_DesktopOptions *options,
    const FVM_DesktopSource *source, const FVM_DesktopManagedLoader *loader, const char **error);
#ifdef __cplusplus
}
#endif
#endif
