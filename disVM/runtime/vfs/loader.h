/* Copyright (c) 2026 bhgv
 * EN: Namespace-aware module admission and opened-file cache metadata.
 * RU: Namespace-aware допуск модулей и cache metadata открытого файла.
 */
#ifndef DVM_VFS_LOADER_H
#define DVM_VFS_LOADER_H
struct Module;
struct String;
struct Dir;
struct DVM_ResourcePlan;

#define DVM_MODULE_IMAGE_LIMIT (8UL * 1024UL * 1024UL) /* Existing reader's exclusive image byte bound. */

#define DVM_MODULE_NO_CACHE (1UL << 30) /* Private Dir marker when native identity cannot fit retained fields. */
#define DVM_MODULE_UNCACHEABLE_QID 1U   /* Copied into Module.qid: cannot match a later representable identity. */


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
#endif
