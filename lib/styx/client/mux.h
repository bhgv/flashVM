/* Copyright (c) 2026 bhgv
 * EN: Private native carrier multiplexer boundary; original codecs remain authoritative.
 * RU: Private native carrier multiplexer; исходные codecs остаются authoritative.
 */
#ifndef STYX_MUX_H
#define STYX_MUX_H
#include "client.h"
#include "life.h"
typedef struct StyxMux StyxMux;       /* Refcounted autonomous native carrier owner. */
typedef struct StyxFlight StyxFlight; /* One bounded admitted transaction. */

typedef struct StyxAnswer
{
    unsigned char *bytes; /* Owned raw original response frame, NULL without acknowledgement. */
    unsigned length;      /* Complete raw response length. */
    int attempted;        /* Original frame started sending. */
    int cancelled;        /* Cancel/timeout retired without original acknowledgement. */
    int cycle;            /* Proven negotiated remote traversal loop, never inferred from Rerror text. */
    int status;           /* 1 expected response,0 Rerror,-1 no acknowledgement. */
} StyxAnswer;

/* EN/RU: Consume connected reliable ordered stream PHY always; owned native engine/NULL.
 * Принять connected stream PHY всегда; свой native engine/NULL. */


/* [standalone-contract:styxmuxcreatephy]
 * EN: styxmuxcreatephy: native client mux operations and lifetime.
 * RU: styxmuxcreatephy: нативные client-операции mux и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * capacity (unsigned capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: StyxMux *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxMux *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxMux *styxmuxcreatephy(RemotePhy *phy, unsigned capacity, unsigned timeout, unsigned limit);

/* EN/RU: Explicit history profile, consumes PHY on every outcome; same bounds, nonzero direction-local data tag.
 * Явный history profile; PHY принят всегда, прежние bounds, nonzero data tag.
 */


/* [standalone-contract:styxmuxcreatewalkphy]
 * EN: styxmuxcreatewalkphy: native client mux operations and lifetime.
 * RU: styxmuxcreatewalkphy: нативные client-операции mux и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * capacity (unsigned capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * Return: StyxMux *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxMux *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxMux *styxmuxcreatewalkphy(RemotePhy *phy, unsigned capacity, unsigned timeout, unsigned limit, unsigned long tag);


/* EN/RU: Borrow terminal source while retaining engine owner. Без передачи владения. */


/* [standalone-contract:styxmuxlife]
 * EN: styxmuxlife: native client mux operations and lifetime.
 * RU: styxmuxlife: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxLife *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxLife *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxLife *styxmuxlife(StyxMux *mux);


/* EN/RU: Admit copied frame within quota; caller owns flight/NULL. Client lock held.
 * Принять copied frame в quota; свой flight/NULL, client lock удержан. */


/* [standalone-contract:styxmuxsubmit]
 * EN: styxmuxsubmit: native client mux operations and lifetime.
 * RU: styxmuxsubmit: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * bound (unsigned bound): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * watching (int watching): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: StyxFlight *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxFlight *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxFlight *styxmuxsubmit(StyxMux *, const void *, unsigned, unsigned, StyxCancel *, int);
/* EN/RU: Collect owned raw response after native completion; consumes flight.
 * Получить свой raw response после native completion; поглощает flight. */


/* [standalone-contract:styxmuxcollect]
 * EN: styxmuxcollect: native client mux operations and lifetime.
 * RU: styxmuxcollect: нативные client-операции mux и время жизни.
 * flight (StyxFlight *flight): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxAnswer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxAnswer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxAnswer styxmuxcollect(StyxFlight *);
/* EN/RU: Local native engine state snapshot, no network I/O.
 * Local native engine state snapshot, без network I/O. */


/* [standalone-contract:styxmuxalive]
 * EN: styxmuxalive: native client mux operations and lifetime.
 * RU: styxmuxalive: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxmuxalive(StyxMux *);

/* EN/RU: Copy tag counts without I/O. / Скопировать число тегов без I/O. */
void styxmuxtagcounts(StyxMux *, unsigned *, unsigned *);
/* EN/RU: Terminally reject all work and wake native owner; no guest access.
 * Terminally отклонить всю работу и разбудить owner, без guest access. */


/* [standalone-contract:styxmuxfail]
 * EN: styxmuxfail: native client mux operations and lifetime.
 * RU: styxmuxfail: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxmuxfail(StyxMux *);
/* EN/RU: Explicit native shutdown waits for carrier owner retirement; retains engine.
 * Явный native shutdown ждёт carrier owner retirement, engine не поглощает. */


/* [standalone-contract:styxmuxshutdown]
 * EN: styxmuxshutdown: native client mux operations and lifetime.
 * RU: styxmuxshutdown: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxmuxshutdown(StyxMux *);
/* EN/RU: Consume owner reference without joining; autonomous worker retires itself.
 * Поглотить owner ref без join; worker завершает себя сам. */


