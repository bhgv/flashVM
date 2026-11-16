/* Copyright (c) 2026 bhgv
 * EN: Private native terminal/health source independent of Client/VM/provider lifetime.
 * RU: Private native terminal/health source независимо от Client/VM/provider lifetime.
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


/* EN/RU: Persistent native observer adopts state on success, immediate snapshot outside lock.
 * Persistent observer принимает state при успехе, snapshot вне lock. */
StyxWatch *styxlifeobserve(StyxLife *, StyxLinkReceive, void *, void (*)(void *));


/* EN/RU: Publish changed observation including first terminal state; engine serializes publications.
 * Опубликовать changed observation; engine caller сериализует publications. */
void styxlifechange(StyxLife *, int);


/* EN/RU: Copy latest state under source lock; output borrowed,1/0.
 * Копировать latest state под source lock; output borrowed,1/0. */
int styxlifesnapshot(StyxLife *, StyxLinkEvent *);
#endif
