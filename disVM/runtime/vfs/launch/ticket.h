/* Copyright (c) 2026 bhgv
 * EN: Separated requester/provider launch capabilities and one-use pre-init environment handoff.
 * RU: Раздельные requester/provider launch capabilities и одноразовая передача окружения до init.
 */
#ifndef DVM_LAUNCH_TICKET_H
#define DVM_LAUNCH_TICKET_H
#include "prepare.h"
#include "terminal.h"
#include "contract.h"
#include "fd_path.h"
#include "../changes.h"
#define DVM_LAUNCH_ARGUMENT_LIMIT 65536U /* Per-request copied startup bytes, no wire format assigned. */
typedef struct DVM_LaunchTicket DVM_LaunchTicket; /* Requester status/cancel authority only. */
typedef struct DVM_LaunchRequest DVM_LaunchRequest; /* Provider preparation/take authority. */


/* EN/RU: States describe preparation/ownership, never evidence that code or init executed.
 * States описывают подготовку/владение, не выполнение кода/init. */
typedef enum DVM_LaunchState
{
    DVM_LAUNCH_INVALID = -1, /* Invalid capability. */
    DVM_LAUNCH_PENDING = 0, /* Await provider decision. */
    DVM_LAUNCH_PREPARING, /* Native root acquisition admitted once. */
    DVM_LAUNCH_READY, /* Private environment available to provider. */
    DVM_LAUNCH_TAKEN, /* Provider consumed one environment; execution is separate. */
    DVM_LAUNCH_CANCELLED, /* Requester/outer native cancellation won before ready/take. */
    DVM_LAUNCH_DENIED /* Provider refusal, abandonment or acquisition failure. */
} DVM_LaunchState;


/* EN/RU: Borrowed inputs through create only. Trusted adapter associates identities with grants;
 * strings are not credentials. Module is a logical name, never implicitly resolved on the host.
 * Inputs borrowed до create; trusted adapter связывает identities/grants, строки не credentials. */
typedef struct DVM_LaunchSpec
{
    unsigned policy; /* Explicit terminal-first policy. */
    DVM_Provider *terminal; /* Granted PATH/CAPTURE-capable terminal root, retained on success. */
    DVM_Origin terminal_origin; /* Copied terminal source/launch association. */
    char expected_provider[DVM_ORIGIN_ID_BYTES]; /* Expected execution installation, not an authorization token. */
    char module[DVM_NS_PATH_LIMIT]; /* Copied nonempty terminated logical module name. */
    const void *arguments; /* Copied bounded startup bytes, never native/managed pointer authority. */
    size_t argument_bytes; /* 0..DVM_LAUNCH_ARGUMENT_LIMIT. */
    const DVM_TerminalChannels *flashvm; /* Optional borrowed channel grant, copied/retained at creation. */
    const DVM_LaunchContract *contract; /* Optional borrowed immutable requirements, cloned at request creation. */
} DVM_LaunchSpec;


/* EN/RU: Immutable copied request description, without root/provider/VM pointers.
 * Immutable copied description без root/provider/VM pointers. */
typedef struct DVM_LaunchDescription
{
    unsigned policy; /* Requested initial union order. */
    DVM_Origin terminal_origin; /* Terminal association captured at request. */
    char expected_provider[DVM_ORIGIN_ID_BYTES]; /* Expected provider installation identity. */
    char module[DVM_NS_PATH_LIMIT]; /* Logical module label, not a host pathname lookup. */
    size_t argument_bytes; /* Exact immutable startup byte count. */
    unsigned terminal_channels; /* One when a separate FlashVM channel grant accompanies this request. */
    unsigned typed; /* One when owner interface validation is required before preparation; not a wire field. */
} DVM_LaunchDescription;


/* EN/RU: Exclusive owned handoff; do not copy. Consumer may move group by clearing this field.
 * No code executes on creation/take. Free remaining owners with dvm_launch_handoff_free.
 * Exclusive handoff не копировать; consumer переносит group, обнуляя поле; код не запускается. */
