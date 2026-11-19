/* Copyright (c) 2026 bhgv
 * EN: Connect generic owned physical carriers to the existing native Styx transaction engine.
 * RU: Подключить generic owned physical carriers к существующему native Styx engine.
 */
#include "internal.h"

/* EN/RU: See client.h; shared native engine consumes PHY even on setup failure.
 * См. client.h; общий engine принимает PHY даже при setup failure. */


/* [standalone-contract:styxclientmountphy]
 * EN: styxclientmountphy: native client phy operations and lifetime.
 * RU: styxclientmountphy: нативные client-операции phy и время жизни.
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
 * Return: StyxClient *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxClient *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxClient *
styxclientmountphy(RemotePhy *phy, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit)
{
    PhyInfo info;
    int bidir = phyinfo(phy, &info) && (info.capabilities & PHY_BIDIR_SESSION);
    StyxMux *mux = styxmuxcreatephy(phy, msize, timeout, limit);
    StyxClient *client = mux ? styxclientstart(NULL, mux, msize, user, name) : NULL;
    if (client)
    {
        client->bidir = bidir;
    }
    return client;
}

/* EN/RU: See client.h; immutable native mode, no I/O or observation of a remote server.
 * См. client.h; immutable native mode, без I/O и наблюдения remote server. */


/* [standalone-contract:styxclientbidir]
 * EN: styxclientbidir: native client phy operations and lifetime.
 * RU: styxclientbidir: нативные client-операции phy и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxclientbidir(StyxClient *client)
{
    return client && client->bidir;
}
