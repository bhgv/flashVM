/* Copyright (c) 2026 bhgv
 * EN: Native mount binding for a dedicated reverse Styx session.
 * RU: Привязка нативного маунта к встречной сессии Styx.
 */
#ifndef STYX_CALLER_SERVER_H
#define STYX_CALLER_SERVER_H
#include "phy.h"
#include "provider.h"

typedef struct StyxCallerServer StyxCallerServer; /* Owned server and binding state. */


/* EN: Consume the reverse transport and return an owned server.
 * RU: Принять встречный транспорт и вернуть сервер. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse);


/* EN: Bind a fresh unpublished forward provider. Keep the server until every capture retires.
 * RU: Привязать свежий неопубликованный провайдер. Удерживать сервер до освобождения захватов. */
int styxcallerbind(StyxCallerServer *server, DVM_Provider *forward);


/* EN: Join and consume after carrier shutdown and release of the forward mount and captures.
 * RU: Дождаться сервера и освободить его после закрытия транспорта и ссылок на маунт. */
void styxcallerjoin(StyxCallerServer *server);
#endif
