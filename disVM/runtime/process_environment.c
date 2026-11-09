/* Error bytes belong to Prog; native jump frames still belong to its OS owner. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "runqueue.h"
#include "process_environment.h"
#include "sys_capture.h"
#include "disvm/native_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Initialize only native storage, not a second heap or full Inferno namespace.
 * Parent buffers belong to the calling executor during the copy. */
void dvm_process_environment_init(Prog *process, Prog *parent)
{
    Osenv *environment = &((DVM_RuntimeProcess *)process)->environment;
    Osenv *source = parent ? parent->osenv : NULL;
    memset(environment,0,sizeof(*environment));
    environment->errstr = environment->errbuf1;
    environment->syserrstr = environment->errbuf0;
    if (source && source->errstr)
    {
        snprintf(environment->errstr,ERRMAX,"%s",source->errstr);
    }
    if (source && source->syserrstr)
    {
        snprintf(environment->syserrstr,ERRMAX,"%s",source->syserrstr);
    }
    process->osenv = environment;
    if (fegetenv(&((DVM_RuntimeProcess *)process)->floating_environment))
    {
        abort();
    }
}


/* A runtime Prog always points to its own wrapper environment. Copy no native
 * context/jump-frame pointers into it; binding lasts exactly the claimed turn. */
int dvm_process_environment_bind(Prog *process)
{
    if (!up || !process || process->osenv != &((DVM_RuntimeProcess *)process)->environment)
    {
        return 0;
    }
    if (fegetenv(&dvm_native_execution()->host_fp_environment) ||
        fesetenv(&((DVM_RuntimeProcess *)process)->floating_environment))
    {
        abort();
    }
    up->env = process->osenv;
    return 1;
}


/* The native default remains allocated through TLS teardown, unlike a retiring
 * Prog. Restore it before a destructor can report a new native host error. */
void dvm_process_environment_release(Prog *process)
{
    if (fegetenv(&((DVM_RuntimeProcess *)process)->floating_environment) ||
        fesetenv(&dvm_native_execution()->host_fp_environment))
    {
        abort();
    }
    up->env = &up->defenv;
}


/* Store one mapped guest string as this logical Prog's last host error. Capture
 * the original return span before string conversion can allocate/borrow REG.
 * Frame/source remain rooted by the claimed turn; no native release occurs.
 * Publish the full native WORD zero only after conversion and bounded copy. */
void Sys_werrstr(void *frame)
{
    F_Sys_werrstr call;
    char *destination;
    char *text;
    WORD result = 0;
    dvm_sys_capture(frame,&call,sizeof(call),offsetof(F_Sys_werrstr,ret),sizeof(*call.ret));
    destination = up->env->errstr;
    text = string2c(call.s);
    snprintf(destination,ERRMAX,"%s",text);
    memmove(call.ret,&result,sizeof(result));
}


/* Copy this bound OS owner's current logical error without consuming it. err
 * borrows size writable bytes; clamp to ERRMAX and preserve termination. No
 * allocation, managed access or throw; startup cleanup also permits absent up. */
void kgerrstr(char *err, uint size)
{
    const char *source = "<no-up>";
    if (!err || !size)
    {
        return;
    }
    if (size > ERRMAX)
    {
        size = ERRMAX;
    }
    if (up && up->env && up->env->errstr)
    {
        source = up->env->errstr;
    }
    snprintf(err, size, "%s", source);
}
