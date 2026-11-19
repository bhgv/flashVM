/* Copyright (c) 2026 bhgv
 * EN: Independent native per-tag sequence/reorder queues and acknowledged local rollover, no generation per packet.
 * RU: Независимые очереди восстановления порядка по тегам и подтверждённый сброс без поколения в каждом пакете.
 */
#ifndef STYX_ORDER_H
#define STYX_ORDER_H
#include "flashvm/sorder.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define STYX_ORDER_RX 0U            /* Independent receiver counter in this namespace/direction. */
#define STYX_ORDER_TX 1U            /* Independent transmitter counter, never inferred from RX. */
#define STYX_ORDER_ACCEPTED 1       /* Complete verified packet copied, ready or held behind its tag's gap. */
#define STYX_ORDER_DUPLICATE 2      /* Old or identical held sequence, never executed twice. */
#define STYX_ORDER_WINDOW 3         /* Future gap exceeds configured transport reorder window; other tags unaffected. */
#define STYX_ORDER_REFUSED 0        /* Invalid/stopped input, or pending boundary would be crossed. */
#define STYX_ORDER_NOMEM (-1)       /* Exact packet copy/counter allocation failed, no partial acceptance. */
#define STYX_ORDER_CONFLICT (-2)    /* Different payload claimed an already held sequence. */
#define STYX_ORDER_RECORD 1         /* Exactly one local copied payload. */
#define STYX_ORDER_EMPTY 0          /* No ready record for selected tag while open. */
#define STYX_ORDER_END (-1)         /* Selected queue empty after connection owner closed. */
#define STYX_ORDER_SMALL (-2)       /* Required size only, ready head/ordinal preserved. */
typedef struct StyxOrder StyxOrder; /* Native owner of one namespace's independent directional tag counters. */


/* EN/RU: Owned engine/NULL, no worker or transport; body limit 1..S_BODY_MAX, reorder window 1..2^63-1.
 * Host feeds validated lanes on native broker threads; clients wait for their own ready tag.
 * Вернуть собственный engine либо NULL, без worker/transport; limit ограничивает тело записи, window — перестановку номеров, а не длину очереди клиента.
 */


/* [standalone-contract:styxordercreate]
 * EN: styxordercreate: independent per-tag life state.
 * RU: styxordercreate: независимое состояние тегов life.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * window (unsigned long long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: StyxOrder *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxOrder *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxOrder *styxordercreate(unsigned limit, unsigned long long window);


/* EN: Tighten receive displacement before first RX data, 1/0; equal window remains idempotent.
 * Borrow engine; native peer has agreed this bound. Existing TX counters are untouched.
 * RU: Сузить окно приёма до первых RX-данных: 1/0, равное окно идемпотентно. TX не меняется.
 */


/* [standalone-contract:styxorderreceivewindow]
 * EN: styxorderreceivewindow: independent per-tag life state.
 * RU: styxorderreceivewindow: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * window (unsigned long long window): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderreceivewindow(StyxOrder *order, unsigned long long window);


/* EN/RU: Native-only seed an unused directional counter before peer traffic,1/0; default starts at0.
 * Existing/used direction cannot be overwritten. Caller agrees initial value with peer; no inferred wire command.
 * Задать first для неиспользованного tag/direction: 1/0; по умолчанию ноль. Начальное значение явно согласуется с другой стороной.
 */


/* [standalone-contract:styxorderstart]
 * EN: styxorderstart: independent per-tag life state.
 * RU: styxorderstart: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * direction (unsigned direction): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * first (unsigned long long first): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderstart(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long first);


/* EN/RU: Encode one packet into caller-owned disjoint spool, advance TX only on OK; no transport I/O.
 * Returned packet must be retained/retried as the SAME bytes until sent or connection retired, not re-encoded.
 * Pending rollover/MAX exhaustion returns CONFLICT for only this tag. Output/used unchanged on failure.
 * Кодировать пакет в буфер caller; TX меняется только при OK. Для повторной передачи сохранять те же байты, не назначать новый номер.
 */


/* [standalone-contract:styxorderencode]
 * EN: styxorderencode: independent per-tag send state.
 * RU: styxorderencode: независимое состояние тегов send.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult styxorderencode(
    StyxOrder *order, unsigned long tag, const void *bytes, size_t size, void *output, size_t capacity, size_t *used);


/* EN/RU: Validate and copy exactly one complete prefixed packet, return acceptance/duplicate/window/error above.
 * Caller retains input through return; no direct client callback, gap in one tag never blocks others.
 * Проверить и скопировать один пакет с префиксом; результаты перечислены выше. Пропуск одного тега не блокирует другие.
 */


/* [standalone-contract:styxorderfeed]
 * EN: styxorderfeed: independent per-tag receive state.
 * RU: styxorderfeed: независимое состояние тегов receive.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * packet (const void *packet): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: STYX_ORDER_REFUSED. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: STYX_ORDER_REFUSED. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderfeed(StyxOrder *order, const void *packet, size_t size);


/* EN/RU: Copy one ready selected-tag record/SMALL/EMPTY/END; successful pop advances consumed checkpoint.
 * One consumer per tag, output/ordinal unchanged except successful copy; SMALL changes only size.
 * Выдать одну готовую запись выбранного тега и отметить выданный номер; SMALL меняет только size, не потребляя запись.
 */


