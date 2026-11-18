/* Copyright (c) 2026 bhgv
 * EN: Native fixture cancellation gates and independent operation ownership, no guest callbacks.
 * RU: Native gates отмены fixture и владение независимой операцией, без guest callbacks.
 */
#ifndef TERMINAL_ACTIVE_IO_GATE_H
#define TERMINAL_ACTIVE_IO_GATE_H
#include "cancel.h"


/* EN/RU: Wait for gate index 0/1 on caller's native cancel scope, 1 released/0 cancelled; no polling.
 * Ждать gate index 0/1 под native cancel scope caller; 1 released/0 cancelled, без опроса.
 */
int activegate(unsigned index);


/* EN/RU: Wait for both native operations to subscribe before triggering retirement; fixture timeout bounds failure.
 * Дождаться подписок обеих native operations до retirement; timeout fixture ограничивает отказ.
 */
void activeentered(void);


/* EN/RU: Verify only server operation cancelled, release independent operation; exit on contract failure.
 * Проверить отмену только server operation и освободить независимую; exit при нарушении contract.
 */
void activeindependent(void);
#endif
