/* Copyright (c) 2026 bhgv
 * EN: Retained trusted native provider factories; no managed values or guest callbacks.
 * RU: Удерживаемые trusted native provider factories; без managed values/callbacks.
 */
#ifndef DVM_VFS_PROVIDER_H
#define DVM_VFS_PROVIDER_H
#include "handle.h"
#define DVM_PROVIDER_STABLE_BIND 1UL /* Factory promises immutable relative-name/node identity for bind aliases. */
typedef struct DVM_Provider DVM_Provider; /* Native refcounted factory; references pin code/state lifetime. */


/* EN/RU: Immutable trusted callbacks, code alive through destroy. Relative paths
 * are normalized namespace suffixes. Open transfers one independent valid handle
 * only on success; failure leaves it empty. Destroy is nonblocking native cleanup.
 * Immutable callbacks; code живёт до destroy. Relative paths — нормализованные
 * suffixes. Open передаёт valid handle при успехе, при отказе оставляет пустым.
 * Destroy — nonblocking native cleanup. No managed access or namespace reentry. */
typedef struct DVM_ProviderOps
{
    FVM_Result (*info)(void *state, const char *relative, FVM_FileInfo *output); /* Borrow paths/output; blocking native metadata. */
    FVM_Result (*open)(void *state, const char *relative, const char *mode, DVM_VfsHandle *output); /* Native factory; empty output, no VM pointers. */
    void (*destroy)(void *state); /* Last factory/handle reference consumes state outside locks. */
    unsigned long flags; /* Declared native capabilities; unknown bits refuse creation. */
} DVM_ProviderOps;


/* EN/RU: Create native factory owning state on success only; ops must outlive it.
 * Return owned reference or NULL/OOM; no callbacks. Создать factory, принять
 * state только при успехе. Своя ссылка либо NULL; callbacks не вызываются. */
DVM_Provider *dvm_provider_create(const DVM_ProviderOps *ops, void *state);


/* EN/RU: Retain live borrowed provider; NULL succeeds; zero on ref overflow.
 * Удержать live provider; NULL допустим; ноль при overflow. No callbacks/I/O. */
int dvm_provider_retain(DVM_Provider *provider);


/* EN/RU: Consume reference; NULL harmless. Last reference destroys native state
 * outside mutex. Поглотить ссылку; последняя очищает native state вне mutex. */
void dvm_provider_release(DVM_Provider *provider);


/* EN/RU: Borrow retained provider/path/output through blocking native metadata.
 * Заимствовать удержанный provider/path/output на время blocking native metadata. */
FVM_Result dvm_provider_info(DVM_Provider *provider, const char *relative, FVM_FileInfo *output);


/* EN/RU: Open relative path/mode into empty output. Preallocate a lifetime wrapper
 * and retain factory before callback; success pins factory until handle close.
 * Открыть relative path/mode в пустой output; wrapper/ref до callback. Успех
 * удерживает factory до close handle. No managed access; return port status. */
FVM_Result dvm_provider_open(DVM_Provider *provider, const char *relative, const char *mode, DVM_VfsHandle *output);

/* EN/RU: Query immutable stable-bind promise on a retained provider; no callback.
 * Проверить immutable stable-bind promise удержанного provider; без callback. */
int dvm_provider_bindable(DVM_Provider *provider);
#endif