/* [standalone-contract:styxordernext]
 * EN: styxordernext: independent per-tag inbox state.
 * RU: styxordernext: независимое состояние тегов inbox.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * ordinal (unsigned long long *ordinal): Output sequence of the consumed record; unchanged when no record is consumed.
 * RU: Выходной номер поглощённой записи; без поглощения не меняется.
 * bytes (void *bytes): Caller-owned destination, or NULL when capacity is zero.
 * RU: Выходной буфер вызывающего либо NULL при capacity=0.
 * capacity (size_t capacity): Destination bytes; SMALL preserves the queued record for retry.
 * RU: Выходная ёмкость; SMALL сохраняет запись в очереди для повторного чтения.
 * size (size_t *size): Required/actual payload bytes when a record is present, including SMALL.
 * RU: Требуемые/фактические байты payload при наличии записи, также при SMALL.
 * Return: int; direct result constants in this body: STYX_ORDER_END. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: STYX_ORDER_END. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxordernext(
    StyxOrder *order, unsigned long tag, unsigned long long *ordinal, void *bytes, size_t capacity, size_t *size);


/* EN/RU: Block only selected-tag native consumer under one monotonic budget 0..120000ms, 1 ready/0 otherwise.
 * Ждать только выбранный тег: 1 — готов, 0 — иначе. Без потребления пакета и остановки брокера.
 */


/* [standalone-contract:styxorderwait]
 * EN: styxorderwait: independent per-tag inbox state.
 * RU: styxorderwait: независимое состояние тегов inbox.
 * order (StyxOrder *order): Live native queue engine; keep alive until this wait and other concurrent calls finish.
 * RU: Живой native queue engine; удерживать до завершения этого wait и параллельных вызовов.
 * tag (unsigned long tag): Only this direction-local tag queue is observed.
 * RU: Наблюдается только очередь этого тега направления.
 * timeout (unsigned timeout): Monotonic wait budget in milliseconds, 0..120000; zero tests current readiness.
 * RU: Бюджет по monotonic clock в миллисекундах 0..120000; ноль проверяет текущую готовность.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderwait(StyxOrder *order, unsigned long tag, unsigned timeout);


/* EN/RU: Native queued-reset proposal for ONE counter; strictly increasing nonzero local operation ID.
 * TX boundary must equal last spooled ordinal. RX boundary may wait for its old prefix to arrive/be consumed.
 * 1/0, no wire traffic; host queues service request after old packets and ACKs only when ready returns1.
 * Подготовить сброс одного счётчика: 1/0. Host сам передаёт служебные request/ACK; остальные теги не затронуты.
 */


/* [standalone-contract:styxorderprepare]
 * EN: styxorderprepare: independent per-tag rollover state.
 * RU: styxorderprepare: независимое состояние тегов rollover.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * direction (unsigned direction): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * operation (unsigned long long operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * last (unsigned long long last): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderprepare(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, unsigned long long boundary);


/* EN/RU: Copy readiness of this exact pending rollover,1/0; RX requires consumed boundary and empty held/ready queues.
 * Готовность точно выбранного сброса: 1/0. RX требует выдачи boundary и пустых очередей held/ready.
 */


/* [standalone-contract:styxorderready]
 * EN: styxorderready: independent per-tag rollover state.
 * RU: styxorderready: независимое состояние тегов rollover.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * direction (unsigned direction): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * operation (unsigned long long operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderready(StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation);


/* EN/RU: Apply matching accepted ACK(1)/explicit NACK(0),1/0; exact most-recent repeats idempotent.
 * ACK resets only this directional counter to0. No automatic wrap; wrong/not-ready confirmation changes nothing.
 * Native host authenticates session/operation and retires old lane deliveries before accepting rollover.
 * Точное ACK сбрасывает лишь этот tag/direction в ноль: 1/0. Ошибочное либо преждевременное подтверждение ничего не меняет.
 */


/* [standalone-contract:styxorderconfirm]
 * EN: styxorderconfirm: independent per-tag rollover state.
 * RU: styxorderconfirm: независимое состояние тегов rollover.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * direction (unsigned direction): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * operation (unsigned long long operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * accepted (int accepted): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderconfirm(
    StyxOrder *order, unsigned long tag, unsigned direction, unsigned long long operation, int accepted);


/* EN/RU: Non-consuming old-prefix checkpoint,1/0; packet merely held/ready is not considered consumed.
 * Native host uses this per-tag watermark before applying assignment requests across lanes.
 * Проверить отметку выданного номера тега: 1/0. Записи в held/ready ещё не выданы; host использует отметку при переназначении.
 */


/* [standalone-contract:styxorderreached]
 * EN: styxorderreached: independent per-tag inbox state.
 * RU: styxorderreached: независимое состояние тегов inbox.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * boundary (unsigned long long boundary): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxorderreached(StyxOrder *order, unsigned long tag, unsigned long long boundary);


/* EN/RU: Close connection owner, wake all native waits, keep ready records individually drainable.
 * Закрыть соединение и разбудить ожидающих; готовые записи остаются доступны для поштучного чтения.
 */


/* [standalone-contract:styxorderclose]
 * EN: styxorderclose: independent per-tag life state.
 * RU: styxorderclose: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxorderclose(StyxOrder *order);


/* EN/RU: Consume after all feed/encode/consumer calls retire; NULL harmless, no owned worker to join.
 * Освободить после завершения всех вызовов; NULL безопасен, собственных workers нет.
 */


/* [standalone-contract:styxorderfree]
 * EN: styxorderfree: independent per-tag life state.
 * RU: styxorderfree: независимое состояние тегов life.
 * order (StyxOrder *order): Native per-tag queue engine; keep it alive until all concurrent calls retire.
 * RU: Нативный механизм очередей тегов; удерживать до завершения параллельных вызовов.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxorderfree(StyxOrder *order);
#ifdef __cplusplus
}
#endif
#endif
