/* Copyright (c) 2026 bhgv
 * EN: Original server platform hooks over an owned PHY, no listener or descriptor namespace.
 * RU: Platform hooks исходного server через owned PHY без listener/descriptor namespace.
 */
#include "internal.h"

__thread StyxPhyWake *styxphyowner; /* Scoped native owner, never shared across threads. */

/* EN/RU: Original init hook; carrier setup already occurred before server admission.
 * Исходный init hook; carrier setup выполнен до server admission. */


/* [standalone-contract:styxinitsocket]
 * EN: styxinitsocket: phy/backend implementation and native state.
 * RU: styxinitsocket: реализация phy/backend и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxinitsocket(void)
{
    return 0;
}

/* EN/RU: Original global cleanup hook; no process-global physical state exists.
 * Исходный global cleanup hook; process-global physical state отсутствует. */


/* [standalone-contract:styxendsocket]
 * EN: styxendsocket: phy/backend implementation and native state.
 * RU: styxendsocket: реализация phy/backend и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxendsocket(void)
{
}

/* EN/RU: Internal tokens are owner-local protocol handles, never OS descriptors or wire fields.
 * Внутренние tokens — owner-local handles, не OS descriptors/wire fields. */


/* [standalone-contract:styxclosesocket]
 * EN: styxclosesocket: phy/backend implementation and native state.
 * RU: styxclosesocket: реализация phy/backend и нативное состояние.
 * token (int token): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclosesocket(int token)
{
    if (styxphyowner && token == 2)
    {
        styxphyowner->failed = 1;
        phydisconnect(styxphyowner->phy);
    }
}

/* EN/RU: Original announce hook admits one already granted peer, without bind/listen or name lookup.
 * Исходный announce hook допускает одного granted peer без bind/listen/name lookup. */


/* [standalone-contract:styxannounce]
 * EN: styxannounce: phy/backend implementation and native state.
 * RU: styxannounce: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxannounce(Styxserver *server, char *name)
{
    USED(server);
    USED(name);
    return 1;
}

/* EN/RU: Original initializer attaches only the staged owner context, allocation already succeeded.
 * Исходный initializer принимает staged owner context; allocation уже завершена. */


/* [standalone-contract:styxinitwait]
 * EN: styxinitwait: phy/backend implementation and native state.
 * RU: styxinitwait: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxinitwait(Styxserver *server)
{
    server->priv = styxphyowner;
}

/* EN/RU: Admit the connected peer once on original process owner; no transport accept/dial.
 * Допустить connected peer один раз на owner, без transport accept/dial. */


/* [standalone-contract:styxnewcall]
 * EN: styxnewcall: phy/backend implementation and native state.
 * RU: styxnewcall: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnewcall(Styxserver *server)
{
    return !((StyxPhyWake *)server->priv)->accepted;
}

/* EN/RU: Return owner-local peer token, preserving original core client construction.
 * Вернуть owner-local peer token для original client construction. */


/* [standalone-contract:styxaccept]
 * EN: styxaccept: phy/backend implementation and native state.
 * RU: styxaccept: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: 2. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 2. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxaccept(Styxserver *server)
{
    ((StyxPhyWake *)server->priv)->accepted = 1;
    return 2;
}

/* EN/RU: Original client-registration hook has no descriptor side effects.
 * Исходный registration hook не меняет descriptors. */


/* [standalone-contract:styxnewclient]
 * EN: styxnewclient: phy/backend implementation and native state.
 * RU: styxnewclient: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * token (int token): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxnewclient(Styxserver *server, int token)
{
    USED(server);
    USED(token);
}

/* EN/RU: Original detachment hook; fid/client cleanup remains in the unchanged core.
 * Исходный detachment hook; fid/client cleanup остаётся в unchanged core. */


/* [standalone-contract:styxfreeclient]
 * EN: styxfreeclient: phy/backend implementation and native state.
 * RU: styxfreeclient: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * token (int token): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxfreeclient(Styxserver *server, int token)
{
    USED(server);
    USED(token);
}

/* EN/RU: Original readiness hook only exposes complete framed input or a terminal stream.
 * Исходный readiness hook показывает только complete input/terminal stream. */


/* [standalone-contract:styxnewmsg]
 * EN: styxnewmsg: phy/backend implementation and native state.
 * RU: styxnewmsg: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * token (int token): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnewmsg(Styxserver *server, int token)
{
    StyxPhyWake *context = server->priv;
    return token == 2 && (context->complete || context->failed);
}

/* EN/RU: Owner pumps native queues, waits through the PHY and returns for local producer wake/space.
 * No cooperative guest, busy-empty polling, socket wait or foreign server callbacks.
 * Owner обрабатывает native queues и PHY wait; return при local wake/space, без guest cooperation. */


/* [standalone-contract:styxphywait]
 * EN: styxphywait: phy/backend implementation and native state.
 * RU: styxphywait: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxphywait(Styxserver *server, int timeout)
{
    StyxPhyWake *context = server ? server->priv : NULL;
    double deadline = 0;
    if (!context || timeout < -1)
    {
        return "invalid PHY wait";
    }
    if (timeout > 0)
    {
        double now = styxphytime();
        if (now < 0)
        {
            return "PHY wait clock unavailable";
        }
        deadline = now + timeout;
    }
    if (!context->accepted)
    {
        return nil;
    }
    for (;;)
    {
        unsigned before = context->queued;
        styxphyoutput(context);
        if (context->queued < context->slots)
        {
            styxphyreceive(context);
        }
        if (context->complete || context->failed || context->queued < before)
        {
            return nil;
        }
        unsigned wanted = context->queued < context->slots ? PHY_READ : 0;
        if (context->queued)
        {
            wanted |= PHY_WRITE;
        }
        int remaining = timeout;
        if (timeout >= 0)
        {
            double now = styxphytime();
            if (!timeout || now >= deadline)
            {
                return nil;
            }
            if (now < 0)
            {
                return "PHY wait clock unavailable";
            }
            double budget = deadline - now;
            remaining = (int)budget;
            if (remaining < budget)
            {
                ++remaining;
            }
        }
        unsigned ready;
        if (phywait(context->phy, wanted, remaining, &ready) != PHY_OK || (ready & PHY_FAILED))
        {
            context->failed = 1;
            return nil;
        }
        if (ready & PHY_WAKE)
        {
            return nil;
        }
    }
}


/* EN/RU: Original wait retains its indefinitely blocking behavior.
 * Исходное ожидание сохраняет неограниченное блокирование. */
char *styxwaitmsg(Styxserver *server)
{
    return styxphywait(server, -1);
}

/* EN/RU: Preserve the original platform fatal-exit hook, without introducing a guest control API.
 * Сохранить original platform fatal-exit hook, без API управления guest. */


/* [standalone-contract:styxexit]
 * EN: styxexit: phy/backend implementation and native state.
 * RU: styxexit: реализация phy/backend и нативное состояние.
 * status (int status): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxexit(int status)
{
    exit(status);
}

/* EN/RU: Explicitly reject OS-descriptor registration on this carrier-independent backend.
 * Existing native producers use owned styxphywake pins instead; -1 unregister is harmless.
 * OS-fd registration здесь запрещён; producers используют styxphywake pin, -1 harmless. */


/* [standalone-contract:styxnotifyfd]
 * EN: styxnotifyfd: phy/backend implementation and native state.
 * RU: styxnotifyfd: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnotifyfd(Styxserver *server, int descriptor)
{
    return server && server->priv && descriptor == -1 ? 0 : -1;
}
