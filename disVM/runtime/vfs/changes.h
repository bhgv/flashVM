/* Copyright (c) 2026 bhgv
 * EN: Provider-scoped change journals and independent native refresh cursors.
 * RU: Provider-scoped журналы изменений и независимые native refresh cursors.
 */
#ifndef DVM_VFS_CHANGES_H
#define DVM_VFS_CHANGES_H
#include "file_port.h"
#include "notice.h"
#include <stdint.h>
#define DVM_CHANGE_RIGHTS 1U /* Effective access policy may have changed. */
#define DVM_CHANGE_NAMESPACE 2U /* Directory/resource membership may have changed. */
#define DVM_CHANGE_CONTENT 4U /* Resource data or attributes may have changed. */
#define DVM_CHANGE_CONNECTION 8U /* Availability, draining or connection state changed. */
#define DVM_CHANGE_LOAD 16U /* Provider-side load observations need refreshing. */
#define DVM_CHANGE_NETWORK 32U /* Client-path network observations need refreshing. */
#define DVM_CHANGE_ALL 63U /* All currently defined change classes. */
#define DVM_CHANGE_KINDS 6U /* Number of independent class watermarks. */
#define DVM_CHANGE_RESOURCE 1U /* Resource ID is meaningful, including ID zero. */
#define DVM_CHANGE_HISTORY 32U /* Bounded journal; overflow conservatively broadens scope. */
typedef struct DVM_Changes DVM_Changes; /* Owned native journal; native push callbacks, no managed pointers. */


/* EN/RU: One session-local immutable event; source owns the provider routing.
 * Одно session-local immutable событие; source отвечает за routing provider. */
typedef struct DVM_ChangeEvent
{
    uint64_t session; /* Local connection epoch, assigned by the journal owner. */
    uint64_t generation; /* Strictly increasing source report generation; never wraps. */
    unsigned kinds; /* Changed classes; zero means unspecified/all. */
    unsigned flags; /* Optional RESOURCE scope, otherwise provider-wide. */
    uint64_t resource; /* Opaque provider-local identity, not a pointer or authority. */
} DVM_ChangeEvent;


/* EN/RU: Each observer owns its cursor; synchronize sharing outside this API.
 * Каждый observer владеет cursor; shared cursor синхронизировать снаружи. */
typedef struct DVM_ChangeCursor
{
    uint64_t session; /* Epoch of acknowledged state; zero means never refreshed. */
    uint64_t seen[DVM_CHANGE_KINDS]; /* Last successfully refreshed version per class. */
} DVM_ChangeCursor;


/* EN/RU: Captured refresh ticket; later notifications do not mutate this value.
 * Захваченный refresh ticket; последующие события не меняют это value. */
typedef struct DVM_ChangeBatch
{
    DVM_ChangeEvent event; /* Coalesced pending classes and conservative resource scope. */
    uint64_t versions[DVM_CHANGE_KINDS]; /* Exactly the class watermarks this query can acknowledge. */
} DVM_ChangeBatch;


/* EN/RU: Create owned journal at session/generation 1 with all classes dirty.
 * Return NULL on allocation/lock failure. Создать журнал; NULL при отказе. */
DVM_Changes *dvm_changes_create(void);


/* EN/RU: Consume journal only after all callers retire; NULL allowed.
 * Поглотить журнал после завершения callers; NULL допустим. No callbacks. */
void dvm_changes_destroy(DVM_Changes *changes);


/* EN/RU: Snapshot current epoch; zero on invalid input. No I/O/callbacks.
 * Получить current epoch; ноль при invalid input. Без I/O/callbacks. */
uint64_t dvm_changes_session(DVM_Changes *changes);


/* EN/RU: Start a new epoch only if expected matches; return new epoch or zero.
 * Resets report generations, invalidates all observers, never wraps.
 * Новая epoch только при matching expected; новая epoch либо ноль. Без wrap. */
uint64_t dvm_changes_reconnect(DVM_Changes *changes, uint64_t expected);


/* EN/RU: Publish borrowed event; 1 accepted, 0 old/duplicate, -1 invalid/future epoch.
 * Gaps broaden to provider-wide ALL. Native push callbacks run after journal unlock;
 * recipients may allocate inbox messages. No managed callbacks or provider I/O.
 * Публиковать event; 1 accepted, 0 old/duplicate, -1 invalid. Gap расширяет до ALL. */
int dvm_changes_notify(DVM_Changes *changes, const DVM_ChangeEvent *event);


/* EN/RU: Capture pending selected classes without acknowledging; 1 dirty, 0 clean,
 * -1 invalid. Output changes on nonnegative result only; cursor stays borrowed.
 * Захватить pending classes без ack; 1 dirty, 0 clean, -1 invalid. No I/O. */
int dvm_changes_pending(DVM_Changes *changes, const DVM_ChangeCursor *cursor,
                         unsigned kinds, DVM_ChangeBatch *output);


/* EN/RU: Read current provider state for the captured classes/scope outside journal
 * locks. Borrow ticket/argument; stage cache data and return port status. Failure
 * must not publish a successful cache. Caller publishes staged data only after
 * refresh returns OK, retaining its session/version tag; no atomic cache is implied.
 * No VM callbacks while outside admission.
 * Перечитать state по ticket вне journal lock; inputs borrowed, вернуть status.
 * При отказе не публиковать successful cache; managed callbacks запрещены. */
typedef FVM_Result (*DVM_ChangeRefresh)(const DVM_ChangeBatch *ticket, void *argument);


/* EN/RU: Acknowledge an exact ticket captured from this same journal, after the
 * caller successfully refreshed state. No I/O. Stale session returns NOT_FOUND;
 * invalid/future versions return INVALID_ARGUMENT without changing cursor.
 * Подтвердить exact ticket этого journal после successful refresh. Без I/O;
 * stale session — NOT_FOUND, invalid versions — INVALID_ARGUMENT, без ack. */
FVM_Result dvm_changes_ack(DVM_Changes *changes, DVM_ChangeCursor *cursor, const DVM_ChangeBatch *ticket);


/* EN/RU: Query once only when dirty, then acknowledge captured versions on success.
 * New changes remain pending; epoch replacement returns NOT_FOUND without ack.
 * Failed callback leaves cursor unchanged. No retry or permission grant.
 * Один query при dirty; ack только captured versions. Новые события остаются;
 * новая epoch — NOT_FOUND без ack. Callback failure не меняет cursor. */
FVM_Result dvm_changes_refresh(DVM_Changes *changes, DVM_ChangeCursor *cursor,
                               unsigned kinds, DVM_ChangeRefresh refresh, void *argument);

/* EN/RU: Borrow push hub while retaining the journal's source owner.
 * Заимствовать push hub при удержанном source owner. No guest authority. */
DVM_NoticeHub *dvm_changes_notices(DVM_Changes *changes);
#endif
