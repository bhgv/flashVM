/* Copyright (c) 2026 bhgv
 * EN: Session control and namespace projection for remotely loaded Dis modules.
 * RU: Управление сессией и проекция пространства загруженных модулей Dis.
 */
#ifndef DVM_LIVE_MODULES_H
#define DVM_LIVE_MODULES_H
#include "../vfs/resource.h"

struct StyxClient; /* Connected peer transport owned by the mount. */


/* EN: Enable one broker for the selected borrowed live namespace.
 * RU: Включить брокер для выбранного заимствованного пространства. */
int dvm_live_modules_start(DVM_Namespace *group, struct StyxClient *client, DVM_Provider *reverse);


/* EN: Start the dedicated listener after the resident scheduler is ready.
 * RU: Запустить отдельный приёмник после подготовки планировщика владельцев. */
int dvm_live_modules_boot(void);

/* EN: Record the primary guest before dispatch and stop loads on its retirement.
 * RU: Записать основной процесс до запуска и завершить загрузки при его выходе. */
void dvm_live_modules_primary(int pid);
void dvm_live_modules_primary_exit(int pid);
/* EN: Stop admission and shut down internal routes before joining managed owners.
 * RU: Прекратить приём и закрыть внутренние маршруты до ожидания владельцев. */
void dvm_live_modules_stop(void);
/* EN: Release routes after the reverse dispatcher and runtime have joined.
 * RU: Освободить маршруты после завершения диспетчера и VM. */
void dvm_live_modules_finish(void);
/* EN: Route an exact hidden path to a retained provider. Adjust path on success.
 * RU: Направить точный скрытый путь в удерживаемый провайдер и уточнить path. */
DVM_Provider *dvm_live_modules_route(void *group, const char **path, int *reserved);

#endif
