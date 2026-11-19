/* Copyright (c) 2026 bhgv
 * EN: Private Inferno-side client state; never include VFS/VM SDK type definitions.
 * RU: Private Inferno-side client state; без VFS/VM SDK type definitions.
 */
#ifndef STYX_CLIENT_INTERNAL_H
#define STYX_CLIENT_INTERNAL_H
#include <lib9.h>
#include <styx.h>
#include <pthread.h>
#include "client.h"
#include "mux.h"
#include "life.h"

/* EN/RU: One connection owns codecs/buffers and serializes bounded native RPCs.
 * Одно соединение владеет buffers и сериализует bounded native RPCs. */
struct StyxClient
{
    StyxLife *life;      /* Owned sticky terminal source; no provider/guest pointers. */
    int terminal;        /* First legacy terminal reason, serialized by client lock. */
    StyxMux *mux;        /* Optional independent native owner; no Client pointers retained by it. */
    int transportclosed; /* Legacy transport close already consumed by explicit shutdown. */
    int cycle;           /* Current acknowledged traversal-cycle annotation, copied before cleanup RPCs. */
    int cancelled;       /* Current returned RPC cancelled, under client lock. */
    int bidir;           /* Immutable native carrier bidirectional-session capability; not peer authority/liveness. */
    unsigned error_profile;  /* Host-selected immutable lookup error vocabulary, default opaque. */
    pthread_mutex_t lock;    /* Protects refs/fids/reply copying; legacy RPC stays serialized. */
    StyxTransport transport; /* Legacy adopted stream; unused with native fd owner. */
    uchar *send;             /* Owned bounded outgoing frame. */
    uchar *receive;          /* Owned bounded incoming frame, decoded pointers borrow it. */
    unsigned msize;          /* Negotiated complete-message bound. */
    u32int lastfid;          /* Monotonic remote fid allocator; never wraps/reuses. */
    size_t references;       /* Owner plus separately owned fids. */
    Qid root;                /* Attached root identity/type. */
    int failed;              /* Fatal framing/transport failure; no automatic reconnection. */
    int attempted;           /* Last RPC attempted stream write; errors may have effects. */
};

/* EN/RU: One independently owned remote object retains its client and data ceiling.
 * Один independent remote object удерживает client и data ceiling. */
struct StyxFid
{
    StyxClient *client;            /* Retained connection until clunk. */
    char walk_path[STYX_NAME_MAX]; /* Canonical relative root path for traversal identity, never reopened for I/O. */
    u32int number;                 /* Independent remote fid, never keyed by Qid. */
    Qid qid;                       /* Last walked/opened remote identity. */
    unsigned unit;                 /* Maximum read/write payload, negotiated and Ropen-limited. */
    int opened;                    /* Topen succeeded. */
    int mode;                      /* Fixed read/write ceiling0/1/2. */
    int poisoned;                  /* Unknown open/data effects forbid later open/data calls. */
};

/* EN/RU: Under client lock, exchange one frame;1 expected reply,0 Rerror,-1 fatal.
 * Returned pointers borrow receive until the next RPC. No retry of failed effects.
 * Под lock обменять frame;1 reply,0 Rerror,-1 fatal; pointers заимствованы. */


/* [standalone-contract:styxrpc]
 * EN: styxrpc: native client session operations and lifetime.
 * RU: styxrpc: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpc(StyxClient *client, Fcall *request, Fcall *reply);


/* [standalone-contract:styxrpccancel]
 * EN: styxrpccancel: native client session operations and lifetime.
 * RU: styxrpccancel: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpccancel(StyxClient *, Fcall *, Fcall *, StyxCancel *);
/* EN/RU: Borrow client under lock for untimed native read; cancel/terminal still retires it.
 * Под client lock выполнить untimed read; cancel/terminal завершают его. */


/* [standalone-contract:styxrpcwatch]
 * EN: styxrpcwatch: native client session operations and lifetime.
 * RU: styxrpcwatch: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * request (Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reply (Fcall *reply): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrpcwatch(StyxClient *, Fcall *, Fcall *, StyxCancel *);


/* [standalone-contract:styxrpcbad]
 * EN: styxrpcbad: native client session operations and lifetime.
 * RU: styxrpcbad: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxrpcbad(StyxClient *); /* Client lock held; terminal failure wakes all multiplexed work. */


/* [standalone-contract:styxclientstart]
 * EN: styxclientstart: native client session operations and lifetime.
 * RU: styxclientstart: нативные client-операции session и время жизни.
 * transport (const StyxTransport *transport): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: StyxClient *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientstart(const StyxTransport *, StyxMux *, unsigned, const char *, const char *);


/* EN/RU: Borrow pinned client under its lock; publish first legacy reason outside that lock.
 * Temporarily unlocks/relocks client; failed exchange has no usable borrowed reply.
 * Под client lock опубликовать первый legacy reason вне lock; client удержан caller. */


/* [standalone-contract:styxclientemit]
 * EN: styxclientemit: native client session operations and lifetime.
 * RU: styxclientemit: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * reason (int reason): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientemit(StyxClient *client, int reason);


/* EN/RU: Under client lock, consume remote fid when the stream remains usable.
 * Под lock поглотить remote fid, если stream ещё пригоден. */


/* [standalone-contract:styxclunknumber]
 * EN: styxclunknumber: native client session operations and lifetime.
 * RU: styxclunknumber: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * number (u32int number): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclunknumber(StyxClient *client, u32int number);


/* EN/RU: Clear borrowed outcome before validation; count remains zero for control RPCs.
 * Очистить borrowed outcome до validation; count ноль для control RPC. */


/* [standalone-contract:styxopclear]
 * EN: styxopclear: native client operation operations and lifetime.
 * RU: styxopclear: нативные client-операции operation и время жизни.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxopclear(StyxTransfer *result);


/* EN/RU: Copy latest RPC outcome while client lock is held; status1/0/-1.
 * Unacknowledged sent requests are uncertain, never inferred successful or replayed.
 * Копировать latest outcome под client lock; sent без acknowledgement — uncertain. */


/* [standalone-contract:styxopresult]
 * EN: styxopresult: native client operation operations and lifetime.
 * RU: styxopresult: нативные client-операции operation и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * status (int status): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxopresult(StyxClient *client, int status, StyxTransfer *result);


/* EN/RU: Schedule best-effort clunk of an abandoned number; client lock held.
 * Worker owns only client/number. Setup failure fails the connection without RPC wait.
 * Отложить best-effort clunk abandoned number; setup failure завершает connection. */


/* [standalone-contract:styxforgetlocked]
 * EN: styxforgetlocked: native client forget operations and lifetime.
 * RU: styxforgetlocked: нативные client-операции forget и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * number (u32int number): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxforgetlocked(StyxClient *client, u32int number);

#endif
