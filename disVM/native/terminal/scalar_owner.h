/* Copyright (c) 2026 bhgv
 * EN: Owner-side scalar request handoff outside the Styx dispatcher.
 * RU: Передача скалярного запроса владельцу вне диспетчера Styx.
 */
#ifndef STYX_SCALAR_OWNER_H
#define STYX_SCALAR_OWNER_H
#include "scalar_gateway.h"

typedef struct StyxScalarOwner StyxScalarOwner; /* Owned one-shot request cursor and reply grant. */


/* EN/RU: Open the exact inbox for read and retain its independently granted reply file.
 * Открыть точный приёмник на чтение и удержать выданный файл ответа. */
StyxScalarOwner *styxscalarowner(DVM_Provider *inbox, unsigned long request_tag,
    DVM_Provider *reply, unsigned long reply_tag);


/* EN/RU: Take one valid call. VALUE publishes its argument, PENDING allows another read.
 * Принять один вызов. VALUE выдаёт аргумент, PENDING допускает ещё одно чтение. */
StyxScalarOutcome styxscalarownertake(StyxScalarOwner *owner, int64_t *argument);


/* EN/RU: Reply once after take; success selects value or owner error.
 * Ответить один раз после take. success выбирает значение или ошибку владельца. */
DVM_LaunchDelivery styxscalarownersend(StyxScalarOwner *owner, int success, int64_t result);


/* EN/RU: Close the request cursor and release the reply grant. NULL allowed.
 * Закрыть позицию чтения и освободить файл ответа. NULL допустим. */
void styxscalarownerfree(StyxScalarOwner *owner);
#endif
