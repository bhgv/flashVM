/* Copyright (c) 2026 bhgv
 * EN: Scalar fixture boundary keeps Inferno and SDK native typedef domains separate.
 * RU: Scalar fixture boundary разделяет native typedef domains Inferno и SDK.
 */
#ifndef EVENT_FIXTURE_H
#define EVENT_FIXTURE_H
#include "file.h"


/* EN/RU: Construct shared source for independent=0, otherwise a bounded factory; fixture owns policy.
 * Создать shared source при independent=0, иначе bounded factory под fixture ownership. */
void eventfixturecreate(int independent);


/* EN/RU: Return owned native fid context/NULL; readonly opens get shared or independent inbox.
 * Вернуть свой native fid context/NULL; readonly open получает shared/independent inbox. */
void *eventfixtureopen(int mode);


/* EN/RU: Borrow this live fid context's inbox readiness fd; no read/close.
 * Заимствовать readiness fd inbox живого fid context; без read/close. */
int eventfixtureopenfd(void *context);


/* EN/RU: Read one chunk from retained ordinary open at explicit offset; initializes result.
 * Прочитать chunk удержанного ordinary open по offset; result инициализируется. */
void eventfixtureread(void *context, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result);


/* EN/RU: Consume owned fid context after pending request retirement, outside producer callbacks.
 * Поглотить fid context после pending request retirement, вне producer callbacks. */
void eventfixtureclose(void *context);


/* EN/RU: Native producer publishes scalar test changes or finishes source; no Client/guest pointers.
 * Native producer публикует scalar test changes либо завершает source, без Client/guest pointers. */
void eventfixturepublish(char command);


/* EN/RU: Consume backing/provider/source after producer join and all native clients retired.
 * Поглотить backing/provider/source после producer join и retirement native clients. */
void eventfixturefree(void);
#endif
