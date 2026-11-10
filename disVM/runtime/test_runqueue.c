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


/* The standalone fixture has no Processctl callers or pending replies. */
void dvm_runtime_wait_request_finished(Prog *guest, DVM_OwnerMessage *message)
{
    USED(guest);
    USED(message);
    abort();
}

static Prog *expected_waiter; /* Borrowed fixture waiter during notification. */
static char *expected_cause; /* Exact cause expected in the published waiter. */
static int observed_waiter; /* Notification observed the completed wait. */


/* Observe the published waiter at the worker notification boundary. */
void dvm_managed_workers_wake(void)
{
    if (expected_waiter)
    {
        observed_waiter = expected_waiter->state == Pready && !expected_waiter->ptr &&
            !expected_waiter->chan && expected_waiter->kill == expected_cause &&
            dvm_runqueue_head() == expected_waiter;
    }
}

/* Queue tests use no native thread service. */
void dvm_managed_workers_wake_guest(Prog *guest)
{
    USED(guest);
    dvm_managed_workers_wake();
}



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
    check(!dvm_runqueue_finish(argument),"foreign native waiter dispatch finish accepted");
    check(dvm_native_context_set_process(context,NULL),"clear forged continuation identity");
    {
        Prog *other = dvm_runqueue_head();
        check(other && other != argument && dvm_runqueue_claim(other),
            "second owner could not claim a different runnable process during native wait");
        check(dvm_runqueue_dispatch_owner(argument) &&
            dvm_runqueue_dispatch_owner(argument) != context &&
            dvm_runqueue_dispatch_owner(other) == context,
            "simultaneous distinct managed-turn owners lost native affinity");
        check(dvm_runqueue_finish(other),"second owner turn finish during native wait");
    }
    check(dvm_native_context_unbind(context),"foreign context unbind");
    check(dvm_native_context_destroy(context),"foreign context destroy");
}


/* Inspect a claimed runnable head on a second real OS thread. The original
 * thread only joins while this function touches the FIFO, so no unsynchronized
 * heap access is implied. Even forged current-process identity grants no claim. */
static void foreign_dispatch(void *argument)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long),NULL);
    check(context && dvm_native_context_bind(context),"dispatch challenger bind");
    check(!dvm_runqueue_claim(argument),"foreign managed dispatch claim accepted");
    check(!dvm_runqueue_pop(Precv),"foreign owner removed claimed runnable head");
    check(dvm_native_context_set_process(context,argument),"forged dispatch identity");
    check(!dvm_runqueue_finish(argument),"foreign managed dispatch finish accepted");
    check(dvm_native_context_unbind(context),"dispatch challenger unbind");
    check(dvm_native_context_destroy(context),"dispatch challenger destroy");
}


/* After a complete blocked turn and wake, claim that same live Prog on another
 * OS owner, block it again and finish. Native continuation affinity must not be
 * confused with permanent affinity of ordinary managed scheduler turns. */
