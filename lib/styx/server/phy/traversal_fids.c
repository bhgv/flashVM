/* Copyright (c) 2026 bhgv
 * EN: Bound per-connection fid search identities for the explicit traversal profile; original libstyx owns all real fids.
 * RU: Bounded fid search identities profile; исходный libstyx владеет всеми настоящими fids.
 */
#include "internal.h"
#define WALK_FID_LIMIT 4096U /* Profile-side scalar metadata quota, independent of original fid authority. */

/* EN/RU: Find borrowed scalar metadata under the sole server owner, or NULL; no I/O or native fid authority.
 * Найти borrowed scalar metadata под единственным server owner/NULL; без I/O/fid authority.
 */


/* [standalone-contract:find]
 * EN: find: phy/traversal_fids implementation and native state.
 * RU: find: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * id (u32int id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: StyxWalkFid *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxWalkFid *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static StyxWalkFid *find(StyxPhyWake *context, u32int id)
{
    for (StyxWalkFid *fid = context->walk_fids; fid; fid = fid->next)
    {
        if (fid->id == id)
        {
            return fid;
        }
    }
    return NULL;
}

/* EN/RU: Remove scalar metadata by ID; original core independently clunks actual fids and their native resources.
 * Удалить scalar metadata по ID; original core независимо clunk actual fids/resources.
 */


/* [standalone-contract:remove_fid]
 * EN: remove_fid: phy/traversal_fids implementation and native state.
 * RU: remove_fid: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * id (u32int id): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void remove_fid(StyxPhyWake *context, u32int id)
{
    StyxWalkFid **link = &context->walk_fids;
    while (*link)
    {
        if ((*link)->id == id)
        {
            StyxWalkFid *old = *link;
            *link = old->next;
            context->walk_fid_count--;
            free(old);
            return;
        }
        link = &(*link)->next;
    }
}

/* EN/RU: Consume only local scalar tracker storage at version/reset/server retirement, outside provider callbacks.
 * Поглотить только scalar tracker storage при version/reset/retirement, вне provider callbacks.
 */


/* [standalone-contract:styxphywalkclear]
 * EN: styxphywalkclear: phy/traversal_fids implementation and native state.
 * RU: styxphywalkclear: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphywalkclear(StyxPhyWake *context)
{
    while (context->walk_fids)
    {
        remove_fid(context, context->walk_fids->id);
    }
    free(context->walk_pending);
    context->walk_pending = NULL;
}

/* EN/RU: Join a counted acknowledged component prefix into bounded canonical state;1/0.
 * Native client uses canonical forward walks, but dot/parent are normalized for standards-compatible peers.
 * Соединить acknowledged component prefix в bounded canonical state;1/0, dot/parent нормализуются.
 */


/* [standalone-contract:path]
 * EN: path: phy/traversal_fids implementation and native state.
 * RU: path: реализация phy/traversal_fids и нативное состояние.
 * base (const char *base): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * request (const Fcall *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * count (unsigned count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * output (char *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int path(const char *base, const Fcall *request, unsigned count, char *output)
{
    strcpy(output, base);
    for (unsigned i = 0; i < count; i++)
    {
        const char *name = request->wname[i];
        if (!strcmp(name, "."))
        {
            continue;
        }
        if (!strcmp(name, ".."))
        {
            char *slash = strrchr(output, '/');
            if (slash)
            {
                *slash = 0;
            }
            else
            {
                strcpy(output, ".");
            }
            continue;
        }
        if (!*name || strchr(name, '/'))
        {
            return 0;
        }
        size_t first = strcmp(output, ".") ? strlen(output) : 0;
        size_t tail = strlen(name);
        if (first + tail + 2 > WALK_PATH_LIMIT)
        {
            return 0;
        }
        if (first)
        {
            output[first++] = '/';
        }
        memcpy(output + first, name, tail + 1);
    }
    return 1;
}

/* EN/RU: Capture request metadata and enter the logical exported endpoint before original dispatch;1/0.
 * Input is an unchanged complete raw frame; decode uses separate owned scratch, scope borrowed through return/dispatch.
 * Снять metadata запроса, войти в logical exported endpoint до original dispatch;1/0. Decode — отдельный scratch.
 */