/* [standalone-contract:styxmuxfree]
 * EN: styxmuxfree: native client mux operations and lifetime.
 * RU: styxmuxfree: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxmuxfree(StyxMux *);
/* EN/RU: Nonwaiting PHY interrupt while holding engine owner or bound cancel token.
 * Сигнал PHY без ожидания при owner ref либо bound cancel token. */


/* [standalone-contract:styxmuxwake]
 * EN: styxmuxwake: native client mux operations and lifetime.
 * RU: styxmuxwake: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxmuxwake(StyxMux *);
/* EN/RU: Cancel binding retains token until unbind; only one admitted flight per token, never reuse.
 * Bind удерживает token до unbind; только один flight на token. */


/* [standalone-contract:styxcancelbind]
 * EN: styxcancelbind: native client cancel operations and lifetime.
 * RU: styxcancelbind: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxcancelbind(StyxCancel *, StyxMux *);


/* [standalone-contract:styxcancelled]
 * EN: styxcancelled: native client cancel operations and lifetime.
 * RU: styxcancelled: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxcancelled(StyxCancel *);

/* EN: Create an owned token for native mount I/O with bounded initial tag waiting.
 * Cancellation and the mux timeout bound admission before any frame can be sent.
 * RU: Создать свой токен для ожидания начального тега в операции маунта.
 * Допуск до отправки ограничен отменой и таймаутом mux. */
StyxCancel *styxcancelcreatewait(void);


/* EN: Check whether cancel still needs polling, without entering its hook.
 * RU: Проверить, нужен ли ещё опрос cancel, без вызова колбэка. */
int styxcancelpolling(StyxCancel *cancel);


/* EN: On the collector thread outside the mux lock, ask the hook to stop cancel.
 * RU: В потоке ожидания вне замка мультиплексора проверить флаг через колбэк. */
void styxcancelpoll(StyxCancel *cancel);


/* EN: Read the borrowed token's immutable admission policy.
 * RU: Прочитать неизменяемое правило допуска заимствованного токена. */
int styxcancelwaits(const StyxCancel *cancel);



/* [standalone-contract:styxcancelunbind]
 * EN: styxcancelunbind: native client cancel operations and lifetime.
 * RU: styxcancelunbind: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxcancelunbind(StyxCancel *);


/* EN/RU: Reserve an unused borrowed outer token for one logical fragmented walk.
 * Own a scope pin on success1;0 rejects reuse/overflow, NULL succeeds without pin.
 * Зарезервировать unused outer token на fragmented walk;1 и scope pin/0 отказ. */


/* [standalone-contract:styxcancelbegin]
 * EN: styxcancelbegin: native client cancel operations and lifetime.
 * RU: styxcancelbegin: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxcancelbegin(StyxCancel *cancel);


/* EN/RU: Borrow reserved outer scope, replace completed fragment with fresh child/NULL.
 * Child is scope-owned until next step/end; sticky request forwards atomically.
 * NULL is OOM, not permission to issue an uncancellable RPC. No guest state.
 * Заменить завершённый fragment на fresh child/NULL; child принадлежит scope. */


/* [standalone-contract:styxcancelstep]
 * EN: styxcancelstep: native client cancel operations and lifetime.
 * RU: styxcancelstep: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: StyxCancel *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxCancel *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxCancel *styxcancelstep(StyxCancel *cancel);


/* EN/RU: Consume successful scope pin after all fragment RPCs retired; NULL harmless.
 * Поглотить scope pin после retirement всех fragment RPC; NULL допустим. */


/* [standalone-contract:styxcancelend]
 * EN: styxcancelend: native client cancel operations and lifetime.
 * RU: styxcancelend: нативные client-операции cancel и время жизни.
 * cancel (StyxCancel *cancel): Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract.
 * RU: Явный sticky token отмены; один принятый operation на token, время жизни задано публичным контрактом.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxcancelend(StyxCancel *cancel);


/* EN/RU: Configure borrowed live engine heartbeat without waiting;1/0.
 * Настроить heartbeat удержанного engine без ожидания;1/0. */


/* [standalone-contract:styxmuxkeepalive]
 * EN: styxmuxkeepalive: native client heartbeat operations and lifetime.
 * RU: styxmuxkeepalive: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * idle (unsigned idle): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * reply (unsigned reply): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * misses (unsigned misses): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxmuxkeepalive(StyxMux *, unsigned, unsigned, unsigned);


/* EN/RU: Local streaming capability requires live carrier owner with at least two RPC slots.
 * Local streaming capability требует live carrier owner минимум с двумя RPC slots. */


/* [standalone-contract:styxmuxwatchable]
 * EN: styxmuxwatchable: native client mux operations and lifetime.
 * RU: styxmuxwatchable: нативные client-операции mux и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxmuxwatchable(StyxMux *mux);
#endif
