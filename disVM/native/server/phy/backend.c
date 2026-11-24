/* Copyright (c) 2026 bhgv
 * EN: Original server platform hooks over an owned PHY, no listener or descriptor namespace.
 * RU: Platform hooks исходного server через owned PHY без listener/descriptor namespace.
 */
#include "internal.h"

__thread StyxPhyWake *styxphyowner; /* Scoped native owner, never shared across threads. */


/* EN/RU: Original init hook; carrier setup already occurred before server admission.
 * Исходный init hook; carrier setup выполнен до server admission. */
int styxinitsocket(void)
{
    return 0;
}


/* EN/RU: Original global cleanup hook; no process-global physical state exists.
 * Исходный global cleanup hook; process-global physical state отсутствует. */
void styxendsocket(void)
{
}


/* EN/RU: Internal tokens are owner-local protocol handles, never OS descriptors or wire fields.
 * Внутренние tokens — owner-local handles, не OS descriptors/wire fields. */
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
int styxannounce(Styxserver *server, char *name)
{
    USED(server);
    USED(name);
    return 1;
}


/* EN/RU: Original initializer attaches only the staged owner context, allocation already succeeded.
 * Исходный initializer принимает staged owner context; allocation уже завершена. */
void styxinitwait(Styxserver *server)
{
    server->priv = styxphyowner;
}


/* EN/RU: Admit the connected peer once on original process owner; no transport accept/dial.
 * Допустить connected peer один раз на owner, без transport accept/dial. */
int styxnewcall(Styxserver *server)
{
    return !((StyxPhyWake *)server->priv)->accepted;
}


/* EN/RU: Return owner-local peer token, preserving original core client construction.
 * Вернуть owner-local peer token для original client construction. */
int styxaccept(Styxserver *server)
{
    ((StyxPhyWake *)server->priv)->accepted = 1;
    return 2;
}


/* EN/RU: Original client-registration hook has no descriptor side effects.
 * Исходный registration hook не меняет descriptors. */
void styxnewclient(Styxserver *server, int token)
{
    USED(server);
    USED(token);
}


/* EN/RU: Original detachment hook; fid/client cleanup remains in the unchanged core.
 * Исходный detachment hook; fid/client cleanup остаётся в unchanged core. */
void styxfreeclient(Styxserver *server, int token)
{
    USED(server);
    USED(token);
}


/* EN/RU: Original readiness hook only exposes complete framed input or a terminal stream.
 * Исходный readiness hook показывает только complete input/terminal stream. */
int styxnewmsg(Styxserver *server, int token)
{
    StyxPhyWake *context = server->priv;
    return token == 2 && (context->complete || context->failed);
}


/* EN/RU: Owner pumps native queues, waits through the PHY and returns for local producer wake/space.
 * No cooperative guest, busy-empty polling, socket wait or foreign server callbacks.
 * Owner обрабатывает native queues и PHY wait; return при local wake/space, без guest cooperation. */
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
void styxexit(int status)
{
    exit(status);
}


/* EN/RU: Explicitly reject OS-descriptor registration on this carrier-independent backend.
 * Existing native producers use owned styxphywake pins instead; -1 unregister is harmless.
 * OS-fd registration здесь запрещён; producers используют styxphywake pin, -1 harmless. */
int styxnotifyfd(Styxserver *server, int descriptor)
{
    return server && server->priv && descriptor == -1 ? 0 : -1;
}

