/* Copyright (c) 2026 bhgv
 * EN: Publish the mixed session's module-reference scope and guest file adapter.
 * RU: Публикация области ссылок парной сессии и файлового адаптера гостя.
 */
#ifndef DVM_TERMINAL_MODULE_REFS_H
#define DVM_TERMINAL_MODULE_REFS_H
#include "../vfs/resource.h"

struct StyxClient; /* Attached ordinary FlashVM export. */
typedef struct DvmModuleRefs DvmModuleRefs; /* Namespace-owned service and upstream mount pin. */


/* EN: Install /.varref before serving group. Borrow the result while group owns its binding.
 * RU: Установить /.varref до обслуживания group. Результат заимствуется до снятия привязки. */
DvmModuleRefs *dvm_terminal_module_refs_install(DVM_Namespace *group);


/* EN: Pin the attached owner once, before starting the guest. NULL is allowed.
 * RU: Удержать подключение владельца один раз до запуска гостя. NULL допустим. */
void dvm_terminal_module_refs_attach(DvmModuleRefs *service, struct StyxClient *issuer);
#endif
