/* Copyright (c) 2026 bhgv
 * EN: Real construction, ownership and channel transfer cleanup regressions.
 * RU: Регрессии создания объектов, владения и очистки передач по каналам.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "runtab-legacy.h"
#include "disvm/native_context.h"
#include "raise.h"
#include "record_new.h"
#include "heap_commit.h"
#include "shared_pointer.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "runqueue.h"
#include "channel_match.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <pthread.h>
#include "channel_sync.h"
#include "pointer_builder.h"
#include <stdlib.h>
#include <string.h>

#include "fixtures/record-new/construction.inc"
#include "fixtures/record-new/channel-lifecycle.inc"
#include "fixtures/record-new/channel-transfer.inc"
#include "fixtures/record-new/alt-lifetime.inc"
#include "fixtures/record-new/mapped-buffer.inc"


/* Use real Type and List storage, then check counts after deferred retirement. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    uchar map = 0x80;
    List *container = H;
    void *empty = H;
    void *published;
    void *current;
    int array_refs;
    int channel_refs;
    __real_dvm_runtime_root_registry_probe(workers);
    array_refs = dvm_refcount_load_i(&Tarray.ref);
    channel_refs = dvm_refcount_load_i(&Tchannel.ref);
    record = dtype(freeheap,sizeof(void *)+sizeof(WORD),&map,1);
    check(record != NULL,"real descriptor");
    dvm_runtime_pointer_cons(&empty,&container);
    ptradd(D2H(container));
    destination = container->data;
    dvm_runtime_record_new(destination,record,0);
    heapmonitor = monitor;
    mode = 1;
    check(kprotect(construct,NULL) && stale == 1 && callbacks == 1,"same candidate after stale");
    memmove(&published,destination,sizeof(published));
    mode = 2;
    check(!kprotect(construct,NULL) && callbacks == 2 &&
        strstr(getup()->env->errstr,"record monitor primary"),"monitor error preserved");
    memmove(&current,destination,sizeof(current));
    check(current == published,"monitor error preserves destination");
    heapmonitor = NULL;
    collect(workers,DVM_GC_CYCLES);
    memmove(&current,destination,sizeof(current));
    check(current == published && D2H(current)->t == record,"published record survives Cycles");
    heapmonitor = monitor;
    imported_case(workers, 0);
    heapmonitor = monitor;
    imported_case(workers, 1);
    array_case(workers);
    channel_case();
    mapped_channel_move_case();
    mapped_channel_send_case();
    mapped_channel_receive_case();
    prog_hold_race();
    channel_drain_case();
    channel_finalize_case(workers);
    direct_channel_failure();
    buffered_channel_completion();
    buffer_owner_case();
    buffer_wait_case();
    alt_publication_failure();
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&record->ref) == 1 &&
        dvm_refcount_load_i(&Tarray.ref) == array_refs &&
        dvm_refcount_load_i(&Tchannel.ref) == channel_refs, "descriptor baselines restored");
    freetype(record);
    record = NULL;
    destination = NULL;
    puts("PASS rooted record construction");
}
