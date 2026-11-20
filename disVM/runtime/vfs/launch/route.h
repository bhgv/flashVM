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


/* EN/RU: Bounded native local permission check for address use;1 allow/otherwise deny, no I/O/VM/waits.
 * Borrow context through call. / Bounded native local permission check;1 разрешить, иначе отказать,
 * context borrowed до return; без I/O/VM/waits.
 */
typedef int (*DVM_LaunchCheck)(void *context);


/* EN/RU: Wrap an owned address with live local policy; consume inner on EVERY outcome.
 * Adopt context/release only on success; caller cleans its context when NULL is returned.
 * New owned address/NULL on invalid/OOM. check is required and outlives wrapper; release is optional,
 * nonblocking native cleanup. Gate does not retarget inner; joins before free apply to the whole chain.
 * Обернуть owned address live local policy; inner поглощается ВСЕГДА. Context/release принять лишь при success,
 * при NULL context очищает caller. Свой address/NULL; check обязателен, cleanup native/nonblocking.
 */
DVM_LaunchAddress *dvm_launch_address_guard(DVM_LaunchAddress *inner, DVM_LaunchCheck check,
    void *context, void (*release)(void *));


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

/* EN/RU: Like route_submit using the selected provider's advertised launch callback, no per-request locator map.
 * Native anchors/unadvertised/denied selected nodes refuse; never retry another side or download code.
 * Same borrowed inputs, native cancellation and one acknowledged ticket contract as route_submit.
 * Как route_submit через advertised callback выбранного provider, без per-request locator map.
 * Native anchors/unadvertised/denied nodes отказывают без retry/скачивания; inputs borrowed, cancellation/ack прежние.
 */
DVM_LaunchTicket *dvm_launch_route_discover(DVM_Namespace *group, const char *path, unsigned local_role,
    const DVM_LaunchSpec *spec);


/* EN/RU: Resolve a caller-relative module to an owned reusable address/NULL, without submitting or reserving quota.
 * Borrow group/path through return; native-only outside managed admission, current cancellation scope applies.
 * Same selection/metadata/refusal rules as route_discover; late cancellation frees the acquired address.
 * Caller frees address after joining its users. No caller namespace is retained or re-resolved on submit;
 * address retains only its native capability and optional live policy. New resolution observes new bindings.
 * No code bytes, guest execution, admission/authorization or automatic owner execution occur here.
 * Разрешить module caller в свой reusable address/NULL, без submit и quota reservation.
 * group/path borrowed до return; native-only вне managed admission, current cancellation scope.
 * Правила selection/metadata/refusal как route_discover; поздняя отмена освобождает address.
 * Caller освобождает после join users; namespace не удерживается и не разрешается при submit.
 * Address сохраняет capability/live policy; новый resolve видит новые binds. Без чтения кода и запуска guest.
 */
DVM_LaunchAddress *dvm_launch_route_resolve(DVM_Namespace *group, const char *path, unsigned local_role);


/* EN/RU: A transport acknowledgement covers request delivery, not guest init.
 * Подтверждение транспорта относится к доставке запроса, а не к init гостя. */
typedef enum DVM_LaunchDelivery
{
    DVM_LAUNCH_NOT_SENT = 0, /* Refused before delivery, or conclusively rejected by the peer. */
    DVM_LAUNCH_DELIVERED = 1, /* Peer acknowledged the exact complete request. */
    DVM_LAUNCH_UNKNOWN = -1 /* Partial/uncertain transport outcome; no automatic retry. */
} DVM_LaunchDelivery;

/* EN/RU: Native transport callback; borrow argument bytes until return.
 * Native transport callback; байты заимствованы до возврата. */
typedef DVM_LaunchDelivery (*DVM_LaunchSend)(void *, const void *, size_t);
typedef DVM_LaunchDelivery (*DVM_LaunchSendProcess)(void *, uint32_t, const void *, size_t);

/* EN/RU: Construct a transport-only address; adopts state only on success.
 * Создать адрес транспортной доставки; state принимается только при успехе. */
DVM_LaunchAddress *dvm_launch_address_transport(DVM_LaunchSend send, void *state, void (*release)(void *));

/* EN/RU: Select an opt-in transport using a caller PID captured before native wait.
 * Выбрать транспорт с PID вызывающего, сохранённым до нативного ожидания. */
DVM_LaunchAddress *dvm_launch_address_process_transport(DVM_LaunchSendProcess send, void *state, void (*release)(void *));

/* EN/RU: Submit once with a trusted PID; unscoped addresses use the original format.
 * Отправить один раз с доверенным PID; обычные адреса используют прежний формат. */
DVM_LaunchDelivery dvm_launch_address_send_process(DVM_LaunchAddress *address, uint32_t pid,
    const void *arguments, size_t size);

/* EN/RU: Submit through the already resolved transport address once, honoring its guards.
 * Подать запрос через разрешённый транспортный адрес один раз с проверкой guards. */
DVM_LaunchDelivery dvm_launch_address_send(DVM_LaunchAddress *address, const void *arguments, size_t size);
#endif
