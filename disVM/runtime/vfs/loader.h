/* Copyright (c) 2026 bhgv
 * EN: Namespace-aware module admission and opened-file cache metadata.
 * RU: Namespace-aware допуск модулей и cache metadata открытого файла.
 */
#ifndef DVM_VFS_LOADER_H
#define DVM_VFS_LOADER_H
#include <stdint.h>
struct Module;
struct String;
struct Dir;
struct DVM_ResourcePlan;
struct DVM_Provider;
struct DVM_ScalarFiles;
struct Import;
struct Modlink;

#define DVM_MODULE_IMAGE_LIMIT (8UL * 1024UL * 1024UL) /* Existing reader's exclusive image byte bound. */

#define DVM_MODULE_NO_CACHE (1UL << 30) /* Private Dir marker when native identity cannot fit retained fields. */
#define DVM_MODULE_UNCACHEABLE_QID 1U   /* Copied into Module.qid: cannot match a later representable identity. */
#define DVM_MODULE_ADAPTER_QID 2U /* Private opened-image provenance for the exact adapter provider. */


/* EN: Original reader on an already frozen native path. Consumes cached module
 * reference; sync retains its existing source-local native release contract.
 * RU: Исходный reader на frozen native path. Поглощает cached module reference;
 * sync сохраняет существующий source-local native release контракт. */
struct Module *
dvm_namespace_native_readmod(char *path, struct Module *cached, int sync, struct DVM_ResourcePlan *input);


/* EN/RU: Open retained module selection on its current source; return owned FD/-1.
 * Открыть удержанный module selection своим source; вернуть свой FD/-1. */
int dvm_namespace_module_open(struct DVM_ResourcePlan *input);


/* EN: Validate borrowed rooted guest name before conversion. Empty/nil-H gives
 * ordinary load failure; embedded NUL cannot alias a builtin or file prefix.
 * Native NULL/malformed objects retain the ordinary VM error path.
 * RU: Проверить укоренённое guest name до conversion. Empty/nil-H даёт обычный
 * отказ load; embedded NUL не становится builtin/file prefix. Native NULL либо
 * malformed object сохраняют обычный VM error path. */
char *dvm_namespace_load_name(struct String *name);


/* EN/RU: Test private copied metadata for exact representability before cache use.
 * Проверить точную представимость private metadata до использования cache. */
int dvm_namespace_module_cacheable(const struct Dir *metadata);


/* [GC:] EN: ACTIVE source borrows one selected provider plan/path; returns owned parsed Module/NULL.
 * Opened-handle metadata and exact bounded bytes are authoritative; no cache reuse by logical path.
 * Requires an accepted native execution-provider launch or an exact local adapter grant.
 * Provider PATH and opened-handle INFO/DATA cancellation are required; cleanup precedes managed reentry, caller retains/frees plan.
 * Ordinary read permission semantics match Dis load; intent changes source order, not authority.
 * RU: ACTIVE source заимствует selected provider plan/path; свой parsed Module/NULL.
 * Metadata открытого handle и bounded bytes определяют образ; cache по logical path не используется.
 * Нужен принятый provider-launch или точный локальный адаптер. PATH и INFO/DATA cancellation
 * обязательны; cleanup до managed reentry,
 * caller удерживает/освобождает plan.
 * Обычные read-права соответствуют Dis load; intent меняет порядок источников, не полномочия.
 */
/* EN/RU: Open a host-selected launch image directly. / Открыть выданный хостом образ запуска. */
struct Module *dvm_provider_read_launch_image(struct DVM_Provider *provider, char *label);
struct Module *dvm_provider_readmod(char *path, struct DVM_ResourcePlan *plan, const struct Import *imports);


/* EN/RU: Borrow the associated local adapter, reporting denied execution separately.
 * Заимствовать локальный адаптер и отдельно сообщить об отказе исполнения. */
struct DVM_Provider *dvm_module_adapter_source(struct DVM_Provider *provider, const char *relative, int *denied, struct DVM_Provider **profile, unsigned long *tag, unsigned long *request_tag, unsigned long *reply_tag, struct DVM_ScalarFiles **files);
/* EN/RU: Consume the loaded module through ordinary linking and bind adapter instances.
 * Передать модуль обычному линкеру и закрепить окружение экземпляров адаптера. */
struct Modlink *dvm_namespace_link_loaded(struct Module *module, struct Import *imports);
/* EN/RU: Resolve once and propagate the instruction's borrowed import table.
 * Разрешить путь один раз и передать заимствованную таблицу импортов инструкции. */
struct Module *dvm_namespace_read_imports(char *path, struct Module *cached, int sync,
    const struct Import *imports);
/* EN/RU: Build one private native scalar module after admitting its interface.
 * Создать приватный нативный скалярный модуль после проверки интерфейса. */
struct Module *dvm_scalar_module(char *path, char *name, unsigned long request_tag,
    unsigned long reply_tag, uint64_t revision, struct DVM_ScalarFiles *files);
void dvm_scalar_module_retire(struct Module *module);
#endif
