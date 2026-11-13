/* Copyright (c) 2026 bhgv
 * EN: Portable mutex/condition ownership and nonblocking acquisition.
 * RU: Переносимое владение mutex/condition и захват без ожидания.
 */


/* FlashVM: Portable platform adapter interface or implementation: channel sync.
 * File: ports/channel_sync.h.
 */

#ifndef FVM_CHANNEL_SYNC_H


/* Define FVM CHANNEL SYNC H constant or compilation switch. */
#define FVM_CHANNEL_SYNC_H
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct FVM_PortSync FVM_PortSync;


/* Process-wide registry lock; initialized by the platform before first use. */


/* Acquire port registry. */
void fvm_port_registry_lock(void);


/* Release port registry. */
void fvm_port_registry_unlock(void);


/* Allocate and initialize the platform mutex/condition pair. */
FVM_PortSync *fvm_port_sync_create(void);


/* Destroy port sync. */
void fvm_port_sync_destroy(FVM_PortSync *sync);


/* Acquire port sync. */
void fvm_port_sync_lock(FVM_PortSync *sync);


/* EN: Try borrowed sync once. Return one owning its mutex, zero without
 * ownership on contention or port error; never wait or touch the condition.
 * RU: Однократно захватить sync. Один означает владение мьютексом, ноль —
 * конфликт либо ошибку порта; ожидания и обращения к condition нет. */
int fvm_port_sync_try_lock(FVM_PortSync *sync);


/* Release port sync. */
void fvm_port_sync_unlock(FVM_PortSync *sync);


/* Wait for port sync. */
void fvm_port_sync_wait(FVM_PortSync *sync);


/* Wait with the mutex held, releasing it until a wake or a relative
 * monotonic timeout. Returns one on wake/timeout or zero on port error. */
int fvm_port_sync_wait_ms(FVM_PortSync *sync, unsigned long milliseconds);


/* Notify port sync all. */
void fvm_port_sync_signal_all(FVM_PortSync *sync);
#ifdef __cplusplus
}
#endif
#endif
