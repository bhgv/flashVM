/* Copyright (c) 2026 bhgv
 * EN: Private native terminal/health source independent of Client/VM/provider lifetime.
 * RU: Private native terminal/health source независимо от Client/VM/provider lifetime.
 */
#ifndef STYX_LIFE_H
#define STYX_LIFE_H
#include "client.h"
typedef struct StyxLife StyxLife; /* Refcounted terminal source with bounded watchers. */


/* EN/RU: Create owned native source/NULL. Создать свой native source/NULL. */


/* [standalone-contract:styxlifecreate]
 * EN: styxlifecreate: native client life operations and lifetime.
 * RU: styxlifecreate: нативные client-операции life и время жизни.
 * Parameters: none. Параметры: нет.
 * Return: StyxLife *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxLife *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxLife *styxlifecreate(void);


/* EN/RU: Retain a borrowed live source;1/0. Удержать source;1/0. */


/* [standalone-contract:styxliferetain]
 * EN: styxliferetain: native client life operations and lifetime.
 * RU: styxliferetain: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxliferetain(StyxLife *life);


/* EN/RU: Consume source ref after borrowers retire; NULL allowed.
 * Поглотить source ref после borrowers; NULL допустим. */


/* [standalone-contract:styxlifedrop]
 * EN: styxlifedrop: native client life operations and lifetime.
 * RU: styxlifedrop: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifedrop(StyxLife *life);


/* EN/RU: Publish first terminal reason, callbacks outside source lock; duplicate ignored.
 * Опубликовать первый terminal reason вне source lock; дубликат игнорируется. */


/* [standalone-contract:styxlifeemit]
 * EN: styxlifeemit: native client life operations and lifetime.
 * RU: styxlifeemit: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reason (int reason): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifeemit(StyxLife *life, int reason);


/* EN/RU: Normal final owner retirement detaches future callbacks without notification/wait.
 * Normal final retirement отсоединяет будущие callbacks без notification/wait. */


/* [standalone-contract:styxlifequiet]
 * EN: styxlifequiet: native client life operations and lifetime.
 * RU: styxlifequiet: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifequiet(StyxLife *life);


/* EN/RU: Subscribe with owned callback state on success; sticky terminal replay outside lock.
 * Subscribe принимает callback state при успехе; sticky replay вне lock. */


/* [standalone-contract:styxlifewatch]
 * EN: styxlifewatch: native client life operations and lifetime.
 * RU: styxlifewatch: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * notify (StyxTerminal notify): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxlifewatch(StyxLife *life, StyxTerminal notify, void *state, void (*release)(void *));


/* EN/RU: Persistent native observer adopts state on success, immediate snapshot outside lock.
 * Persistent observer принимает state при успехе, snapshot вне lock. */


/* [standalone-contract:styxlifeobserve]
 * EN: styxlifeobserve: native client life operations and lifetime.
 * RU: styxlifeobserve: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * receive (StyxLinkReceive receive): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxlifeobserve(StyxLife *, StyxLinkReceive, void *, void (*)(void *));


/* EN/RU: Publish changed observation including first terminal state; engine serializes publications.
 * Опубликовать changed observation; engine caller сериализует publications. */


/* [standalone-contract:styxlifechange]
 * EN: styxlifechange: native client life operations and lifetime.
 * RU: styxlifechange: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * state (int state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifechange(StyxLife *, int);


/* EN/RU: Copy latest state under source lock; output borrowed,1/0.
 * Копировать latest state под source lock; output borrowed,1/0. */


/* [standalone-contract:styxlifesnapshot]
 * EN: styxlifesnapshot: native client life operations and lifetime.
 * RU: styxlifesnapshot: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * output (StyxLinkEvent *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxlifesnapshot(StyxLife *, StyxLinkEvent *);
#endif
