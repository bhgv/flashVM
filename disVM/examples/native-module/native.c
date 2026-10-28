/* Tutorial native exports: no UI policy, asynchronous callbacks or cross-VM pointers. */
#include "lib9.h" /* Import the selected host's scalar types and memmove declaration. */
#include "isa.h" /* Use the same instruction/cell target as the compiled caller. */
#include "interp.h" /* Obtain the actual VM module registration and allocation contracts. */
#include "nativedemo_native.h" /* Our Coco/R compiler generates frames, signatures and registration maps from .m. */


/* Register generated exports once after allocator/Sys initialization on the VM owner.
 * No parameters/result. Failure raises; this is static builtin registration, not dlopen. */
void dvm_nativedemo_initialize(void) /* The launcher calls this only when the optional tutorial is enabled. */
{ /* Registration does not create worker threads or retain a foreign VM context. */
    if (sizeof(LONG) != sizeof(uvlong)) /* Check the representation assumed by the wrap-preserving copy. */
    { /* Reject incompatible builds before any native export can run. */
        error("native demo: incompatible big width"); /* Deliver failure through Dis's error boundary. */
    } /* Both the generated frame and C big carrier must agree. */
    if (builtinmod("$Nativedemo", Nativedemomodtab, Nativedemomodlen) == nil) /* Register the exact generated table. */
    { /* A failed module construction must not advertise successful loading. */
        error("native demo: registration failed"); /* Raise on this owner; no C++ exception crosses the ABI. */
    } /* A later Limbo load resolves the registered module and checks signatures. */
} /* The host owns registered module metadata for its runtime lifetime. */


/* Add two borrowed big arguments with modulo-2^64 semantics, without signed C overflow.
 * frame is a VM-owned generated call frame; return storage belongs to its Limbo caller.
 * No allocation, blocking, retained pointers or shared mutable state are involved. */
void Nativedemo_add(void *frame) /* The generated export table supplies this callback. */
{ /* Access frame payloads bytewise, also permitting packed native boundaries. */
    const uchar *arguments = frame; /* Use generated offsets without dereferencing a packed C struct. */
    void *result_slot; /* Borrow the caller result address stored in the generated prefix. */
    LONG left; /* Copy the first signed-big argument into an aligned C local. */
    LONG right; /* Copy the second signed-big argument into an aligned C local. */
    uvlong result; /* Unsigned arithmetic makes wrapping defined by C. */
    memmove(&left, arguments + offsetof(F_Nativedemo_add, left), sizeof(left)); /* Borrow the argument value, not its slot address. */
    memmove(&right, arguments + offsetof(F_Nativedemo_add, right), sizeof(right)); /* Keep packed accesses free of aligned scalar loads. */
    result = (uvlong)left + (uvlong)right; /* Preserve all 64 bits, including MIN/MAX wrap cases. */
    memmove(&result_slot, arguments + offsetof(F_Nativedemo_add, ret), sizeof(result_slot)); /* Copy the return pointer, not a managed value. */
    memmove(result_slot, &result, sizeof(result)); /* Publish the resulting bit pattern into the caller's big slot. */
} /* Do not free or retain the borrowed frame. */


/* [GC:] Return a new VM-managed string on the sole execution owner.
 * frame/return slot are generated and caller-owned; retstr allocates before replacing the old result.
 * No input pointer survives this call. Allocation failure follows the normal Dis error path. */
void Nativedemo_greeting(void *frame) /* The export signature is greeting: fn(): string. */
{ /* [GC:] The native call frame remains a VM root until this callback returns. */
    void *result_slot; /* This is borrowed return storage, not a string heap owner. */
    memmove(&result_slot, (uchar *)frame + offsetof(F_Nativedemo_greeting, ret), sizeof(result_slot)); /* Read its generated position safely. */
    retstr("native module ready", result_slot); /* [GC:] The standard helper publishes a Dis string and retires the previous slot owner. */
} /* The caller now owns the returned value; no native raw string pointer is exported. */


/* Raise a recoverable Dis exception for a rejected status.
 * frame is borrowed; a successful void export writes no imaginary result cell.
 * error() unwinds VM state and must only be used on the bound VM execution owner. */
void Nativedemo_check(void *frame) /* Exported void callback, not a host-thread error-reporting API. */
{ /* No native resources are acquired, so this example needs no additional protected cleanup. */
    const uchar *arguments = frame; /* Borrow raw frame bytes; layout comes only from the generated header. */
    WORD status; /* Use WORD rather than assuming Limbo int equals host C int. */
    memmove(&status, arguments + offsetof(F_Nativedemo_check, status), sizeof(status)); /* Read the actual selected cell width safely. */
    if (status == 0) /* A zero status is the tutorial's explicit error condition. */
    { /* The caller may catch this without terminating the whole VM. */
        error("native demo: rejected"); /* Raise the documented error, with no successful dummy response. */
    } /* Nonzero status succeeds and changes no module state. */
} /* Void means there is no result value to synthesize. */
