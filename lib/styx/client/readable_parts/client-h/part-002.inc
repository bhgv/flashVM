/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of client.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* [standalone-contract:styxclientrootdirectory]
 * EN: styxclientrootdirectory: native client fid operations and lifetime.
 * RU: styxclientrootdirectory: нативные client-операции fid и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientrootdirectory(StyxClient *client);


#define STYX_LINK_FAILED 1          /* Terminal transport/framing failure. */
#define STYX_LINK_CLOSED 2          /* Explicit native shutdown, not normal final owner drop. */
typedef struct StyxWatch StyxWatch; /* Owned native terminal subscription. */

#define STYX_LINK_READY 3           /* Responsive/initial source; heartbeat success after suspicion. */
#define STYX_LINK_SUSPECT 4         /* Sent probe has missed a response window, connection retained. */
#define STYX_LINK_STALLED 5         /* Probe cannot finish sending within a send window. */
#define STYX_LINK_TIMEOUT 6         /* Heartbeat policy exhausted; does not prove remote death. */

/* EN/RU: Copied local connection observation, no permission or remote object identity.
 * Копия local observation соединения, без permissions/remote object identity. */
typedef struct StyxLinkEvent
{
    unsigned long long generation; /* Monotonic local observation version, initially1. */
    int state;                     /* STYX_LINK_*; timeout is policy failure rather than proof of death. */
} StyxLinkEvent;

/* EN/RU: Native nonwaiting observation outside source/client/engine locks; no guest,
 * provider/transport I/O or target wait. Nonblocking native readiness is allowed.
 * Native observation вне locks; без guest/provider/transport I/O/target wait;
 * nonblocking native readiness разрешён. */
typedef void (*StyxLinkReceive)(void *state, const StyxLinkEvent *event);


/* EN/RU: Own state only on success; persistent watch/NULL, immediate current snapshot.
 * Caller pins client; ordered generations fence replay racing with native delivery.
 * Detach with clientunwatch; release follows all admitted callbacks, no guest calls.
 * Persistent watch/NULL принимает state при успехе; snapshot немедленно, generations ordered. */


/* [standalone-contract:styxclientobserve]
 * EN: styxclientobserve: native client fd operations and lifetime.
 * RU: styxclientobserve: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * receive (StyxLinkReceive receive): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *));


/* EN/RU: Copy local latest observation without I/O;1/0, output untouched on invalid input.
 * Копировать latest local observation без I/O;1/0, invalid не меняет output. */


/* [standalone-contract:styxclientlink]
 * EN: styxclientlink: native client fd operations and lifetime.
 * RU: styxclientlink: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * output (StyxLinkEvent *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientlink(StyxClient *client, StyxLinkEvent *output);


/* EN/RU: Nonwaiting native terminal delivery outside source/client/engine locks.
 * May copy an event/unsubscribe, never perform provider/transport I/O, guest calls or wait for a target.
 * Nonblocking native readiness notification is allowed.
 * Native terminal callback вне locks; без provider/transport I/O/guest calls/target wait;
 * native readiness signal разрешён. */
typedef void (*StyxTerminal)(void *state, int reason);


/* EN/RU: Adopt state only on success, owned watch/NULL; terminal state replays once.
 * At most256 live watcher nodes/client. Retain client while registering. State
 * outlives already admitted callbacks; release runs after final unsubscribe/flight.
 * Принять state только при успехе; свой watch/NULL, sticky terminal replay. */


/* [standalone-contract:styxclientwatch]
 * EN: styxclientwatch: native client session operations and lifetime.
 * RU: styxclientwatch: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * notify (StyxTerminal notify): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxclientwatch(StyxClient *client, StyxTerminal notify, void *state, void (*release)(void *));


/* EN/RU: Consume watch without waiting; an already admitted callback may finish.
 * Поглотить watch без ожидания; admitted callback может завершиться позже. */


/* [standalone-contract:styxclientunwatch]
 * EN: styxclientunwatch: native client life operations and lifetime.
 * RU: styxclientunwatch: нативные client-операции life и время жизни.
 * watch (StyxWatch *watch): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientunwatch(StyxWatch *watch);


/* EN/RU: Drop connection reference after callers retire; live fids pin transport.
 * Final release closes transport once; server retires root on disconnect. NULL allowed.
 * Поглотить ссылку; live fids удерживают transport; NULL допустим. */


