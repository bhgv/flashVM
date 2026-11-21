/* Copyright (c) 2026 bhgv
 * EN: Retained trusted native provider factories; no managed values or guest callbacks.
 * RU: Удерживаемые trusted native provider factories; без managed values/callbacks.
 */
#ifndef DVM_VFS_PROVIDER_H
#define DVM_VFS_PROVIDER_H
#include <stdint.h>
#include "handle.h"
#include "anchor_port.h"
#include "changes.h"
#include "walk.h"
#define DVM_VFS_CYCLE ((FVM_Result)-7) /* Proven traversal cycle; not NOT_FOUND or a permission decision. */

#define DVM_PROVIDER_CANCEL_LAUNCH 8U /* Requires PATH and a launch callback; address discovery consumes current scope. */
#define DVM_PROVIDER_CANCEL_CAPTURE 4U /* Requires PATH; capture consumes scope and returns a PATH-capable root. */
#define DVM_PROVIDER_CANCEL_LIST 2U /* Requires PATH; directory open/list and all returned stream closes consume scope. */
#define DVM_PROVIDER_CANCEL_PATH 1U /* Native factory info/open/access consume current cancellation scope. */
#define DVM_PROVIDER_STABLE_BIND 1UL /* Factory promises immutable relative-name/node identity for bind aliases. */
typedef struct DVM_Provider DVM_Provider; /* Native refcounted factory; references pin code/state lifetime. */


/* EN/RU: Capture one live relative node into an owned fresh root provider/NULL.
 * Root dot must stay anchored; preserve access policy, never reopen an old absolute path.
 * Native I/O permitted outside namespace/lifetime locks; caller pins source through return.
 * Захватить relative node в свой fresh root provider/NULL; сохранить policy, native I/O вне locks. */
typedef DVM_Provider *(*DVM_ProviderCapture)(void *state, const char *relative);


struct DVM_LaunchAddress; /* Native owned submit capability; never serialized as a pointer. */


/* EN/RU: Borrow canonical relative node and state; return owned native launch address/NULL, no guest execution.
 * Enforce current source policy; native I/O may block outside locks, current scope requires explicit LAUNCH opt-in.
 * Returned addresses may outlive the factory: bind any required ongoing policy/lifetime with an address guard.
 * Заимствовать canonical relative node/state; вернуть owned native launch address/NULL, без guest execution.
 * Проверять current source policy; native I/O вне locks, current scope требует LAUNCH opt-in.
 * Address переживает factory: ongoing policy/lifetime закреплять address guard.
 */
typedef struct DVM_LaunchAddress *(*DVM_ProviderLaunch)(void *state, const char *relative);


/* EN/RU: Immutable trusted callbacks, code alive through destroy. Relative paths
 * are normalized namespace suffixes. Open transfers one independent valid handle
 * only on success; failure leaves it empty. Destroy is nonblocking native cleanup.
 * Immutable callbacks; code живёт до destroy. Relative paths — нормализованные
 * suffixes. Open передаёт valid handle при успехе, при отказе оставляет пустым.
 * Destroy — nonblocking native cleanup. No managed access or namespace reentry. */
typedef struct DVM_ProviderOps
{
    FVM_Result (*info)(void *state, const char *relative, FVM_FileInfo *output); /* Borrow paths/output; blocking native metadata. */
    FVM_Result (*open)(void *state, const char *relative, const char *mode, DVM_VfsHandle *output); /* Native factory; empty output, no VM pointers. */
    void (*destroy)(void *state); /* Last factory/handle reference consumes state outside locks. */
    unsigned long flags; /* Declared native capabilities; unknown bits refuse creation. */
    FVM_Result (*access)(void *state, const char *relative, unsigned access); /* Optional effective rights check; no effects/VM access. */
} DVM_ProviderOps;


