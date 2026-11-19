/* Copyright (c) 2026 bhgv
 * EN: Native PHY server admission, original-core owner scoping and safe wake-pin retirement.
 * RU: Native PHY server admission, original-core owner scoping и safe wake-pin retirement.
 */
#include "internal.h"

/* EN/RU: Consume one native context ref; last release occurs after server and wake users retire.
 * Поглотить native context ref; last release после server/wake users. */


/* [standalone-contract:styxphywakefree]
 * EN: styxphywakefree: phy/lifetime implementation and native state.
 * RU: styxphywakefree: реализация phy/lifetime и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphywakefree(StyxPhyWake *context)
{
    if (context && __atomic_sub_fetch(&context->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        styxphywalkclear(context);
        free(context->walk_decode);
        phyfree(context->phy);
        free(context->receive);
        free(context->output);
        free(context->lengths);
        free(context);
    }
}

/* EN/RU: See server.h; callback capability survives server end without retaining its shell/VM pointers.
 * См. server.h; callback capability живёт после server end без удержания shell/VM pointers. */


/* [standalone-contract:styxphywakepin]
 * EN: styxphywakepin: phy/lifetime implementation and native state.
 * RU: styxphywakepin: реализация phy/lifetime и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: StyxPhyWake *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: StyxPhyWake *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxPhyWake *styxphywakepin(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context)
    {
        return nil;
    }
    size_t refs = __atomic_load_n(&context->references, __ATOMIC_RELAXED);
    while (refs && refs != (size_t)-1)
    {
        if (__atomic_compare_exchange_n(&context->references, &refs, refs + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return context;
        }
    }
    return nil;
}

/* EN/RU: See server.h; no mutable server state or guest callback is accessed on a producer.
 * См. server.h; producer не касается mutable server state/guest callback. */


/* [standalone-contract:styxphywake]
 * EN: styxphywake: phy/lifetime implementation and native state.
 * RU: styxphywake: реализация phy/lifetime и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphywake(StyxPhyWake *context)
{
    if (context)
    {
        phywake(context->phy);
    }
}

/* EN/RU: Legacy constructor cannot silently create a network listener in the PHY backend.
 * Legacy constructor не создаёт network listener в PHY backend. */


/* [standalone-contract:styxinit]
 * EN: Initialize an original Styx server, its root and selected platform listener.
 * RU: Инициализировать исходный Styx-сервер, его корень и listener выбранной платформы.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * ops (Styxops *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * port (char *port): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * needfile (int needfile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxinit(Styxserver *server, Styxops *ops, char *port, int mode, int needfile)
{
    USED(server);
    USED(ops);
    USED(port);
    USED(mode);
    USED(needfile);
    return "use styxphyinit with an explicitly granted carrier";
}

/* EN/RU: See server.h; prepare all owned buffers before invoking unchanged original initialization.
 * См. server.h; подготовить buffers до unchanged original initialization. */


/* [standalone-contract:styxphyinitprofile]
 * EN: styxphyinitprofile: phy/lifetime implementation and native state.
 * RU: styxphyinitprofile: реализация phy/lifetime и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * ops (Styxops *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * permissions (int permissions): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * needfile (int needfile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * queued_frames (unsigned queued_frames): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * root (const WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxphyinitprofile(Styxserver *server,
                         Styxops *ops,
                         RemotePhy *phy,
                         int permissions,
                         int needfile,
                         unsigned queued_frames,
                         const WalkEntity *root)
{
    PhyInfo info;
    if (!server || !ops || server->ftab || server->priv || queued_frames < 1 || queued_frames > 128 ||
        !phyinfo(phy, &info) || !info.connected ||
        (info.capabilities & (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE)) != (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE))
    {
        phyfree(phy);
        return "invalid PHY server admission";
    }
    StyxPhyWake *context = calloc(1, sizeof(*context));
    if (!context)
    {
        phyfree(phy);
        return Enomem;
    }
    context->walk_mode = root != NULL;
    if (root)
    {
        context->walk_root = *root;
    }
    context->capacity = MSGMAX + (root ? WALK_PACKET_OVERHEAD : 0);
    context->stride = MSGMAX + (root ? 128U : 0);
    context->references = 1;
    context->phy = phy;
    context->wanted = 4;
    context->slots = queued_frames;
    context->walk_decode = root ? malloc(MSGMAX) : NULL;
    context->receive = malloc(context->capacity);
    context->output = malloc((size_t)queued_frames * context->stride);
    context->lengths = calloc(queued_frames, sizeof(*context->lengths));
    if (!context->receive || !context->output || !context->lengths || (root && !context->walk_decode))
    {
        styxphywakefree(context);
        return Enomem;
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    char *error = styxcoreinit(server, ops, "PHY", permissions, needfile);
    styxphyowner = previous;
    if (error)
    {
        styxphywakefree(context);
    }
    return error;
}

/* EN/RU: Original process API scopes owner-local close tokens; hardware output remains nonblocking.
 * Исходный process API задаёт owner-local close context; hardware output nonblocking. */


