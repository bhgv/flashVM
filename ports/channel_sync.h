#ifndef FVM_CHANNEL_SYNC_H
#define FVM_CHANNEL_SYNC_H
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_PortSync FVM_PortSync;
    /* Process-wide registry lock; initialized by the platform before first use. */
    void fvm_port_registry_lock(void);
    void fvm_port_registry_unlock(void);
    FVM_PortSync *fvm_port_sync_create(void);
    void fvm_port_sync_destroy(FVM_PortSync *sync);
    void fvm_port_sync_lock(FVM_PortSync *sync);
    void fvm_port_sync_unlock(FVM_PortSync *sync);
    void fvm_port_sync_wait(FVM_PortSync *sync);
    void fvm_port_sync_signal_all(FVM_PortSync *sync);
#ifdef __cplusplus
}
#endif
#endif
