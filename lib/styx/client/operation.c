/* Copyright (c) 2026 bhgv
 * EN: Scalar native control-RPC outcomes, preserving acknowledgement versus cancellation.
 * RU: Scalar native control-RPC outcomes с различением acknowledgement и cancellation.
 */
#include "internal.h"

/* EN/RU: See internal.h; a refused local operation has no remote effects.
 * См. internal.h; локальный отказ не создаёт remote effects. */


/* [standalone-contract:styxopclear]
 * EN: styxopclear: native client operation operations and lifetime.
 * RU: styxopclear: нативные client-операции operation и время жизни.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxopclear(StyxTransfer *result)
{
    memset(result, 0, sizeof(*result));
    result->confirmed = 1;
}

/* EN/RU: See internal.h; snapshot before cleanup RPC can overwrite client scratch state.
 * См. internal.h; snapshot до cleanup RPC, который может изменить client scratch state. */


/* [standalone-contract:styxopresult]
 * EN: styxopresult: native client operation operations and lifetime.
 * RU: styxopresult: нативные client-операции operation и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * status (int status): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * result (StyxTransfer *result): Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect.
 * RU: Копия native-подтверждения: count, uncertainty и stop reason; не разрешает повторить эффект.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxopresult(StyxClient *client, int status, StyxTransfer *result)
{
    styxopclear(result);
    result->success = status == 1;
    result->cancelled = client->cancelled;
    result->uncertain = status < 0 && client->attempted;
}
