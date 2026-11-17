/* Copyright (c) 2026 bhgv
 * EN: Resource purpose controls code lookup independently of ordinary data stack order.
 * RU: Назначение ресурса задаёт code lookup независимо от обычного порядка data stack.
 */
#ifndef DVM_VFS_INTENT_H
#define DVM_VFS_INTENT_H
#include "resource.h"


/* EN/RU: Native intent identifiers, not wire tags, file suffix tests or execution permissions.
 * Native intent identifiers; не wire tags, не suffix tests и не разрешения на исполнение. */
typedef enum DVM_ResourceIntent
{
    DVM_RESOURCE_DATA = 0, /* Ordinary data, files, channels, sockets and devices: current stack order. */
    DVM_RESOURCE_DIS, /* Dis module and dependency lookup: provider candidates first. */
    DVM_RESOURCE_INTERFACE, /* Module interface source, including .m descriptions. */
    DVM_RESOURCE_NATIVE, /* Future native module lookup; no format/loader assigned here. */
    DVM_RESOURCE_SWF /* Initial or nested SWF code, not ordinary media/data. */
} DVM_ResourceIntent;


/* EN/RU: Resolve explicit purpose into empty caller-owned frozen plan;1/0, no provider I/O.
 * DATA uses cwd/data order; code uses import base and provider-first visible union candidates.
 * Preserve order within provider/other groups and all namespace visibility/anchoring rules.
 * Invalid arguments clear non-NULL output; never overwrite a live plan; close successful plans.
 * Разрешить purpose в empty owned plan;1/0, без provider I/O. DATA — cwd/order,
 * code — import base/provider-first; сохранить namespace visibility и порядок внутри групп. */
int dvm_resource_plan_intent(DVM_Namespace *group, const char *path, DVM_ResourceIntent intent, DVM_ResourcePlan *plan);
#endif
