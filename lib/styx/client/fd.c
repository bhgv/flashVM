/* Copyright (c) 2026 bhgv
 * EN: Explicit POSIX fd-owner construction, native lifetime pins and shutdown.
 * RU: Явное создание POSIX fd-owner, native lifetime pins и shutdown.
 */
#include "internal.h"
#include "drivers/posix/fd.h"
#include <unistd.h>

/* EN/RU: See client.h; native engine consumes connected fd on all outcomes.
 * См. client.h; native engine принимает connected fd при любом исходе. */


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
styxclientmountfd(int fd, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit)
{
    return styxclientmountphy(
        phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), msize, user, name, timeout, limit);
}

/* EN/RU: See client.h; caller already holds a live capability through this call.
 * См. client.h; caller уже удерживает live capability на время вызова. */


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
int styxclientretain(StyxClient *client)
{
    if (!client)
    {
        return 0;
    }
    pthread_mutex_lock(&client->lock);
    int valid = client->references != (size_t)-1;
    if (valid)
    {
        client->references++;
    }
    pthread_mutex_unlock(&client->lock);
    return valid;
}

/* EN/RU: See client.h; explicit shutdown may wait on native I/O, never on guest execution.
 * См. client.h; explicit shutdown может ждать native I/O, но не guest execution. */


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
void styxclientshutdown(StyxClient *client)
{
    if (!client)
    {
        return;
    }
    if (client->mux)
    {
        styxmuxshutdown(client->mux);
    }
    else
    {
        pthread_mutex_lock(&client->lock);
        client->failed = 1;
        if (!client->transportclosed)
        {
            client->transportclosed = 1;
            client->transport.close(client->transport.state);
        }
        styxclientemit(client, STYX_LINK_CLOSED);
        pthread_mutex_unlock(&client->lock);
    }
}

/* EN/RU: See client.h; native configuration never performs protocol work on caller.
 * См. client.h; configuration не выполняет protocol work на caller. */


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
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses)
{
    return client && client->mux ? styxmuxkeepalive(client->mux, idle, reply, misses) : 0;
}

/* EN/RU: See client.h; subscription context owns only explicitly retained native data.
 * См. client.h; context владеет только явно удержанными native data. */


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
StyxWatch *styxclientobserve(StyxClient *client, StyxLinkReceive receive, void *state, void (*release)(void *))
{
    return client ? styxlifeobserve(client->life, receive, state, release) : NULL;
}

/* EN/RU: See client.h; copy independent native source state, without transport access.
 * См. client.h; копировать independent native state без transport access. */


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
int styxclientlink(StyxClient *client, StyxLinkEvent *output)
{
    return client ? styxlifesnapshot(client->life, output) : 0;
}

/* EN/RU: See client.h; native capability snapshot, not a quota reservation.
 * См. client.h; native capability snapshot, не quota reservation. */


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
int styxclientwatchable(StyxClient *client)
{
    return client && client->mux ? styxmuxwatchable(client->mux) : 0;
}