static void reassigned_dispatch(void *argument)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long),NULL);
    check(context && dvm_native_context_bind(context),"reassigned context bind");
    check(dvm_runqueue_claim(argument) && dvm_runqueue_dispatch_owner(argument) == context,
        "woken process could not transfer to another dispatcher");
    check(dvm_runqueue_pop(Psend) == argument && dvm_runqueue_finish(argument),
        "reassigned blocked turn finish");
    check(dvm_native_context_unbind(context),"reassigned context unbind");
    check(dvm_native_context_destroy(context),"reassigned context destroy");
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
    for (index = 0; index < 3; index++)
    {
        int states[] = {Psend, Precv, Palt};
        Prog *waiter = &extra.process;
        char *pending = "pending cancellation";
        char *cause = index == 1 ? "channel hangup" : NULL;
        waiter->state = states[index];
        waiter->ptr = &extra;
        waiter->chan = (Channel *)&extra;
        waiter->kill = pending;
        expected_waiter = waiter;
        expected_cause = cause ? cause : pending;
        observed_waiter = 0;
        check(dvm_runqueue_complete_channel_wait(waiter, cause) && observed_waiter,
            "worker notification sees fully completed channel wait");
        expected_waiter = NULL;
        check(!dvm_runqueue_complete_channel_wait(waiter, "duplicate") &&
            waiter->kill == expected_cause && dvm_runqueue_pop(Psend) == waiter,
            "duplicate channel completion leaves the published cause intact");
    }
    check(dvm_runqueue_append(&nodes[0].process),"first append");
    check(!dvm_runqueue_append(&nodes[0].process),"duplicate runnable admission accepted");
    for (index = 1; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        check(dvm_runqueue_append(&nodes[index].process),"fill bounded queue");
    }
    extra.process.state = Precv;
    extra.process.ptr = &extra;
    extra.process.chan = (Channel *)&extra;
    extra.process.kill = expected_cause;
    check(!dvm_runqueue_complete_channel_wait(&extra.process, "refused") &&
        extra.process.state == Precv && extra.process.ptr == &extra &&
        extra.process.chan == (Channel *)&extra && extra.process.kill == expected_cause,
        "full queue preserves blocked wait and pending cause");
    extra.process.ptr = nil;
    extra.process.chan = nil;
    extra.process.kill = nil;
    check(!dvm_runqueue_append(&extra.process),"process quota exceeded");
    process = dvm_runqueue_head();
    saved = process->link;
    process->link = NULL;
    check(!dvm_runqueue_claim(process) && !dvm_runqueue_pop(Precv) &&
        !dvm_runqueue_append(&extra.process) && dvm_runqueue_head() == process,
        "truncated FIFO changed before endpoint validation");
    process->link = saved;
    check(dvm_runqueue_claim(process),"claim runnable managed turn");
    check(dvm_runqueue_dispatch_owner(process) == context &&
        !dvm_runqueue_claim(process),"duplicate managed dispatch claim accepted");
    thread = fvm_port_thread_start(foreign_dispatch,process);
    check(thread != NULL,"start foreign dispatch challenger");
    fvm_port_thread_join(thread);
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
    check(!dvm_runqueue_append(&extra.process) && !dvm_runqueue_contains(&extra.process) &&
        dvm_runqueue_inflight() == 1, "native return slot was not reserved");
    check(!dvm_runqueue_claim(dvm_runqueue_head()),"nested managed dispatch claim accepted");
    check(!dvm_runqueue_finish(process),"native waiter finished managed turn before reentry");
    thread = fvm_port_thread_start(foreign_restore,process);
    check(thread != NULL,"start real foreign owner");
    fvm_port_thread_join(thread);
    nodes[DVM_RUNTIME_PROCESS_MAX-1].process.link = dvm_runqueue_head();
    check(!dvm_runqueue_resume(process) && dvm_runqueue_inflight() == 1 &&
        dvm_runqueue_owner(process) == context && process->state == Prelease,
        "corrupt FIFO consumed native return reservation");
    nodes[DVM_RUNTIME_PROCESS_MAX-1].process.link = NULL;
    check(dvm_runqueue_resume(process),"original owner restore");
    check(dvm_runqueue_contains(process) == 1, "native return accounting diverged");
    check(!dvm_runqueue_resume(process),"duplicate native restore accepted");
    check(dvm_runqueue_dispatch_owner(process) == context && dvm_runqueue_finish(process),
        "native reentry lost managed dispatch claim");
    check(!dvm_runqueue_finish(process),"duplicate managed dispatch finish accepted");
    for (index = 0; index < DVM_RUNTIME_PROCESS_MAX; index++)
    {
        check(dvm_runqueue_pop(Psend) == &nodes[index].process,"FIFO fill/drain order");
    }
    check(dvm_runqueue_append(&nodes[0].process), "append expected withdrawal fixture");
    check(!dvm_runqueue_pop_expected(&nodes[1].process, Precv) &&
        dvm_runqueue_head() == &nodes[0].process &&
        dvm_runqueue_pop_expected(&nodes[0].process, Psend) == &nodes[0].process,
        "foreign guest changed the runnable head");
    check(dvm_runqueue_append(&nodes[0].process) && dvm_runqueue_append(&nodes[1].process) &&
        dvm_runqueue_append(&nodes[2].process), "append direct selection fixture");
    check(dvm_runqueue_select_guest(&nodes[1].process) == &nodes[1].process &&
        dvm_runqueue_contains(&nodes[1].process) == 1 &&
        dvm_runqueue_select_guest(&nodes[2].process) == &nodes[2].process &&
        dvm_runqueue_pop(Psend) == &nodes[2].process &&
        dvm_runqueue_pop(Psend) == &nodes[1].process &&
        dvm_runqueue_pop(Psend) == &nodes[0].process,
        "direct selection damaged middle or tail queue links");
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
            check(dvm_runqueue_claim(process),"oracle active claim");
            check(dvm_runqueue_suspend(process),"oracle native withdraw");
            check(dvm_runqueue_head() == (count > 1 ? oracle[1] : NULL) &&
                dvm_runqueue_contains(process) == 0,"oracle native visibility");
            check(!dvm_runqueue_append(process),"ordinary append bypassed native lease");
            check(dvm_runqueue_resume(process),"oracle native resume");
            check(dvm_runqueue_finish(process),"oracle native turn finish");
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
    /* Blocked turns lose queue membership before their dispatch finishes. The
     * original owner alone can finish, then another OS thread may claim after
     * an ordinary wake/append, with the process/root allocation unchanged. */
    process = &extra.process;
    check(dvm_runqueue_append(process) && dvm_runqueue_claim(process),"blocked turn claim");
    check(dvm_runqueue_pop(Precv) == process && dvm_runqueue_dispatch_owner(process) == context,
        "blocking prematurely dropped dispatch owner");
    check(dvm_runqueue_append(&nodes[0].process),"prepare other runnable process during error cleanup");
    check(dvm_native_context_set_process(context,NULL),"clear process root during guarded cleanup");
    check(!dvm_runqueue_claim(&nodes[0].process),"cleared GC root permitted nested dispatch");
    check(!dvm_native_context_unbind(context),"claimed context unbound before finish");
    check(dvm_native_context_turn(context) == process && dvm_runqueue_finish(process),
        "error-root clearing prevented owner turn finish");
    check(!dvm_runqueue_dispatch_owner(process),"completed blocked turn retained owner");
    check(dvm_runqueue_pop(Psend) == &nodes[0].process,"drain other cleanup runnable process");
    check(dvm_runqueue_append(process),"wake unclaimed blocked process");
    thread = fvm_port_thread_start(reassigned_dispatch,process);
    check(thread != NULL,"start reassigned dispatcher");
    fvm_port_thread_join(thread);
    check(!dvm_runqueue_head() && !dvm_runqueue_dispatch_owner(process),"reassigned turn retained ownership");
    dvm_runqueue_enable_direct();
    Prog *first = &nodes[0].process;
    Prog *second = &nodes[1].process;
    check(dvm_runqueue_append(first) && dvm_runqueue_append(second) &&
        dvm_runqueue_ready() && !dvm_runqueue_head() && !first->link && !second->link,
        "direct readiness publishes no FIFO links");
    check(dvm_runqueue_select_guest(second) == second && dvm_runqueue_claim(second) &&
        dvm_runqueue_head() == second, "second guest dispatches directly");
    check(dvm_runqueue_suspend(second) && dvm_runqueue_ready() &&
        dvm_runqueue_resume(second) && !second->link, "direct native return preserves independent readiness");
    check(dvm_runqueue_finish(second) && dvm_runqueue_pop_expected(second, Pexiting) == second &&
        dvm_runqueue_pop_expected(first, Pexiting) == first && !dvm_runqueue_ready(),
        "direct guests retire independently of admission order");
    check(dvm_native_context_unbind(context),"unbind original owner");
    check(dvm_native_context_destroy(context),"destroy original owner");
    puts("PASS actual runnable FIFO: 20000 oracle operations, real foreign owner refusal, independent root links, bounded diagnostics");
    return 0;
}