/* EN: Trusted native binding between a mounted function and its reverse session.
 * Begin borrows the exact target and module tree, returning a lease or NULL.
 * End consumes the lease. Both callbacks execute in native wait scope.
 * RU: Доверенная связь файла функции со встречной сессией.
 * Begin заимствует точный узел и дерево модуля, возвращает аренду или NULL.
 * End завершает аренду. Оба вызова выполняются в области нативного ожидания. */
typedef struct DVM_CallerBinding
{
    void *(*begin)(void *state, DVM_Provider *target, DVM_Provider *tree, uint32_t pid); /* Start reverse publication. */
    void (*end)(void *state, void *lease); /* Retire one active publication. */
    void *state; /* Context held by the bound provider. */
    void (*release)(void *state); /* Optional owned context retirement. */
} DVM_CallerBinding;


/* EN: Install once on an exclusively owned unpublished native provider.
 * On success adopts the context when release is provided. Failure leaves ownership intact.
 * RU: Установить один раз на свой неопубликованный нативный провайдер.
 * При успехе принимает контекст с release. При отказе владение сохраняется. */
int dvm_provider_bind_caller(DVM_Provider *provider, const DVM_CallerBinding *binding);


/* EN: Copy the trusted binding inherited by a captured node, or return zero.
 * Keep a reference to the input provider while calling the copied callbacks.
 * RU: Скопировать доверенную связь захваченного узла либо вернуть ноль.
 * Удерживать входной провайдер при обращении к скопированным callback. */
int dvm_provider_caller_binding(DVM_Provider *provider, DVM_CallerBinding *binding);


/* EN/RU: Set stable logical projection/node identity on an exclusively owned unpublished factory;1/0.
 * entity is native trusted metadata, never permission. Refuse after any callback/get or shared publication.
 * Установить logical projection/node identity в exclusive unpublished factory;1/0, не permission.
 */
int dvm_provider_walk_identity(DVM_Provider *provider, const WalkEntity *entity, unsigned long node_high, unsigned long node_low);


/* EN/RU: Copy and seal immutable native projection identity; borrowed provider/output through return,1/0 invalid.
 * Скопировать и seal immutable projection identity; provider/output borrowed до return,1/0 invalid.
 */
int dvm_provider_walk_get(DVM_Provider *provider, WalkEntity *entity);


/* EN/RU: Create native factory owning state on success only; ops must outlive it.
 * Return owned reference or NULL/OOM; no callbacks. Создать factory, принять
 * state только при успехе. Своя ссылка либо NULL; callbacks не вызываются. */
DVM_Provider *dvm_provider_create(const DVM_ProviderOps *ops, void *state);


/* EN/RU: Like create, with immutable optional capture callback; state adopted only on success.
 * fixed_root1 promises immutable dot identity only,0 leaves root capture to the callback.
 * Existing ProviderOps layout/flag values are unchanged; callback code lives through destroy.
 * Как create с optional capture callback; state принят только при успехе, ProviderOps ABI прежний. */
DVM_Provider *dvm_provider_create_capture(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture, int fixed_root);


/* EN/RU: Like create_capture, with immutable native_flags (DVM_PROVIDER_CANCEL_PATH).
 * Adopt state only on success; existing callback-table layout stays unchanged.
 * Как create_capture с immutable native_flags; state принят только при успехе. */
DVM_Provider *dvm_provider_create_native(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture, int fixed_root, unsigned native_flags);


/* EN/RU: Like create_native, with immutable optional launch callback, adopting state only on success.
 * ProviderOps ABI stays unchanged; LAUNCH requires callback/PATH. No callbacks or I/O during construction.
 * Как create_native с optional immutable launch callback, state принят только при success.
 * ProviderOps ABI прежний; LAUNCH требует callback/PATH, constructor без callbacks/I/O.
 */
DVM_Provider *dvm_provider_create_routed(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture,
    int fixed_root, unsigned native_flags, DVM_ProviderLaunch launch);


