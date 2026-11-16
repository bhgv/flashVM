/* Copyright (c) 2026 bhgv
 * EN: Private one-shot native terminal source, independent of Client/VM/provider lifetime.
 * RU: Private one-shot terminal source независимо от Client/VM/provider lifetime.
 */
#ifndef STYX_LIFE_H
#define STYX_LIFE_H
#include "client.h"
typedef struct StyxLife StyxLife; /* Refcounted terminal source with bounded watchers. */


/* EN/RU: Create owned native source/NULL. Создать свой native source/NULL. */
StyxLife *styxlifecreate(void);


/* EN/RU: Retain a borrowed live source;1/0. Удержать source;1/0. */
int styxliferetain(StyxLife *life);


/* EN/RU: Consume source ref after borrowers retire; NULL allowed.
 * Поглотить source ref после borrowers; NULL допустим. */
void styxlifedrop(StyxLife *life);


/* EN/RU: Publish first terminal reason, callbacks outside source lock; duplicate ignored.
 * Опубликовать первый terminal reason вне source lock; дубликат игнорируется. */
void styxlifeemit(StyxLife *life, int reason);


/* EN/RU: Normal final owner retirement detaches future callbacks without notification/wait.
 * Normal final retirement отсоединяет будущие callbacks без notification/wait. */
void styxlifequiet(StyxLife *life);


/* EN/RU: Subscribe with owned callback state on success; sticky terminal replay outside lock.
 * Subscribe принимает callback state при успехе; sticky replay вне lock. */
StyxWatch *styxlifewatch(StyxLife *life, StyxTerminal notify, void *state, void (*release)(void *));
#endif