/* [standalone-contract:styxclientfree]
 * EN: styxclientfree: native client session operations and lifetime.
 * RU: styxclientfree: нативные client-операции session и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientfree(StyxClient *client);


/* EN/RU: Walk canonical relative path, dot for root; owned independent fid/NULL.
 * Reject absolute/empty/dotdot/embedded dot components before wire I/O. Multiple
 * MAXWELEM batches preserve the same new fid. Calls serialized by connection mutex.
 * Пройти canonical relative path; свой fid/NULL, без выхода выше root. */


/* [standalone-contract:styxclientwalk]
 * EN: styxclientwalk: native client walk operations and lifetime.
 * RU: styxclientwalk: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalk(StyxClient *client, const char *relative);


/* EN/RU: Walk from a borrowed pinned unopened fid; dot clones, owned independent result/NULL.
 * Serialize base open/clunk against this call; concurrent walks are allowed. File descendants
 * and unsafe paths refuse before wire effects. No fallback to the original attach pathname.
 * Пройти от pinned unopened fid; dot клонирует, свой result/NULL; без fallback к attach pathname. */


/* [standalone-contract:styxclientwalkfrom]
 * EN: styxclientwalkfrom: native client walk operations and lifetime.
 * RU: styxclientwalkfrom: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxFid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);


/* EN/RU: Cancellable logical walk for native PHY clients; owned fid/NULL, required result.
 * One outer token covers all MAXWELEM fragments and cannot be reused. Result uses
 * StyxTransfer with count0; success means the complete walk was acknowledged.
 * Failure cleanup is native detached best-effort clunk, never a caller RPC wait.
 * Base overload borrows a pinned unopened fid; same confinement as walkfrom.
 * Отменяемый logical walk; свой fid/NULL, required result, count0; token один на все
 * fragments. Cleanup без RPC wait caller; base удержан и unopened на время вызова. */


/* [standalone-contract:styxclientwalkcancel]
 * EN: styxclientwalkcancel: native client walk operations and lifetime.
 * RU: styxclientwalkcancel: нативные client-операции walk и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkcancel(StyxClient *client, const char *relative, StyxCancel *cancel, StyxTransfer *result);


/* [standalone-contract:styxclientwalkfromcancel]
 * EN: styxclientwalkfromcancel: native client walk operations and lifetime.
 * RU: styxclientwalkfromcancel: нативные client-операции walk и время жизни.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: StyxFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result);


/* EN/RU: Open unopened fid in mode0/1/2 without create/truncate;1/0.
 * Caller serializes a fid's calls. Ropen iounit limits every later chunk.
 * An unacknowledged sent open fences later open/data calls; stat/clunk remain allowed.
 * Открыть fid без create/truncate;1/0, iounit ограничивает chunk. Sent open без
 * acknowledgement запрещает повторный open/data; stat/clunk остаются доступны. */


/* [standalone-contract:styxclientopen]
 * EN: styxclientopen: native client open operations and lifetime.
 * RU: styxclientopen: нативные client-операции open и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientopen(StyxFid *fid, int mode);


/* EN/RU: Query current remote fid metadata;1/0, output changes only on success.
 * Tstat не открывает путь заново;1/0, output только при успехе. */


/* [standalone-contract:styxclientstat]
 * EN: styxclientstat: native client stat operations and lifetime.
 * RU: styxclientstat: нативные client-операции stat и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * output (StyxMeta *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientstat(StyxFid *fid, StyxMeta *output);


/* EN/RU: Cancellable Topen/Tstat require mountfd and result; return1/0, count0.
 * Original replies win over cancellation. Sent unacknowledged Topen poisons the fid:
 * no later open/data replay; caller still owns it and may stat/clunk. Tstat leaves
 * output untouched on every failure and does not poison the fid. Serialize/pin fid.
 * NULL cancel keeps the normal deadline; non-NULL tokens remain single-use.
 * Отменяемые Topen/Tstat:1/0, count0, result обязателен; original reply приоритетен.
 * Topen без acknowledgement блокирует повторный open/data; Tstat не меняет output
 * при отказе и не poison fid. Caller удерживает/сериализует fid; token single-use. */


