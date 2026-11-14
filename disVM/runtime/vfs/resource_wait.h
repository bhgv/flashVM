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
#endif