typedef struct DVM_LaunchHandoff
{
    unsigned local_role; /* Native host sets caller-local TERMINAL/PROVIDER class; zero keeps provider startup default. */
    DVM_Namespace *group; /* Owned isolated environment, survives ticket/request retirement. */
    DVM_LaunchDescription description; /* Owned copied immutable request metadata. */
    DVM_Origin provider_origin; /* Accepted provider association, same launch session. */
    unsigned char *arguments; /* Owned independent copy of startup bytes. */
    DVM_TerminalChannels flashvm; /* Owned channel root, moved once at take; provider NULL when absent. */
    struct UnibusBinding *unibus; /* Optional owned native per-launch handler, never a serialized frame. */
    DVM_LaunchContract *contract; /* Owned independent copied requirements, mandatory when description.typed is one. */
    unsigned char inherited_fds[DVM_LAUNCH_FD_COUNT]; /* Trusted native mode ceilings; zero means no import. */
} DVM_LaunchHandoff;


/* EN: Replace the terminal VFS root of an exclusive prepared handoff. Borrow the
 * new terminal provider through return. Preserve accepted execution authority,
 * union order and terminal channel grant. Native capture may block. Returns
 * 1 on replacement, 0 with the original handoff intact.
 * RU: Заменить терминальный корень эксклюзивного handoff. Новый provider
 * заимствован до возврата. Полномочия исполнения, порядок union и транспортный
 * grant сохраняются. Захват может блокировать. 1 при замене, иначе handoff цел. */
int dvm_launch_handoff_project_terminal(DVM_LaunchHandoff *handoff, DVM_Provider *terminal);


/* EN/RU: Retain a native Unibus handler in an exclusive live handoff, 1/0; no claim/guest execution.
 * Attach only once before adoption; caller keeps its reference. / Удержать native handler до adoption;
 * только один раз, 1/0; caller сохраняет свой ref, без claim/guest-кода.
 */
int dvm_launch_handoff_unibus(DVM_LaunchHandoff *handoff, struct UnibusBinding *binding);


/* EN/RU: Create requester ticket and provider request; output initially NULL, both owned on success.
 * No I/O/authentication/guest execution; unsupported roots/invalid/OOM returnNULL with outputNULL.
 * Caller routes request only through authenticated owner admission and enforces aggregate quotas.
 * Создать две owned capabilities без I/O/auth/init; failure даётNULL/outputNULL,
 * trusted adapter обеспечивает admission и суммарные quotas. */
DVM_LaunchTicket *dvm_launch_request_create(const DVM_LaunchSpec *spec, DVM_LaunchRequest **request);


/* EN/RU: Clone copied requirements for native owner inspection; owned snapshot/NULL for untyped/invalid/OOM.
 * Borrow request through return; description.typed distinguishes untyped from a failed typed clone.
 * No state change, authorization, I/O or code execution; snapshot outlives request/ticket after success.
 * Копировать requirements для native owner inspection; свой snapshot/NULL при untyped/invalid/OOM.
 * Request borrowed до return; description.typed отличает untyped от failed clone. Без state change/I/O/execution.
 */
DVM_LaunchContract *dvm_launch_request_contract(DVM_LaunchRequest *request);


/* EN/RU: Owner checks actual immutable exports against copied required names/signatures;1 accepted/0 mismatch or not PENDING.
 * Borrow request/exports through return; no ownership transfer, code execution, provider I/O or prepare on this call.
 * A mismatch leaves PENDING for explicit refusal/review. Typed requests cannot prepare until a match is accepted.
 * Current/request cancellation refuses; authorization and choosing the actual implementation remain the owner's responsibility.
 * Owner сверяет actual immutable exports с copied requirements;1/0 при mismatch либо не PENDING.
 * Inputs borrowed; без передачи ownership, code execution, provider I/O/prepare. Mismatch сохраняет PENDING.
 * Typed request не подготавливается до match; cancellation отказывает, actual implementation/authorization выбирает owner.
 */
int dvm_launch_request_check(DVM_LaunchRequest *request, const DVM_LaunchContract *exports);


/* EN/RU: Borrow live ticket; copied state, no wait or I/O; INVALID for NULL.
 * Заимствовать live ticket; copied state без ожидания/I/O, NULL — INVALID. */
