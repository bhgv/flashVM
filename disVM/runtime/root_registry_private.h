/* Private shared list geometry; not managed or serialized object layout. */
#ifndef DVM_ROOT_REGISTRY_PRIVATE_H
#define DVM_ROOT_REGISTRY_PRIVATE_H
#include "channel_sync.h"


/* One occurrence belongs either to the ordinary reusable list or one batch. */
typedef struct RootEntry
{
    void *key; /* Opaque borrowed identity; caller provides lifetime/authority. */
    struct RootEntry *next; /* Active bucket or ordinary reusable chain. */
    const void *batch; /* Nonnull batch identity excludes ordinary remove/reuse. */
} RootEntry;


/* Return process-lifetime mutex; create may allocate, zero never allocates. */
FVM_PortSync *dvm_root_registry_sync(int create);


/* Return stable bucket head; caller must hold the registry mutex while using. */
RootEntry **dvm_root_registry_bucket(const void *key);
#endif
