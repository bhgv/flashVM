/* Copyright (c) 2026 bhgv
 * EN: One admitted native scalar call and its reply.
 * RU: Один допущенный нативный скалярный вызов и его ответ.
 */
#ifndef STYX_SCALAR_GATEWAY_H
#define STYX_SCALAR_GATEWAY_H
#include "scalar_interface.h"

typedef struct StyxScalarGateway StyxScalarGateway; /* Caller-owned one-shot route. */

typedef enum StyxScalarOutcome
{
    STYX_GATEWAY_PENDING, /* Try reading again without resending the request. */
    STYX_GATEWAY_VALUE, /* Output contains the owner result. */
    STYX_GATEWAY_ERROR, /* Output contains the owner error code. */
    STYX_GATEWAY_UNKNOWN /* No trustworthy result; do not replay. */
} StyxScalarOutcome;


/* EN: Check a profile read from the supplied exact file, then retain the request and open the reply.
 * The host supplies exact file grants and expected import. Invalid input returns NULL.
 * RU: Сверить профиль из выданного файла, удержать запрос и открыть ответ.
 * Хост выдаёт точные файлы и ожидаемый импорт. При отказе вернуть NULL. */
StyxScalarGateway *styxscalargateway(DVM_Provider *profile, unsigned long profile_tag,
    const char *name, uint32_t signature, DVM_Provider *request, unsigned long call_tag,
    DVM_Provider *reply, unsigned long reply_tag);


/* EN/RU: Attempt one write only, with no replay even after unknown delivery.
 * Выполнить одну попытку записи, без повтора даже при неизвестном исходе. */
DVM_LaunchDelivery styxscalargatewaysend(StyxScalarGateway *gateway, int64_t argument);


/* EN/RU: Read one reply after delivery; output changes only for value/error.
 * Прочитать ответ после доставки; выход меняется только при значении/ошибке. */
StyxScalarOutcome styxscalargatewayreceive(StyxScalarGateway *gateway, int64_t *value);


/* EN/RU: Close the reply and release the request; NULL admitted.
 * Закрыть ответ и освободить запрос; NULL допустим. */
void styxscalargatewayfree(StyxScalarGateway *gateway);

#endif
