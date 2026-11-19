/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of client.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */


/* [standalone-contract:styxclientcancellable]
 * EN: styxclientcancellable: native client fid operations and lifetime.
 * RU: styxclientcancellable: нативные client-операции fid и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientcancellable(StyxClient *client);


/* EN/RU: Local capability snapshot;1 if live carrier owner has at least two RPC slots,0 otherwise.
 * Local capability snapshot;1 для live carrier owner минимум с двумя RPC slots. */


/* [standalone-contract:styxclientwatchable]
 * EN: styxclientwatchable: native client fd operations and lifetime.
 * RU: styxclientwatchable: нативные client-операции fd и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientwatchable(StyxClient *client);


/* EN/RU: Long-lived native read without ordinary request deadline; explicit token required.
 * Same precision/flush fences as readcancel. Heartbeat/terminal/cancel still operate.
 * Caller pins/serializes fid and token; requires watchable client, uses one RPC slot.
 * Долгий native read без обычного deadline; explicit token, heartbeat/cancel сохраняются. */


/* [standalone-contract:styxclientreadwatch]
 * EN: styxclientreadwatch: native client io operations and lifetime.
 * RU: styxclientreadwatch: нативные client-операции io и время жизни.
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
void styxclientreadwatch(StyxFid *, unsigned long long, void *, size_t, StyxCancel *, StyxTransfer *);


/* EN/RU: Decode one complete original stat record; bytes borrowed,1/0 and consumed.
 * Reject malformed/truncated records and unsafe basename, no partial output.
 * Разобрать одну полную stat-запись;1/0 и consumed, без unsafe basename. */


/* [standalone-contract:styxclientrecord]
 * EN: styxclientrecord: native client stat operations and lifetime.
 * RU: styxclientrecord: нативные client-операции stat и время жизни.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * output (StyxMeta *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * consumed (size_t *consumed): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientrecord(const void *bytes, size_t size, StyxMeta *output, size_t *consumed);


/* EN/RU: Consume one fid, clunk if transport usable, then drop its connection pin.
 * Поглотить fid, clunk если transport пригоден; NULL допустим. */


/* [standalone-contract:styxclientclunk]
 * EN: styxclientclunk: native client fid operations and lifetime.
 * RU: styxclientclunk: нативные client-операции fid и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientclunk(StyxFid *fid);

/* EN/RU: Consume owned fid always; cancellable Tclunk on carrier-owner, async cleanup on legacy.
 * An unacknowledged request schedules one independent best-effort cleanup; never reuses fid numbers.
 * Caller serializes/pins fid before transfer. Token is borrowed single-use, NULL keeps deadline.
 * Поглотить fid всегда; carrier-owner Tclunk отменяем, legacy cleanup async; без Rclunk
 * один independent best-effort cleanup. Token borrowed single-use, NULL оставляет deadline. */


/* [standalone-contract:styxclientclunkcancel]
 * EN: styxclientclunkcancel: native client clunk operations and lifetime.
 * RU: styxclientclunkcancel: нативные client-операции clunk и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientclunkcancel(StyxFid *fid, StyxCancel *cancel);


/* EN/RU: Consume fid without RPC wait; native worker owns client/number only.
 * Setup failure fails the connection, never leaks caller ownership or waits for a guest.
 * Поглотить fid без RPC wait; worker владеет client/number; setup failure завершает connection. */


/* [standalone-contract:styxclientclunklater]
 * EN: styxclientclunklater: native client clunk operations and lifetime.
 * RU: styxclientclunklater: нативные client-операции clunk и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientclunklater(StyxFid *fid);


/* EN/RU: Borrow pinned fid; directory1/file0/invalid-1, no RPC.
 * Заимствовать pinned fid; directory1/file0/invalid-1, без RPC. */


/* [standalone-contract:styxclientfiddirectory]
 * EN: styxclientfiddirectory: native client fid operations and lifetime.
 * RU: styxclientfiddirectory: нативные client-операции fid и время жизни.
 * fid (StyxFid *fid): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientfiddirectory(StyxFid *fid);