/* EN/RU: Borrow live provider/canonical relative node; owned launch address/NULL, no authority inferred from metadata.
 * Reject invalid relative path/pre-cancel/unsupported callback or scope before invoking provider.
 * Native-only outside managed admission; no source-lifetime lock during callback. Caller owns returned address
 * even when cancellation races callback return; free it or use routed submit, which handles that race.
 * Заимствовать live provider/canonical relative node; owned launch address/NULL, metadata не даёт authority.
 * Invalid path/pre-cancel/unsupported scope отказывают до callback. Native-only вне managed admission,
 * callback без lifetime lock; caller владеет address даже при racing cancellation и обязан освободить.
 */
struct DVM_LaunchAddress *dvm_provider_launch(DVM_Provider *provider, const char *relative);


/* EN/RU: Borrow retained factory; immutable native capabilities/0, no I/O or authority.
 * Заимствовать retained factory; immutable native capabilities/0, без I/O/authority. */
unsigned dvm_provider_native_flags(DVM_Provider *provider);


/* EN/RU: Does this retained provider support per-node capture? No I/O or authority grant.
 * Поддерживает ли retained provider per-node capture? Без I/O/выдачи authority. */
int dvm_provider_capturable(DVM_Provider *provider);


/* EN/RU: Test stable route or already anchored dot; no callback or I/O.
 * Проверить stable route либо уже anchored dot; без callback/I/O. */
int dvm_provider_bindable_at(DVM_Provider *provider, const char *relative);


/* EN/RU: Owned anchored root/NULL; may perform native I/O. Source stays borrowed/pinned.
 * Fixed dot may return another owner of the same provider. Otherwise callback must return
 * a fresh unshared factory. Source invalidations forward provider-wide; source remains pinned.
 * Under an explicit cancel scope, callbacks require CAPTURE and returned roots require PATH;
 * pre-cancel refuses before acquisition. Под явным scope callback требует CAPTURE,
 * root требует PATH; pre-cancel отказывает до acquisition. Свой root/NULL; native I/O. */
DVM_Provider *dvm_provider_capture(DVM_Provider *provider, const char *relative);


/* EN/RU: Retain live borrowed provider; NULL succeeds; zero on ref overflow.
 * Удержать live provider; NULL допустим; ноль при overflow. No callbacks/I/O. */
int dvm_provider_retain(DVM_Provider *provider);


/* EN/RU: Consume reference; NULL harmless. Last reference destroys native state
 * outside mutex. Поглотить ссылку; последняя очищает native state вне mutex. */
void dvm_provider_release(DVM_Provider *provider);


/* EN/RU: Borrow retained provider/path/output through blocking native metadata.
 * Заимствовать удержанный provider/path/output на время blocking native metadata. */
FVM_Result dvm_provider_info(DVM_Provider *provider, const char *relative, FVM_FileInfo *output);


/* EN/RU: Check effective caller access on retained provider/path; optional callback.
 * Missing callback returns UNSUPPORTED; never infer rights from metadata mode.
 * Проверить effective access удержанного provider/path; без callback — UNSUPPORTED.
 * Native-only blocking, no namespace locks; callback must have no side effects. */
FVM_Result dvm_provider_access(DVM_Provider *provider, const char *relative, unsigned access);


/* EN/RU: Open relative path/mode into empty output. Preallocate a lifetime wrapper
 * and retain factory before callback; success pins factory until handle close.
 * Открыть relative path/mode в пустой output; wrapper/ref до callback. Успех
 * удерживает factory до close handle. No managed access; return port status. */
FVM_Result dvm_provider_open(DVM_Provider *provider, const char *relative, const char *mode, DVM_VfsHandle *output);

/* EN/RU: Query immutable stable-bind promise on a retained provider; no callback.
 * Проверить immutable stable-bind promise удержанного provider; без callback. */
int dvm_provider_bindable(DVM_Provider *provider);

/* EN/RU: Borrow provider-scoped native event journal while holding a provider ref.
 * No authority or ownership transfer; changesretain can pin the journal independently.
 * Заимствовать journal под provider ref; changesretain удерживает его независимо. */
DVM_Changes *dvm_provider_changes(DVM_Provider *provider);
#endif
