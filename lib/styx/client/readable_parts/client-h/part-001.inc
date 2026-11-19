/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of client.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
#include "walk.h"
#include "file.h"
#include "phy.h"

typedef struct StyxClient StyxClient; /* Owned connection reference. */
typedef struct StyxCancel StyxCancel; /* Owned sticky native cancellation capability. */
typedef struct StyxFid StyxFid;       /* Owned independent remote fid and connection pin. */

/* EN/RU: Native stream callbacks; positive bounded counts,0 EOF,-1 error.
 * Host supplies bounded I/O/deadline and trusted connection; no guest callbacks.
 * close consumes state once without waiting for protocol or external work. Code/state live until close. No implicit authentication.
 * Native stream callbacks; host задаёт bounded I/O и доверенное соединение. */
typedef struct StyxTransport
{
    void *state;                                 /* Owned stream state, no VM pointers. */
    long (*read)(void *, void *, size_t);        /* Borrow bytes for one native receive. */
    long (*write)(void *, const void *, size_t); /* Borrow bytes for one native send. */
    void (*close)(void *);                       /* Consuming stream teardown. */
    int (*begin)(void *); /* Establish one total RPC I/O deadline;0 refuses before wire effects. */
} StyxTransport;

/* EN/RU: Consume a valid transport on every outcome; negotiate256..65536 bytes and
 * attach NOFID with copied host-supplied identity hints. Invalid callback struct
 * is not adopted. Caller serializes creation; returned connection is owned/NULL.
 * Принять valid transport всегда; negotiate+attach, свой client/NULL. */


/* [standalone-contract:styxclientmount]
 * EN: styxclientmount: native client session operations and lifetime.
 * RU: styxclientmount: нативные client-операции session и время жизни.
 * transport (const StyxTransport *transport): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: StyxClient *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);


/* EN/RU: Consume connected POSIX stream fd always; independent native owner supports
 * up to limit1..128 outstanding RPCs. timeout1..120000ms per request, then at most
 * one extra timeout interval for flush. No transport authentication is implied.
 * Принять connected POSIX fd; отдельный native owner, bounded RPC/flush. */


/* [standalone-contract:styxclientmountfd]
 * EN: styxclientmountfd: native client fd operations and lifetime.
 * RU: styxclientmountfd: нативные client-операции fd и время жизни.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: StyxClient *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *
styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);


/* EN/RU: Consume owned connected reliable ordered byte-stream PHY on every outcome; owned client/NULL.
 * Uses the same native multiplex/heartbeat/cancel engine as mountfd, with no fd requirement.
 * Ordinary single 9P session: limit bounds outstanding RPCs, never enables P9MX/reverse sessions.
 * Driver callbacks remain native-only; stream adaptations must precede this admission boundary.
 * Принять connected reliable ordered stream PHY всегда; свой client/NULL, общий native engine,
 * без fd requirement; limit ограничивает RPC одной сессии, не включает P9MX.
 * Packet/reliability adaptations должны предшествовать этому уровню. */


/* [standalone-contract:styxclientmountphy]
 * EN: styxclientmountphy: native client phy operations and lifetime.
 * RU: styxclientmountphy: нативные client-операции phy и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: StyxClient *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientmountphy(
    RemotePhy *phy, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);


/* EN/RU: Explicit negotiated S-record history profile around unchanged Styx frames; consumes PHY always.
 * Both endpoints must enable the profile. tag is a nonzero direction-local assignment in tag0 controls.
 * No raw fallback/dial/authentication; active history must have exportable VM identities.
 * Явный S-record history profile, PHY принят всегда; обе стороны включают profile, tag согласуется через tag0.
 */


/* [standalone-contract:styxclientwalkmountphy]
 * EN: styxclientwalkmountphy: native client traversal operations and lifetime.
 * RU: styxclientwalkmountphy: нативные client-операции traversal и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * msize (unsigned msize): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * user (const char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * Return: StyxClient *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientwalkmountphy(RemotePhy *phy,
                                   unsigned msize,
                                   const char *user,
                                   const char *name,
                                   unsigned timeout,
                                   unsigned limit,
                                   unsigned long tag);


/* EN/RU: Copy acknowledged fixed server projection identity,1/0; no I/O or new authority, inputs borrowed.
 * Скопировать acknowledged server projection identity,1/0; без I/O/new authority, inputs borrowed.
 */


/* [standalone-contract:styxclientwalktarget]
 * EN: styxclientwalktarget: native client traversal operations and lifetime.
 * RU: styxclientwalktarget: нативные client-операции traversal и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * root (WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientwalktarget(StyxClient *client, WalkEntity *root);


/* EN/RU: Probe active destination state before crossing a negotiated stream, without pushing it or doing I/O.
 * Raw profile returns WALK_OK. base/path identify the retained root, operation/detail follow walk.h; WALK_* result.
 * Проверить active destination до negotiated stream, без push/I/O; raw profile WALK_OK. base/path — retained root.
 */


/* [standalone-contract:styxclientwalkprobe]
 * EN: styxclientwalkprobe: native client traversal operations and lifetime.
 * RU: styxclientwalkprobe: нативные client-операции traversal и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * operation (unsigned operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * detail (const char *detail): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: WALK_INVALID, WALK_OK. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: WALK_INVALID, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientwalkprobe(
    StyxClient *client, StyxFid *base, const char *relative, unsigned operation, const char *detail);


/* EN/RU: Borrow live client;1 explicitly bidir-session carrier/0 ordinary or NULL.
 * This immutable local capability is neither reverse-server liveness nor access authority.
 * Borrow live client;1 bidir-session carrier/0 ordinary/NULL, не liveness/authority. */


