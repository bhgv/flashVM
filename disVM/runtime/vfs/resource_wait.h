/* Copyright (c) 2026 bhgv
 * EN: Source-local directory acquisition with native admission released.
 * RU: Source-local захват каталога с освобождённым native admission.
 */
#ifndef DVM_RESOURCE_WAIT_H
#define DVM_RESOURCE_WAIT_H
#include "resource.h"


/* EN: Open selected directory (directory!=0), or selected file's parent and
 * copy its basename into component. Own result remains in plan.created.
 * Return one/zero with guest error; root frame/plan stay source-owned across I/O.
 * RU: Открыть выбранный каталог (directory!=0) либо parent файла и записать
 * basename в component. Результат удерживает plan.created. Один/ноль с guest
 * error; root frame/plan принадлежат source до конца I/O. */
int dvm_resource_anchor_wait(DVM_ResourcePlan *plan, int directory, char *component);


/* EN/RU: ACTIVE caller borrows retained plan through native acquire and commit.
 * Return1 committed,0 stale,-1 refused/cancelled. Register eligible waits before
 * heap release, retire scope before reentry; preserve committed result after cancel.
 * ACTIVE caller заимствует plan для native acquire/commit. Вернуть1/0/-1;
 * scope до heap release и снять до reentry, поздняя отмена не отменяет commit. */
int dvm_resource_chdir_wait(DVM_ResourcePlan *plan);
#endif
