#include "../channel_sync.h"
#include <pthread.h>
#include <stdlib.h>
struct FVM_PortSync { pthread_mutex_t mutex; pthread_cond_t condition; };
FVM_PortSync *fvm_port_sync_create(void) {
    FVM_PortSync *s=(FVM_PortSync *)malloc(sizeof(*s));
    if (!s) return 0;
    if (pthread_mutex_init(&s->mutex,0)) { free(s); return 0; }
    if (pthread_cond_init(&s->condition,0)) { pthread_mutex_destroy(&s->mutex); free(s); return 0; }
    return s;
}
void fvm_port_sync_destroy(FVM_PortSync *s) { pthread_cond_destroy(&s->condition); pthread_mutex_destroy(&s->mutex); free(s); }
void fvm_port_sync_lock(FVM_PortSync *s) { pthread_mutex_lock(&s->mutex); }
void fvm_port_sync_unlock(FVM_PortSync *s) { pthread_mutex_unlock(&s->mutex); }
void fvm_port_sync_wait(FVM_PortSync *s) { pthread_cond_wait(&s->condition,&s->mutex); }
void fvm_port_sync_signal_all(FVM_PortSync *s) { pthread_cond_broadcast(&s->condition); }