/* [standalone-contract:styxclientbidir]
 * EN: styxclientbidir: native client phy operations and lifetime.
 * RU: styxclientbidir: нативные client-операции phy и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientbidir(StyxClient *client);


#define STYX_ERRORS_OPAQUE 0U  /* Default: no remote text implies absence. */
#define STYX_ERRORS_INFERNO 1U /* Explicit exact Inferno Enonexist vocabulary for lookup only. */


/* EN/RU: Select known error vocabulary before sharing client or allocating walk fids;1/0.
 * No I/O/authentication, unknown profile/late change refuses. Default is OPAQUE.
 * Host attests server vocabulary, not access rights; partial walks never imply absence.
 * Выбрать словарь ошибок до sharing/walk;1/0, без I/O/auth. Default OPAQUE;
 * unknown/late change refused, partial walk не означает absence. */


/* [standalone-contract:styxclienterrorprofile]
 * EN: styxclienterrorprofile: native client walk operations and lifetime.
 * RU: styxclienterrorprofile: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * profile (unsigned profile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclienterrorprofile(StyxClient *client, unsigned profile);


/* EN/RU: Root-relative lookup with per-call copied absence, owned fid/NULL; missing required.
 * Borrow client/path/cancel through return. Missing1 only exact acknowledged Inferno
 * Enonexist under explicit profile; all other failures leave missing0. Inferno lookup
 * walks one component per RPC so a partial Rwalk cannot erase the failure reason.
 * Native PHY or legacy supported; non-NULL cancel requires PHY owner. No data retry.
 * Root-relative lookup, свой fid/NULL, required missing;1 только explicit Inferno
 * Enonexist, прочие ошибки0. Inputs borrowed, по одному компоненту под Inferno profile. */


/* [standalone-contract:styxclientlookup]
 * EN: styxclientlookup: native client walk operations and lifetime.
 * RU: styxclientlookup: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * path (const char *path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * missing (int *missing): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientlookup(StyxClient *client, const char *path, StyxCancel *cancel, int *missing);


/* EN/RU: Same lookup from caller-pinned unopened base; serialize its open/clunk through return.
 * Тот же lookup от pinned unopened base; open/clunk сериализованы до return. */


/* [standalone-contract:styxclientlookupfrom]
 * EN: styxclientlookupfrom: native client walk operations and lifetime.
 * RU: styxclientlookupfrom: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * path (const char *path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * missing (int *missing): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing);


/* EN/RU: Configure carrier-owner heartbeat without protocol wait;1/0. All zeros disables.
 * idle/reply1..120000ms, misses2..16; NULL/legacy/terminal/invalid refuses unchanged.
 * One probe retains its tags/deadline through reconfiguration or disabling.
 * Настроить carrier-owner heartbeat без ожидания;1/0, три нуля отключают будущие probes. */


/* [standalone-contract:styxclientkeepalive]
 * EN: styxclientkeepalive: native client fd operations and lifetime.
 * RU: styxclientkeepalive: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * idle (unsigned idle): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * reply (unsigned reply): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * misses (unsigned misses): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);


/* EN/RU: Retain borrowed live client;1/0 overflow. Drop with clientfree.
 * Удержать borrowed live client;1/0 overflow, release через clientfree. */


/* [standalone-contract:styxclientretain]
 * EN: styxclientretain: native client fd operations and lifetime.
 * RU: styxclientretain: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientretain(StyxClient *client);


/* EN/RU: Explicit shutdown on native I/O worker, may wait for native owner retirement.
 * No guest execution/pause; final clientfree itself remains nonwaiting.
 * Явный native shutdown может ждать owner retirement; clientfree не ждёт. */


/* [standalone-contract:styxclientshutdown]
 * EN: styxclientshutdown: native client fd operations and lifetime.
 * RU: styxclientshutdown: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientshutdown(StyxClient *client);


/* EN/RU: Owned sticky cancellation token/NULL; request is nonwaiting and thread-safe.
 * One admitted operation per token; admitted operation retains it independently.
 * Caller can free after admission; freeing alone does not request cancellation.
 * Свой sticky token/NULL; request thread-safe и без ожидания, один operation/token. */


/* [standalone-contract:styxcancelcreate]
 * EN: styxcancelcreate: native client cancel operations and lifetime.
 * RU: styxcancelcreate: нативные client-операции cancel и время жизни.
 * Parameters: none. Параметры: нет.
 * Return: StyxCancel *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxCancel *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxCancel *styxcancelcreate(void);


/* EN/RU: Request sticky cancellation from a retained native capability; no protocol wait.
 * Запросить sticky cancellation с retained capability; без protocol wait. */


/* [standalone-contract:styxcancelrequest]
 * EN: styxcancelrequest: native client cancel operations and lifetime.
 * RU: styxcancelrequest: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxcancelrequest(StyxCancel *cancel);


/* EN/RU: Consume caller token ref; admitted work retains its own ref, no implicit cancel.
 * Поглотить caller token ref; admitted work имеет свою ref, без implicit cancel. */


/* [standalone-contract:styxcancelfree]
 * EN: styxcancelfree: native client cancel operations and lifetime.
 * RU: styxcancelfree: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxcancelfree(StyxCancel *cancel);


/* EN/RU: Snapshot terminal transport/framing state under connection lock;1/0.
 * Not a heartbeat or a permission check. Снимок terminal state; не heartbeat/права. */


/* [standalone-contract:styxclientalive]
 * EN: styxclientalive: native client session operations and lifetime.
 * RU: styxclientalive: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientalive(StyxClient *client);


/* EN/RU: Copy immutable attached root kind,1 directory/0 file/-1 NULL or auth root.
 * Caller retains client; no protocol request or mutable server metadata query.
 * Копировать immutable attach root kind:1 directory/0 file/-1 invalid; без protocol request. */
