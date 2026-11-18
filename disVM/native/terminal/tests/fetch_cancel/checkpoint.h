/* Copyright (c) 2026 bhgv
 * EN: Native fixture checkpoints for admitted RPCs, sink I/O and readiness waits, no polling.
 * RU: Native checkpoints admitted RPCs, sink I/O и readiness waits, без опроса.
 */
#ifndef TERMINAL_FETCH_CHECKPOINT_H
#define TERMINAL_FETCH_CHECKPOINT_H
#define FETCH_RPC 1U /* Actual original deferred Tread admitted. */
#define FETCH_SINK 2U /* Native sink subscribed and waiting for cancellation. */
#define FETCH_CANCEL 4U /* Sink's current native scope cancelled. */
#define FETCH_FLUSH 8U /* Original deferred read released by Tflush/close. */
#define FETCH_READY_WAIT 16U /* Actual receiver readiness condition wait entered. */


/* EN/RU: Publish one fixture checkpoint bit under native mutex, no guest work.
 * Опубликовать checkpoint bit под native mutex, без guest work.
 */
void fetchmark(unsigned bit);


/* EN/RU: Wait until all requested bits are present; external peer timeout bounds a broken fixture.
 * Ждать все bits; внешний peer timeout ограничивает сломанный fixture.
 */
void fetchwait(unsigned bits);


/* EN/RU: Block only this native sink on its current scope, returning on cancellation, no polling.
 * Блокировать только native sink на current scope до cancellation, без опроса.
 */
void fetchsinkwait(void);


/* EN/RU: Borrow receiver condition address through wait/free, observe its real native wait only.
 * Заимствовать address receiver condition до wait/free, наблюдать только настоящий native wait.
 */
void fetcharmwait(void *condition);
#endif
