/* Copyright (c) 2026 bhgv
 * EN: Bounded native proxy ownership and admission; these are private host types.
 * RU: Ограниченное владение proxy и admission; это приватные типы host, не wire ABI.
 */
#ifndef DVM_REMOTE_SESSION_H
#define DVM_REMOTE_SESSION_H
#include <stddef.h>
#include <stdint.h>

#define DVM_REMOTE_REFS 8U      /* Maximum borrowed ref arguments per admitted call. */
#define DVM_REMOTE_PAYLOAD 256U /* Maximum bytes retained for exact replay comparison. */


/* EN/RU: An opaque, individually locked session; no VM pointers cross its boundary.
 * Непрозрачная сессия со своим lock; VM pointers через её границу не передаются. */
typedef struct DvmRemoteSession DvmRemoteSession;


/* EN/RU: Local statuses; none assigns a public DIS64 or transport identifier.
 * Локальные статусы; они не назначают публичные DIS64/transport identifiers. */
typedef enum
{
    DVM_REMOTE_OK,        /* Operation committed, including completed ownership changes. */
    DVM_REMOTE_DUPLICATE, /* Exact request already admitted; never execute it again. */
    DVM_REMOTE_CLOSED,    /* Session closed or lease expired; no new admission. */
    DVM_REMOTE_INVALID,   /* Invalid arguments, wrong object kind or handle. */
    DVM_REMOTE_CAPACITY,  /* Bounded object/request/pin capacity exhausted. */
    DVM_REMOTE_STALE,     /* Retired object generation or request outside replay window. */
    DVM_REMOTE_CONFLICT,  /* Same request ID with different operation/arguments. */
    DVM_REMOTE_BUSY       /* Active operation or uncollected owner roots remain. */
} DvmRemoteStatus;


/* EN/RU: Distinguish a variable owner from a function owner, without wire IDs.
 * Различать владельца переменной и функции, не вводя wire IDs. */
typedef enum
{
    DVM_REMOTE_VARIABLE, /* Get/Set target; storage remains with its owner. */
    DVM_REMOTE_FUNCTION  /* Invoke target; closure remains with its owner. */
} DvmRemoteKind;


/* EN/RU: Checked process-local identity; authentication belongs to the transport.
 * Проверяемая локальная identity; authentication реализуется transport-слоем. */
typedef struct
{
    uint64_t session;    /* Unique nonzero session identity, allocated by this process. */
    uint64_t generation; /* Never wraps or repeats within an object slot. */
    size_t slot;         /* Bounded object table index, never an address. */
} DvmRemoteHandle;


/* EN/RU: A copied bounded request; refs preserve alias identities and get pins.
 * Ограниченная копия запроса; refs сохраняют aliases и получают pins. */
typedef struct
{
    uint64_t id;                               /* Nonzero correlation ID; one execution per session. */
    DvmRemoteHandle target;                    /* Variable/function owner to keep alive. */
    DvmRemoteKind kind;                        /* Required target kind. */
    uint32_t operation;                        /* Local adapter operation, not a wire opcode. */
    size_t ref_count;                          /* Number of live variable references. */
    DvmRemoteHandle refs[DVM_REMOTE_REFS];     /* Borrowed variable handles, copied on admission. */
    size_t payload_size;                       /* Exact significant bytes in payload. */
    unsigned char payload[DVM_REMOTE_PAYLOAD]; /* Snapshot bytes, never managed pointers. */
} DvmRemoteRequest;


/* [GC:] EN/RU: Owner-only root token returned after all peer references and pins end.
 * Owner-only токен корня, возвращаемый после завершения peer refs и pins. */
typedef struct
{
    void *token;        /* Opaque owner root registration, never dereferenced here. */
    DvmRemoteKind kind; /* Adapter needed to unregister the root on its owner thread. */
} DvmRemoteRoot;


/* EN: Allocate bounded native storage; limits must be nonzero, expiry is absolute
 * monotonic time in caller units. Return NULL on allocation/identity exhaustion.
 * RU: Выделить ограниченную память; limits ненулевые, expiry — абсолютное monotonic
 * время в единицах caller. NULL при OOM/исчерпании identity. No managed allocation. */
