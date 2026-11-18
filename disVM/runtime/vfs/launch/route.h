/* Copyright (c) 2026 bhgv
 * EN: Immutable native launch addresses and pinned caller-relative VFS-to-admission routing.
 * RU: Immutable native launch addresses и pinned caller-relative VFS-to-admission routing.
 */
#ifndef DVM_LAUNCH_ROUTE_H
#define DVM_LAUNCH_ROUTE_H
#include "service.h"
typedef struct DVM_LaunchAddress DVM_LaunchAddress; /* Owned native submit destination plus owner-chosen logical module label. */


/* EN/RU: Copy nonempty bounded module label and clone borrowed endpoint; owned address/NULL, no I/O or code execution.
 * module is a NUL-terminated string shorter than DVM_NS_PATH_LIMIT, not a resolved host path.
 * Caller retains its inputs through return; closed/OOM/invalid refuses without consuming them.
 * Скопировать непустой bounded module label и clone borrowed endpoint; свой address/NULL, без I/O/execution.
 * module — NUL-terminated string короче DVM_NS_PATH_LIMIT, не host path; inputs удерживаются до return.
 */
DVM_LaunchAddress *dvm_launch_address_create(DVM_LaunchEndpoint *endpoint, const char *module);


/* EN/RU: Submit through borrowed immutable address using spec's terminal/channel grants, policy and copied arguments.
 * Ignore spec.module/expected_provider: destination capability supplies both. Owned requester ticket/NULL.
 * Pre-cancel refuses before submission; acknowledged ticket survives later racing cancellation, no replay/retry.
 * No provider I/O/init. Callers join address users before free; service owner retains final review/prepare authority.
 * Подать request через borrowed address с grants/policy/arguments из spec; destination задаёт module/expected_provider.
 * Свой requester ticket/NULL; pre-cancel отказывает, acknowledged ticket сохраняется при racing cancellation.
 * Без provider I/O/init; users join до address free, owner сохраняет final review/prepare authority.
 */
DVM_LaunchTicket *dvm_launch_address_submit(DVM_LaunchAddress *address, const DVM_LaunchSpec *spec);


/* EN/RU: Consume exclusive address after all users join; NULL allowed, no service close or request revocation.
 * Освободить exclusive address после join users; NULL допустим, без service close/request revocation.
 */
void dvm_launch_address_free(DVM_LaunchAddress *address);


/* EN/RU: Native resolver borrows one already selected plan/metadata through return, produces owned address/NULL.
 * Inspect actual provider/anchor capability and relative name, not an ambient path or identity string as authority.
 * Never copy/alter/retain the plan, run guest code or enter managed admission. Native I/O may block outside locks;
 * consume current cancellation scope. Callback/context code lives through routed submit and must return normally.
 * NULL refuses this selection; router never tries another candidate after resolver refusal.
 * Native resolver заимствует selected plan/metadata до return, выдаёт owned address/NULL.
 * Использовать actual capability/relative name; не считать path/identity authority. Plan не менять/копировать/удерживать.
 * Без guest/managed admission; native I/O вне locks с current cancellation scope. NULL — final отказ выбранного source.
 */
typedef DVM_LaunchAddress *(*DVM_LaunchLocate)(void *context, const DVM_ResourcePlan *selected, const FVM_FileInfo *metadata);


/* EN/RU: Select visible call target using local_role, obtain its native address and submit exactly once; owned ticket/NULL.
 * Borrow group/path/spec/locate/context through return; native-only outside managed admission, no code image download.
 * Preflight validates caller grants/arguments before provider I/O; callback only sees a selected nondirectory node.
 * Current-scope metadata requires cancellable candidates. Pin selected source across resolver even if namespace changes.
 * Request/permission/cancellation/resolver/quota failures never retry another side. Owner reviews/authorizes separately.
 * Выбрать visible target по local_role, получить native address и один раз submit; свой ticket/NULL.
 * Все inputs borrowed до return; native-only вне managed admission, без скачивания code image.
 * Preflight до I/O; callback видит selected nondirectory node. Source удерживается при смене namespace.
 * Failures не вызывают retry другой стороны; review/authorization остаются у owner.
 */
DVM_LaunchTicket *dvm_launch_route_submit(DVM_Namespace *group, const char *path, unsigned local_role,
    const DVM_LaunchSpec *spec, DVM_LaunchLocate locate, void *context);
#endif
