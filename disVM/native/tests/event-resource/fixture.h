/* Copyright (c) 2026 bhgv
 * EN: Scalar fixture boundary keeps Inferno and SDK native typedef domains separate.
 * RU: Scalar fixture boundary разделяет native typedef domains Inferno и SDK.
 */
#ifndef EVENT_FIXTURE_H
#define EVENT_FIXTURE_H
#include "file.h"


/* EN/RU: Construct one explicitly filtered/formatted source and ordinary backing; owned by fixture.
 * Создать filtered/formatted source и ordinary backing под fixture ownership. */
StyxBacking *eventfixturecreate(void);


/* EN/RU: Borrow source readiness fd while fixture is alive; never drain/close it.
 * Заимствовать source readiness fd до fixture cleanup; не читать/закрывать. */
int eventfixturefd(void);


/* EN/RU: Native producer publishes scalar test changes or finishes source; no Client/guest pointers.
 * Native producer публикует scalar test changes либо завершает source, без Client/guest pointers. */
void eventfixturepublish(char command);


/* EN/RU: Consume backing/provider/source after producer join and all native clients retired.
 * Поглотить backing/provider/source после producer join и retirement native clients. */
void eventfixturefree(void);
#endif
