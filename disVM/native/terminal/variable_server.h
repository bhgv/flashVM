/* Copyright (c) 2026 bhgv
 * EN: Serve one exact file capability under .var on a connected Styx channel.
 * RU: Экспорт точной файловой ссылки под .var через подключённый Styx.
 */
#ifndef STYX_VARIABLE_SERVER_H
#define STYX_VARIABLE_SERVER_H
#include "provider.h"
#include "phy.h"
#include <stdint.h>

typedef struct StyxVariableServer StyxVariableServer;


/* EN/RU: Consume channel, retain borrowed file on success, return owned server or NULL.
 * Принять канал, удержать файл при успехе, вернуть сервер или NULL. */
StyxVariableServer *styxvariableserverstart(RemotePhy *channel, const char *name, DVM_Provider *file);


/* EN/RU: Consume channel and publish module/.prog/pid/.var/name and read-only .pid.
 * Принять канал и опубликовать module/.prog/pid/.var/name и .pid только для чтения. */
StyxVariableServer *styxvariableserverstartprogram(RemotePhy *channel, const char *module,
    uint32_t pid, const char *name, DVM_Provider *file);


/* EN/RU: After carrier close, join the server and consume it on success.
 * После закрытия канала дождаться сервера и поглотить его при успехе. */
int styxvariableserverjoin(StyxVariableServer *server);

#endif