/* [standalone-contract:styxprocess]
 * EN: Dispatch available original Styx messages through the selected server callbacks.
 * RU: Диспетчеризовать доступные исходные Styx-сообщения через callbacks сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxprocess(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context)
    {
        return "PHY server is not initialized";
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    WalkScope endpoint = {0};
    WalkScope scope = {0};
    context->walk_before = walk_cycles();
    context->walk_cycle = 0;
    if (context->walk_mode && context->complete && !styxphywalkenter(context, &scope))
    {
        context->failed = 1;
        styxphyowner = previous;
        return "invalid traversal control/data record";
    }
    if (context->walk_mode && context->complete &&
        !styxphywalkrequest(context, context->receive, context->used, &endpoint))
    {
        int cycle = walk_cycles() != context->walk_before;
        context->processing = 1;
        int accepted = cycle && styxphywalkreject(server);
        context->processing = 0;
        if (scope.active && !walk_leave(&scope))
        {
            abort();
        }
        if (accepted)
        {
            styxphyoutput(context);
        }
        else
        {
            context->failed = 1;
        }
        styxphyowner = previous;
        return accepted ? NULL : "invalid traversal endpoint state";
    }
    context->processing = 1;
    char *error = styxcoreprocess(server);
    if (endpoint.active && !walk_leave(&endpoint))
    {
        abort();
    }
    if (scope.active && !walk_leave(&scope))
    {
        abort();
    }
    context->processing = 0;
    styxphyoutput(context);
    styxphyowner = previous;
    return error ? error : context->failed ? "PHY server carrier failed" : nil;
}

/* EN/RU: Original end API retires deferred reads/fids/clients once, then releases native context owner.
 * Native producer pins remain wakeable after disconnect until their own release.
 * Исходный end API завершает reads/fids/clients один раз; producer pins остаются wakeable до release. */


/* [standalone-contract:styxend]
 * EN: Retire the original server, its clients and opened fid state through their cleanup callbacks.
 * RU: Завершить исходный сервер, клиентов и открытые fid через callbacks освобождения.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxend(Styxserver *server)
{
    if (!server)
    {
        return nil;
    }
    StyxPhyWake *context = server->priv;
    if (!context)
    {
        return nil;
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    if (server->ftab)
    {
        styxendserver(server);
    }
    phydisconnect(context->phy);
    server->priv = nil;
    styxphyowner = previous;
    styxphywakefree(context);
    return nil;
}

/* EN/RU: See server.h; no driver or protocol operation occurs during this native quota query.
 * См. server.h; без driver/protocol operation при native quota query. */


/* [standalone-contract:styxphyspace]
 * EN: styxphyspace: phy/lifetime implementation and native state.
 * RU: styxphyspace: реализация phy/lifetime и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: unsigned; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: unsigned; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
unsigned styxphyspace(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context || context->failed)
    {
        return 0;
    }
    unsigned reserve = context->processing || context->complete ? 1 : 0;
    unsigned free = context->slots - context->queued;
    return free > reserve ? free - reserve : 0;
}

/* EN/RU: See server.h; sum bounded reply extents minus only the head's confirmed prefix.
 * См. server.h; сумма bounded reply extents минус confirmed prefix head. */


/* [standalone-contract:styxphypending]
 * EN: styxphypending: phy/lifetime implementation and native state.
 * RU: styxphypending: реализация phy/lifetime и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
size_t styxphypending(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    size_t pending = 0;
    if (context)
    {
        for (unsigned i = 0; i < context->queued; i++)
        {
            pending += context->lengths[(context->first + i) % context->slots];
        }
        pending -= context->sent;
    }
    return pending;
}
