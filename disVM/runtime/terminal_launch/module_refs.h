/* Copyright (c) 2026 bhgv
 * EN: Publish the mixed session's module-reference scope and guest file adapter.
 * RU: Публикация области ссылок парной сессии и файлового адаптера гостя.
 */
#ifndef DVM_TERMINAL_MODULE_REFS_H
#define DVM_TERMINAL_MODULE_REFS_H
#include "../vfs/resource.h"

struct StyxClient; /* Attached ordinary FlashVM export. */
struct StyxCallerServer; /* Reverse ordinary server which issued local reference Qids. */
typedef struct DvmModuleRefs DvmModuleRefs; /* Namespace-owned service and upstream mount pin. */


/* EN: Install /.varref before serving group. Borrow the result while group owns its binding.
 * RU: Установить /.varref до обслуживания group. Результат заимствуется до снятия привязки. */
DvmModuleRefs *dvm_terminal_module_refs_install(DVM_Namespace *group);


/* EN: Pin the local publication server for ref adoption before the forward attach.
 * RU: Удержать местный сервер публикаций до прямого attach для приёма ссылок. */
int dvm_terminal_module_refs_local(DvmModuleRefs *service, struct StyxCallerServer *local);


/* EN: Pin the attached owner once, before starting the guest. NULL is allowed.
 * RU: Удержать подключение владельца один раз до запуска гостя. NULL допустим. */
void dvm_terminal_module_refs_attach(DvmModuleRefs *service, struct StyxClient *issuer);
#endif