DVM_LaunchState dvm_launch_ticket_status(DVM_LaunchTicket *ticket);


/* EN/RU: Copy immutable description from borrowed live request into out;1 success,0 invalid.
 * No root pointers or I/O. Скопировать description из live request в out;1/0,
 * без root pointers и I/O. */
int dvm_launch_describe(DVM_LaunchRequest *request, DVM_LaunchDescription *out);


/* EN/RU: Copy a bounded immutable argument slice; request/bytes/copied borrowed,1/0.
 * Zero capacity acceptsNULL bytes, offset beyond end refuses without copying.
 * Скопировать bounded slice, request/bytes/copied borrowed;1/0, capacity0 допускаетNULL. */
int dvm_launch_arguments(DVM_LaunchRequest *request, size_t offset, void *bytes, size_t capacity, size_t *copied);


/* EN/RU: Return owned independent change journal/NULL; use native notices, not target polling.
 * Events invalidate assumptions; read ticket state for authority, no full transition history promised.
 * Вернуть owned journal/NULL; native notices без target polling, события — hints, не authority. */
DVM_Changes *dvm_launch_observe(DVM_LaunchTicket *ticket);


/* EN/RU: Borrow provider request/root/origin through native preparation outside locks.
 * Typed requests require an accepted request_check first; unvalidated requests remain PENDING without provider I/O.
 * Require cancellable PATH/CAPTURE routes; forward current outer native cancellation via native-only state.
 * Return1 only if READY published,0 refusal/cancel/race; one preparation, no automatic retry.
 * Typed request требует принятого request_check; иначе PENDING без provider I/O.
 * Заимствовать request/root/origin для native подготовки вне locks; capable routes,
 * outer cancellation передаётся native state;1 READY/0 refusal, без replay. */
int dvm_launch_request_prepare(DVM_LaunchRequest *request, DVM_Provider *provider, const DVM_Origin *origin);


/* EN/RU: Provider-only one-use take; owned independent handoff/NULL, OOM leaves READY.
 * No init/process/network action. Provider-only take возвращает owned handoff/NULL;
 * OOM сохраняет READY, никакого init/process/network action. */
DVM_LaunchHandoff *dvm_launch_request_take(DVM_LaunchRequest *request);


/* EN/RU: Cancel/refuse before TAKEN;1 changed,0 terminal/NULL. Native cancellation and cleanup
 * run outside state mutex. Cancellation cannot revoke an already transferred environment.
 * Cancel/refuse до TAKEN;1/0, signals/cleanup вне mutex; transferred environment не отзывается. */
int dvm_launch_ticket_cancel(DVM_LaunchTicket *ticket);


/* EN/RU: Provider refuses Pending/Preparing/Ready;1 changed/0 terminal orNULL.
 * Signals and prepared-group disposal run outside shared mutex; TAKEN is preserved.
 * Provider отклоняет Pending/Preparing/Ready;1/0, signals/cleanup вне mutex, TAKEN сохранён. */
int dvm_launch_request_refuse(DVM_LaunchRequest *request);


/* EN/RU: Consume exclusive requester ticket after its callers join; NULL allowed.
 * Unfinished work is cancelled, TAKEN remains. Same-wrapper use/free must not overlap;
 * opposite wrappers may operate concurrently. Поглотить ticket после join callers;
 * unfinished work отменяется, TAKEN сохранён; use/free одного wrapper не совмещаются. */
void dvm_launch_ticket_free(DVM_LaunchTicket *ticket);


/* EN/RU: Consume exclusive provider wrapper after its callers join; refuse unfinished work.
 * NULL allowed, transferred environments survive. Поглотить provider wrapper после join,
 * unfinished work отклоняется; NULL допустим, transferred environment сохраняется. */
void dvm_launch_request_free(DVM_LaunchRequest *request);


/* EN/RU: Consume exclusive handoff, freeing remaining namespace, channel pin and bytes; NULL allowed.
 * Consumer clears group/flashvm.provider after explicitly moving their ownership to an instance.
 * Поглотить handoff и оставшиеся group/channel pin/bytes; после move owned fields обнуляются. */
void dvm_launch_handoff_free(DVM_LaunchHandoff *handoff);
#endif
