/* Standalone public-API tutorial; the epoch below is simulated, not a real GC. */
#include "native_wait.h"
#include <stdio.h>
#include <stdlib.h>


/* Fail closed on a violated native protocol. ok/message are borrowed; success
 * returns without effects, failure ends this standalone process (no VM return). */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr,"native owner demo: %s\n",message);
        exit(EXIT_FAILURE);
    }
}


/* Read a caller-owned native scalar while active. user stays alive through the
 * wait round trip. This models the placement of a coherent collector epoch read;
 * it does not claim an actual collection took place in this standalone demo. */
static unsigned long epoch(void *user)
{
    return *(unsigned long *)user;
}


/* Model native work returning an I/O-style failure. user is only a private
 * native clock, not managed storage. No throw/cancel/VM access is permitted;
 * return -7 so main verifies that failure still restored heap ownership. */
static int native_failure(void *user)
{
    *(unsigned long *)user += 1;
    return -7;
}


/* Use only installed public headers/libraries. No args; returns success after
 * exact status/counter checks and full native teardown. Protocol/allocation
 * failure terminates this demonstration process, never resumes managed code. */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long),NULL);
    DVM_NativeWorld *world = dvm_world_create_serial(0);
    DVM_HeapOwner *owner;
    DVM_HeapOwnerStats state;
    unsigned long clock = 10;
    int status = 99;
    require(context && world,"allocation");
    require(dvm_native_context_bind(context),"bind TLS context");
    owner = dvm_native_owner_current();
    require(dvm_native_owner_attach(owner,world),"register immutable roots");
    require(dvm_native_owner_enter(owner),"enter exclusive heap turn");
    require(dvm_example_native_wait(owner,epoch,&clock,native_failure,&clock,&status),
        "audited native round trip");
    require(status == -7 && dvm_native_owner_statistics(owner,&state) &&
        state.phase == DVM_HEAP_ACTIVE && state.releases == 1 && state.reentries == 1 &&
        state.gc_progress == 1 && !state.pending_progress,"status/pairing/simulated epoch");
    require(dvm_native_owner_seal(owner),"close new registration");
    require(dvm_native_owner_leave(owner),"publish final roots");
    require(dvm_native_owner_detach(owner),"detach idle roots");
    require(dvm_native_context_unbind(context),"unbind TLS");
    require(dvm_native_context_destroy(context),"destroy native context");
    require(dvm_world_destroy(world),"destroy empty world");
    puts("PASS public native owner API: status=-7, matched reentry, simulated_epoch=1, retired");
    return 0;
}
