/* Copyright (c) 2026 bhgv
 * EN: Explicit original-Styx-over-S-record history profile; no peer guessing or change to raw 9P constructors.
 * RU: Явный original-Styx-over-S-record history profile; без угадывания peer и изменения raw 9P constructors.
 */
#include "mux_internal.h"
#include "packet.h"

/* EN/RU: See client.h; configure the profile before native owner publication/Tversion, consuming PHY always.
 * См. client.h; настроить profile до native owner publication/Tversion; PHY принять всегда.
 */


/* [standalone-contract:styxclientwalkmountphy]
 * EN: styxclientwalkmountphy: native client traversal operations and lifetime.
 * RU: styxclientwalkmountphy: нативные client-операции traversal и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
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
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * Return: StyxClient *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *styxclientwalkmountphy(RemotePhy *phy,
                                   unsigned msize,
                                   const char *user,
                                   const char *name,
                                   unsigned timeout,
                                   unsigned limit,
                                   unsigned long tag)
{
    PhyInfo info;
    int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
    StyxMux *mux = tag && tag <= 0xffffffffUL ? styxmuxcreatewalkphy(phy, msize, timeout, limit, tag) : NULL;
    if (!mux)
    {
        if (!tag || tag > 0xffffffffUL)
        {
            phyfree(phy);
        }
        return NULL;
    }
    StyxClient *client = styxclientstart(NULL, mux, msize, user, name);
    if (client)
    {
        client->bidir = bidir;
    }
    return client;
}

/* EN/RU: See client.h; copy only an acknowledged fixed root description, never an authority inferred from an ID.
 * См. client.h; копировать лишь acknowledged fixed root description, не authority из ID.
 */


/* [standalone-contract:styxclientwalktarget]
 * EN: styxclientwalktarget: native client traversal operations and lifetime.
 * RU: styxclientwalktarget: нативные client-операции traversal и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * root (WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientwalktarget(StyxClient *client, WalkEntity *root)
{
    if (!client || !client->mux || !root)
    {
        return 0;
    }
    StyxMux *mux = client->mux;
    pthread_mutex_lock(&mux->lock);
    int valid = mux->walk_tag && mux->walk_bound && !mux->failed;
    if (valid)
    {
        *root = mux->walk_root;
    }
    pthread_mutex_unlock(&mux->lock);
    return valid;
}

/* EN/RU: Under mux lock, validate the envelope then restore unchanged raw frame for original codec/response-tag processing.
 * Под mux lock проверить envelope, восстановить raw frame для исходного codec/tag processing;1/0.
 */


/* [standalone-contract:styxmuxwalkreply]
 * EN: styxmuxwalkreply: native client traversal operations and lifetime.
 * RU: styxmuxwalkreply: нативные client-операции traversal и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * cycle (unsigned *cycle): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxmuxwalkreply(StyxMux *mux, unsigned *cycle)
{
    WalkPacket packet;
    if (!walk_packet_read(mux->receive, mux->want, &packet) || packet.tag != mux->walk_tag ||
        (packet.kind & ~WALK_PACKET_CYCLE) != WALK_PACKET_REPLY || packet.frame_size > mux->capacity ||
        (mux->walk_bound && memcmp(&mux->walk_root, &packet.root, sizeof(packet.root))))
    {
        return 0;
    }
    mux->walk_root = packet.root;
    mux->walk_bound = 1;
    *cycle = (packet.kind & WALK_PACKET_CYCLE) != 0;
    memmove(mux->receive, packet.frame, packet.frame_size);
    mux->want = (unsigned)packet.frame_size;
    return 1;
}

/* EN/RU: See client.h; comparison identity never replaces the fid used by the actual operation.
 * См. client.h; comparison identity не заменяет fid, используемый actual operation.
 */


/* [standalone-contract:styxclientwalkprobe]
 * EN: styxclientwalkprobe: native client traversal operations and lifetime.
 * RU: styxclientwalkprobe: нативные client-операции traversal и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * base (StyxFid *base): Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid.
 * RU: Удерживаемый удалённый fid; согласовать open/clunk с использующими его вызовами.
 * relative (const char *relative): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * operation (unsigned operation): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * detail (const char *detail): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: WALK_INVALID, WALK_OK. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: WALK_INVALID, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientwalkprobe(StyxClient *client, StyxFid *base, const char *relative, unsigned operation, const char *detail)
{
    WalkEntity root;
    if (!client || !relative || !styxclientwalktarget(client, &root))
    {
        return WALK_OK;
    }
    const char *prefix = base ? base->walk_path : ".";
    size_t first = strcmp(prefix, ".") ? strlen(prefix) : 0;
    size_t tail = strcmp(relative, ".") ? strlen(relative) : 0;
    char path[STYX_NAME_MAX];
    if (first + tail + 2 > sizeof(path))
    {
        return WALK_INVALID;
    }
    memcpy(path, prefix, first);
    if (first && tail)
    {
        path[first++] = '/';
    }
    memcpy(path + first, relative, tail);
    path[first + tail] = 0;
    if (!first && !tail)
    {
        strcpy(path, ".");
    }
    WalkScope scope = {0};
    uvlong node = base && (operation == WALK_FSTAT || operation == WALK_LIST) ? base->qid.path : 0;
    int result = walk_enter(
        &scope, &root, (unsigned long)(node >> 32), (unsigned long)(node & 0xffffffffULL), operation, path, detail);
    if (result == WALK_OK && !walk_leave(&scope))
    {
        abort();
    }
    return result;
}