/* [standalone-contract:styxclientopencancel]
 * EN: styxclientopencancel: native client open operations and lifetime.
 * RU: styxclientopencancel: нативные client-операции open и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result);


/* [standalone-contract:styxclientstatcancel]
 * EN: styxclientstatcancel: native client stat operations and lifetime.
 * RU: styxclientstatcancel: нативные client-операции stat и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * output (StyxMeta *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientstatcancel(StyxFid *fid, StyxMeta *output, StyxCancel *cancel, StyxTransfer *result);


/* EN: Return nonzero to stop the caller's pending data transfer.
 * state is borrowed opaque data; keep the hook brief and keep it valid until return.
 * RU: Вернуть ненулевое значение для остановки ожидающей передачи. Колбэк
 * должен быстро завершаться, непрозрачное state сохраняется до возврата. */
typedef int (*StyxClientStop)(void *state);


/* EN: Read one native chunk. Sample optional stop(state) on the calling
 * thread before sending and during multiplexed reply waits. Keep fid and the
 * buffer valid through return. offset and size are bytes. Set result to the
 * acknowledged count or termination flags. When stop is set, cancelled reports
 * a hook-requested stop. Serial clients sample before sending only.
 * RU: Прочитать нативный фрагмент. Проверять необязательный stop(state) у
 * вызывающего до отправки и во время ожидания на мультиплексоре. Удерживать
 * fid и буфер до возврата. offset и size заданы в байтах. В result записать
 * подтверждённую длину или флаги завершения. Если задан stop, cancelled отмечает
 * остановку по колбэку. Последовательный клиент проверяет до отправки. */
void styxclientreaduntil(StyxFid *fid, unsigned long long offset, void *bytes, size_t size,
    StyxClientStop stop, void *state, StyxTransfer *result);


/* EN: Write one native chunk with the same stop protocol as readuntil.
 * Read size bytes from bytes at offset with the same result and ownership rules.
 * On cancellation after a write attempt, effects may be uncertain.
 * RU: Записать нативный фрагмент с отменой как у readuntil. После попытки
 * записи её эффекты при отмене могут остаться неизвестными. Прочитать size байтов
 * из bytes для записи со смещения offset по тем же правилам result и владения. */
void styxclientwriteuntil(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size,
    StyxClientStop stop, void *state, StyxTransfer *result);


/* EN/RU: One actual Tread/Twrite chunk, signed64 offset range; exact confirmed
 * prefix, no auto retry. Uncertain effects poison further data calls, except a
 * new offset-zero read of an already-open read-only directory.
 * Один Tread/Twrite chunk; точный prefix, без retry. При uncertain effects
 * разрешён только новый Tread с нуля у каталога, открытого на чтение. */


/* [standalone-contract:styxclientread]
 * EN: styxclientread: native client io operations and lifetime.
 * RU: styxclientread: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientread(StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result);


/* [standalone-contract:styxclientwrite]
 * EN: styxclientwrite: native client io operations and lifetime.
 * RU: styxclientwrite: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientwrite(StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *result);


/* EN/RU: Cancellable chunk requires native PHY client; same precise-prefix contract.
 * Original acknowledgement before Rflush wins; otherwise cancelled is set, with
 * uncertain effects if original bytes were sent. Caller serializes the fid.
 * Cancellable chunk требует mountfd; original acknowledgement не теряется. */


/* [standalone-contract:styxclientreadcancel]
 * EN: styxclientreadcancel: native client io operations and lifetime.
 * RU: styxclientreadcancel: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientreadcancel(
    StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);


/* [standalone-contract:styxclientwritecancel]
 * EN: styxclientwritecancel: native client io operations and lifetime.
 * RU: styxclientwritecancel: нативные client-операции io и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * offset (unsigned long long offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientwritecancel(
    StyxFid *fid, unsigned long long offset, const void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);


/* EN/RU: Immutable explicit-cancel capability,1 for carrier-owner,0 for NULL/legacy; no I/O.
 * Immutable explicit-cancel capability:1 carrier-owner,0 NULL/legacy; без I/O/прав. */