DvmRemoteSession *dvm_remote_session_new(size_t objects, size_t requests, uint64_t expiry);


/* EN: Destroy only after callers join, all roots are collected and calls finish.
 * BUSY leaves session intact. No GC callback or blocking owner handoff occurs.
 * RU: Уничтожить после join callers, сбора корней и завершения calls. BUSY сохраняет
 * сессию. GC callback и блокирующий owner handoff не выполняются. */
DvmRemoteStatus dvm_remote_session_destroy(DvmRemoteSession *session);


/* EN: Advance monotonic time, optionally renew to expiry > now (zero means no
 * renewal). An expired/closed session cannot revive; backward time is INVALID.
 * RU: Продвинуть monotonic время; expiry > now продлевает lease, ноль не продлевает.
 * Закрытая/истёкшая сессия не оживает; время назад даёт INVALID. Thread safe. */
DvmRemoteStatus dvm_remote_session_tick(DvmRemoteSession *session, uint64_t now, uint64_t expiry);


/* EN: Revoke all peer ownership and future admissions; admitted calls keep pins
 * until finish. Idempotent, thread safe, does not claim admitted effects cancelled.
 * RU: Отозвать peer refs и новые admissions; calls держат pins до finish.
 * Идемпотентно, thread safe; принятые эффекты не объявляются отменёнными. */
void dvm_remote_session_close(DvmRemoteSession *session);


/* EN: Transfer one pre-registered owner root on success only; token is non-NULL,
 * kind checked, output untouched on failure. The owner must later collect token.
 * RU: Передать заранее зарегистрированный owner root только при успехе; token
 * ненулевой, kind проверен, output при отказе не меняется. Затем owner collect. */
DvmRemoteStatus dvm_remote_publish(DvmRemoteSession *session, DvmRemoteKind kind, void *token, DvmRemoteHandle *output);


/* EN: Add/remove one peer reference to a checked handle. Release cannot remove
 * admission pins; duplicate network messages must pass replay admission first.
 * RU: Добавить/снять peer ref проверенного handle. Release не снимает admission
 * pins; сетевые повторы сначала проходят replay admission. Thread safe. */
DvmRemoteStatus dvm_remote_retain(DvmRemoteSession *session, DvmRemoteHandle handle);
DvmRemoteStatus dvm_remote_release(DvmRemoteSession *session, DvmRemoteHandle handle);


/* EN: Atomically copy/validate request and pin target/all ref variables. OK alone
 * authorizes execution. DUPLICATE reports active/completed through done. Exact
 * bytes, ref order and aliases are compared. Older IDs outside the bounded window
 * are STALE, never retried. Caller retains its own result cache until response ack.
 * RU: Атомарно проверить/скопировать request и pin target/refs. Только OK разрешает
 * исполнение. DUPLICATE сообщает active/completed через done. Байты и aliases
 * сравниваются точно. Старые IDs вне окна — STALE, без replay. Result cache у caller. */
DvmRemoteStatus dvm_remote_admit(DvmRemoteSession *session, const DvmRemoteRequest *request, int *done);


/* EN: Complete one admitted id exactly once and release all pins, even after
 * close. No managed result/root publication is performed. Unknown/double finish
 * returns STALE; owner effect and result must be committed before this call.
 * RU: Завершить принятый id один раз, снять pins даже после close. Публикации
 * managed result/root здесь нет. Неизвестный/повторный finish — STALE. Thread safe. */
DvmRemoteStatus dvm_remote_finish(DvmRemoteSession *session, uint64_t id);


/* EN: Owner thread claims one root with zero peer refs and pins. OK transfers
 * token back; STALE means none ready. Owner unregisters it outside session lock.
 * RU: Owner thread забирает root с нулевыми refs/pins. OK возвращает token;
 * STALE — готовых нет. Owner снимает регистрацию вне session lock. */
DvmRemoteStatus dvm_remote_collect(DvmRemoteSession *session, DvmRemoteRoot *output);
#endif
