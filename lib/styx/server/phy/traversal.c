/* Copyright (c) 2026 bhgv
 * EN: Explicit S-record history server admission and scoped restoration before original libstyx dispatch.
 * RU: Явный S-record history server admission и scoped restoration до исходного libstyx dispatch.
 */
#include "internal.h"
#include "packet.h"

/* EN/RU: Validate native granted-root description; IDs describe a projection and never authenticate the peer.
 * Проверить native granted-root description; IDs описывают projection, не аутентифицируют peer.
 */


/* [standalone-contract:root_valid]
 * EN: root_valid: phy/traversal implementation and native state.
 * RU: root_valid: реализация phy/traversal и нативное состояние.
 * root (const WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int root_valid(const WalkEntity *root)
{
    unsigned long domain = 0;
    unsigned long object = 0;
    if (!root)
    {
        return 0;
    }
    for (unsigned i = 0; i < 4; i++)
    {
        if (root->domain[i] > 0xffffffffUL || root->object[i] > 0xffffffffUL)
        {
            return 0;
        }
        domain |= root->domain[i];
        object |= root->object[i];
    }
    return domain && object;
}

/* EN/RU: See server.h; explicit mode cannot fall back to raw 9P after malformed/unsupported control traffic.
 * См. server.h; explicit mode не делает fallback к raw 9P при malformed/unsupported control traffic.
 */


/* [standalone-contract:styxphywalkinit]
 * EN: styxphywalkinit: phy/traversal implementation and native state.
 * RU: styxphywalkinit: реализация phy/traversal и нативное состояние.
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
char *styxphywalkinit(Styxserver *server,
                      Styxops *ops,
                      RemotePhy *phy,
                      int permissions,
                      int needfile,
                      unsigned queued_frames,
                      const WalkEntity *root)
{
    if (!root_valid(root))
    {
        phyfree(phy);
        return "invalid traversal root identity";
    }
    return styxphyinitprofile(server, ops, phy, permissions, needfile, queued_frames, root);
}

/* EN/RU: Restore copied request-local state before raw libstyx sees the unchanged frame;1/0.
 * scope belongs to caller through dispatch/leave; receive buffer may now be reused safely.
 * Восстановить copied request-local state до raw libstyx frame;1/0. Scope caller до dispatch/leave.
 */


/* [standalone-contract:styxphywalkenter]
 * EN: styxphywalkenter: phy/traversal implementation and native state.
 * RU: styxphywalkenter: реализация phy/traversal и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * scope (WalkScope *scope): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkenter(StyxPhyWake *context, WalkScope *scope)
{
    WalkPacket packet;
    if (!walk_packet_read(context->receive, context->used, &packet) || packet.kind != WALK_PACKET_REQUEST ||
        packet.frame_size > MSGMAX || (context->walk_tag && context->walk_tag != packet.tag) ||
        walk_import(scope, packet.history, packet.history_size) != WALK_OK)
    {
        return 0;
    }
    context->walk_tag = packet.tag;
    memmove(context->receive, packet.frame, packet.frame_size);
    context->used = context->wanted = (unsigned)packet.frame_size;
    return 1;
}

/* EN/RU: Original API selects unchanged raw protocol; no hidden peer sniffing or control record interpretation.
 * Исходный API выбирает raw protocol; без hidden peer sniffing/control record interpretation.
 */


/* [standalone-contract:styxphyinit]
 * EN: styxphyinit: phy/traversal implementation and native state.
 * RU: styxphyinit: реализация phy/traversal и нативное состояние.
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
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *
styxphyinit(Styxserver *server, Styxops *ops, RemotePhy *phy, int permissions, int needfile, unsigned queued_frames)
{
    return styxphyinitprofile(server, ops, phy, permissions, needfile, queued_frames, NULL);
}

/* EN/RU: See server.h; only the currently owned callback may attach an explicit cycle outcome to its own reply.
 * См. server.h; только current owner callback может отметить cycle outcome собственного reply.
 */


/* [standalone-contract:styxphycycleerror]
 * EN: styxphycycleerror: phy/traversal implementation and native state.
 * RU: styxphycycleerror: реализация phy/traversal и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxphycycleerror(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : NULL;
    if (context && context == styxphyowner && context->processing)
    {
        context->walk_cycle = 1;
    }
    return "namespace traversal cycle";
}

/* EN/RU: See internal.h; pending scalar candidate is retired by normal response tracking, original fids remain untouched.
 * См. internal.h; normal response tracking освобождает pending scalar candidate, original fids не меняются.
 */


/* [standalone-contract:styxphywalkreject]
 * EN: styxphywalkreject: phy/traversal implementation and native state.
 * RU: styxphywalkreject: реализация phy/traversal и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkreject(Styxserver *server)
{
    StyxPhyWake *context = server->priv;
    Fcall reply = {0};
    unsigned char bytes[128];
    reply.type = Rerror;
    reply.tag = context->walk_request.tag;
    reply.ename = styxphycycleerror(server);
    unsigned length = convS2M(&reply, bytes, sizeof(bytes));
    int accepted = length && styxsend(server, 2, (char *)bytes, (int)length, 0) == (int)length;
    context->used = 0;
    context->wanted = 4;
    context->complete = 0;
    return accepted;
}