/* [standalone-contract:styxphywalkrequest]
 * EN: styxphywalkrequest: phy/traversal_fids implementation and native state.
 * RU: styxphywalkrequest: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame (const void *frame): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * endpoint (WalkScope *endpoint): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkrequest(StyxPhyWake *context, const void *frame, unsigned length, WalkScope *endpoint)
{
    memcpy(context->walk_decode, frame, length);
    Fcall *request = &context->walk_request;
    if (convM2S(context->walk_decode, length, request) != length)
    {
        return 0;
    }
    if (request->type == Tversion)
    {
        styxphywalkclear(context);
        return 1;
    }
    StyxWalkFid *base = find(context, request->fid);
    unsigned operation = 0;
    uvlong node = 0; /* Named traversal uses root/path; held metadata/list also identifies its actual Qid. */
    const char *detail = "";
    if (request->type == Tattach || request->type == Twalk)
    {
        if (context->walk_fid_count >= WALK_FID_LIMIT || (request->type == Twalk && !base))
        {
            return 0;
        }
        context->walk_pending = calloc(1, sizeof(*context->walk_pending));
        if (!context->walk_pending)
        {
            return 0;
        }
        context->walk_pending->id = request->type == Tattach ? request->fid : request->newfid;
        strcpy(context->walk_base, base ? base->path : ".");
        if (request->type == Tattach)
        {
            strcpy(context->walk_pending->path, ".");
        }
        else if (!path(context->walk_base, request, request->nwname, context->walk_pending->path))
        {
            return 0;
        }
        strcpy(context->walk_target, context->walk_pending->path);
        operation = WALK_INFO;
    }
    else if (base)
    {
        strcpy(context->walk_target, base->path);
        if (request->type == Tstat)
        {
            operation = base->opened ? WALK_FSTAT : WALK_INFO;
            node = base->opened ? base->node : 0;
        }
        else if (request->type == Topen)
        {
            operation = WALK_OPEN;
            detail = (request->mode & 3) == 0 ? "r" : (request->mode & 3) == 1 ? "wo" : "rw";
        }
        else if (request->type == Tread && base->directory)
        {
            operation = WALK_LIST;
            node = base->node;
        }
    }
    return !operation || walk_enter(endpoint,
                                    &context->walk_root,
                                    (unsigned long)(node >> 32),
                                    (unsigned long)(node & 0xffffffffULL),
                                    operation,
                                    context->walk_target,
                                    detail) == WALK_OK;
}

/* EN/RU: Apply only acknowledged original replies to scalar fid metadata; partial Rwalk retains the actual prefix.
 * Request decode/path storage stays owner-local. No original fid or provider is created/freed by this function.
 * Применить лишь acknowledged replies к scalar fid metadata; partial Rwalk сохраняет actual prefix, без fid authority.
 */


/* [standalone-contract:styxphywalkresponse]
 * EN: styxphywalkresponse: phy/traversal_fids implementation and native state.
 * RU: styxphywalkresponse: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame (const void *frame): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkresponse(StyxPhyWake *context, const void *frame, unsigned length)
{
    Fcall reply;
    unsigned char decoded[MSGMAX];
    memcpy(decoded, frame, length);
    if (convM2S(decoded, length, &reply) != length)
    {
        return 0;
    }
    Fcall *request = &context->walk_request;
    if (reply.tag != request->tag)
    {
        return 1; /* Deferred byte replies have no search/fid mutation here. */
    }
    StyxWalkFid *pending = context->walk_pending;
    if (pending && (reply.type == Rattach || reply.type == Rwalk))
    {
        if (reply.type == Rwalk &&
            (reply.nwqid > request->nwname || !path(context->walk_base, request, reply.nwqid, pending->path)))
        {
            return 0;
        }
        StyxWalkFid *base = find(context, request->fid);
        pending->directory = reply.type == Rattach ? (reply.qid.type & QTDIR) != 0
                             : reply.nwqid         ? (reply.wqid[reply.nwqid - 1].type & QTDIR) != 0
                                                   : base && base->directory;
        pending->node = reply.type == Rattach ? reply.qid.path
                        : reply.nwqid         ? reply.wqid[reply.nwqid - 1].path
                        : base                ? base->node
                                              : 0;
        remove_fid(context, pending->id);
        pending->next = context->walk_fids;
        context->walk_fids = pending;
        context->walk_pending = NULL;
        context->walk_fid_count++;
    }
    else if (reply.type == Ropen)
    {
        StyxWalkFid *fid = find(context, request->fid);
        if (fid)
        {
            fid->directory = (reply.qid.type & QTDIR) != 0;
            fid->opened = 1;
            fid->node = reply.qid.path;
        }
    }
    else if (reply.type == Rclunk || reply.type == Rremove)
    {
        remove_fid(context, request->fid);
    }
    free(context->walk_pending);
    context->walk_pending = NULL;
    return 1;
}
