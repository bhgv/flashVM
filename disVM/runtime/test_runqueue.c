/* Production FIFO against an independent array oracle and native owner identity. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Fail closed on a semantic invariant; valid/message are borrowed and no VM
 * error ABI is simulated. Positive fixture resources retire before success. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr,"FAIL run queue: %s\n",message);
        exit(1);
    }
}


/* Deliberately claim the same current Prog from another native context, then
 * require the continuation lease to reject it. argument borrows a rooted live
 * runtime allocation until join. Caller does not touch the FIFO concurrently. */
static void foreign_restore(void *argument)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long),NULL);
    check(context && dvm_native_context_bind(context),"foreign context bind");
    check(dvm_native_context_set_process(context,argument),"foreign process identity");
    check(!dvm_runqueue_resume(argument),"foreign continuation restoration accepted");
    check(!dvm_runqueue_append(argument),"leased process admitted by ordinary append");
    check(dvm_native_context_unbind(context),"foreign context unbind");
    check(dvm_native_context_destroy(context),"foreign context destroy");
}


/* Test the actual C scheduler FIFO, not a second queue implementation. No args;
 * array order/membership is an independent oracle over 20000 operations. Shared
 * fake Progs use the real retained layout and a separate root-list link chain;
 * this focused fixture does not execute managed bytecode or a collector. */
int main(void)
{
    DVM_RuntimeProcess nodes[DVM_RUNTIME_PROCESS_MAX];
    DVM_RuntimeProcess extra;
    Prog *oracle[DVM_RUNTIME_PROCESS_MAX];
    int member[DVM_RUNTIME_PROCESS_MAX];
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long),NULL);
    FVM_PortThread *thread;
    Prog *process;
    Prog *saved;
    unsigned int count = 0;
    unsigned int random = 12345;
    unsigned int step;
    unsigned int index;
    unsigned int chosen;
    memset(nodes,0,sizeof(nodes));
    memset(&extra,0,sizeof(extra));
    memset(member,0,sizeof(member));
    check(context && dvm_native_context_bind(context),"bind continuation owner");
    for (index = 0; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        nodes[index].process.pid = (int)index+1;
        nodes[index].process.next = index+1 < DVM_RUNTIME_PROCESS_MAX ? &nodes[index+1].process : NULL;
    }
    check(dvm_runqueue_append(&nodes[0].process),"first append");
    check(!dvm_runqueue_append(&nodes[0].process),"duplicate runnable admission accepted");
    for (index = 1; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        check(dvm_runqueue_append(&nodes[index].process),"fill bounded queue");
    }
    check(!dvm_runqueue_append(&extra.process),"process quota exceeded");
    process = dvm_runqueue_head();
    check(dvm_native_context_set_process(context,process),"current process");
    process->native64holds = 1;
    check(!dvm_runqueue_release_allowed(process) && !dvm_runqueue_suspend(process) &&
        dvm_runqueue_head() == process && !dvm_runqueue_owner(process),
        "synchronous Native64Caller hold was promoted to resumable");
    process->native64holds = 0;
    check(dvm_runqueue_release_allowed(process) && dvm_runqueue_release_allowed(NULL),
        "release admission after synchronous hold retirement");
    check(dvm_runqueue_suspend(process),"withdraw native continuation");
    check(dvm_runqueue_head() != process && dvm_runqueue_contains(process) == 0 &&
        process->state == Prelease,"native continuation remained runnable");
    check(dvm_runqueue_owner(process) == context,"same continuation owner retained");
    thread = fvm_port_thread_start(foreign_restore,process);
    check(thread != NULL,"start real foreign owner");
    fvm_port_thread_join(thread);
    check(dvm_runqueue_resume(process),"original owner restore");
    check(!dvm_runqueue_resume(process),"duplicate native restore accepted");
    for (index = 0; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        check(dvm_runqueue_pop(Psend) == &nodes[index].process,"FIFO fill/drain order");
    }
    check(!dvm_runqueue_pop(Pready),"empty pop accepted");
    for (step = 0; step < 20000; step++)
    {
        random = random*1664525U+1013904223U;
        chosen = (random >> 8)%DVM_RUNTIME_PROCESS_MAX;
        if (random%4 == 0)
        {
            process = &nodes[chosen].process;
            check(dvm_runqueue_append(process) == !member[chosen],"oracle append/membership");
            if (!member[chosen])
            {
                oracle[count++] = process;
                member[chosen] = 1;
            }
        }
        else if (random%4 == 1)
        {
            process = dvm_runqueue_pop(Precv);
            check(process == (count ? oracle[0] : NULL),"oracle pop order");
            if (count)
            {
                member[process->pid-1] = 0;
                count--;
                memmove(oracle,oracle+1,count*sizeof(*oracle));
                check(process->link == NULL && process->state == Precv,"blocked link/state");
            }
        }
        else if (count)
        {
            process = oracle[0];
            check(dvm_native_context_set_process(context,process),"oracle active identity");
            check(dvm_runqueue_suspend(process),"oracle native withdraw");
            check(dvm_runqueue_head() == (count > 1 ? oracle[1] : NULL) &&
                dvm_runqueue_contains(process) == 0,"oracle native visibility");
            check(!dvm_runqueue_append(process),"ordinary append bypassed native lease");
            check(dvm_runqueue_resume(process),"oracle native resume");
        }
        check(dvm_runqueue_head() == (count ? oracle[0] : NULL),"oracle head");
        check(dvm_runqueue_contains(&nodes[chosen].process) == member[chosen],"oracle membership");
    }
    if (count)
    {
        process = dvm_runqueue_head();
        saved = process->link;
        process->link = process;
        check(dvm_runqueue_contains(process) == -1,"diagnostic cycle not detected");
        process->link = saved;
    }
    for (index = 0; index < count; index++)
    {
        check(dvm_runqueue_pop(Pexiting) == oracle[index],"final oracle drain");
    }
    for (index = 0; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        check(nodes[index].process.next == (index+1 < DVM_RUNTIME_PROCESS_MAX ?
            &nodes[index+1].process : NULL),"native withdrawal changed GC root links");
    }
    check(!dvm_runqueue_head() && !dvm_runqueue_inflight(),"drained queue/continuations");
    check(dvm_native_context_unbind(context),"unbind original owner");
    check(dvm_native_context_destroy(context),"destroy original owner");
    puts("PASS actual runnable FIFO: 20000 oracle operations, real foreign owner refusal, independent root links, bounded diagnostics");
    return 0;
}
