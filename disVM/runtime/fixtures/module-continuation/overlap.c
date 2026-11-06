/* Private test gates make simultaneous admission independent of host timing. */
#include "lib9.h"
#include "interp.h"
#include "isa.h"
#include "pool.h"
#include "disvm/native_owner.h"
#include "disvm/native_context.h"
#include "disvm/gc_workers.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define GUEST_COUNT 40 /* Independent guest threads reaching one copy each. */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Gate state. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Both test barriers. */
static unsigned arriving; /* Guests with published roots outside NativeWorld. */
static unsigned admitted; /* Guests that have acquired actual SHARED admission. */

int __real_dvm_native_owner_enter_shared(DVM_HeapOwner *owner);
int __real_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind);


/* Force the queued path so every test guest reaches the same barrier. */
int __wrap_dvm_native_owner_try_share(DVM_HeapOwner *owner, unsigned long epoch)
{
    (void)owner;
    (void)epoch;
    return 0;
}


/* Inject one reservation refusal before touching frame ownership. */
int __real_dvm_runtime_rc_transfer_frame(const void *frame, const Type *type, void *extra);
int __wrap_dvm_runtime_rc_transfer_frame(const void *frame, const Type *type, void *extra)
{
    static int refused; /* Protected by the test mutex. */
    int inject;
    pthread_mutex_lock(&mutex);
    inject = !refused;
    refused = 1;
    pthread_mutex_unlock(&mutex);
    if (inject)
    {
        fprint(2, "FRAME retirement retry injected\n");
        return -1;
    }
    return __real_dvm_runtime_rc_transfer_frame(frame, type, extra);
}


/* Observe only successful shared pops of actual exception frames. */
int __real_dvm_runtime_exception_pop(Prog *process, const uchar *handler_frame);
int __wrap_dvm_runtime_exception_pop(Prog *process, const uchar *handler_frame)
{
    const uchar *extent = DVM_R.EX;
    int popped = __real_dvm_runtime_exception_pop(process, handler_frame);
    if (popped >= 2)
    {
        fprint(2, "FRAME exception popped in shared mode\n");
    }
    if (popped && extent != DVM_R.EX)
    {
        fprint(2, "FRAME exception crossed a stack extent\n");
    }
    return popped;
}


/* Count only successful installations on the shared path. */
int __real_dvm_runtime_exception_install(Prog *process, Type *cleanup, void *slot, Inst *target);
int __wrap_dvm_runtime_exception_install(Prog *process, Type *cleanup, void *slot, Inst *target)
{
    int installed = __real_dvm_runtime_exception_install(process, cleanup, slot, target);
    if (installed)
    {
        fprint(2, "FRAME handler installed in shared mode\n");
    }
    return installed;
}


/* Record real shared handler lookup admission. */
int __real_dvm_runtime_exception_search_enter(Prog *process);
int __wrap_dvm_runtime_exception_search_enter(Prog *process)
{
    int typed = process->exval != H && D2H(process->exval)->t != &Tstring;
    int entered = __real_dvm_runtime_exception_search_enter(process);
    if (entered)
    {
        fprint(2, "FRAME handler search entered shared mode\n");
        if (typed)
        {
            fprint(2, "FRAME typed handler search entered shared mode\n");
        }
    }
    return entered;
}


/* Move the outer frame and its array argument ahead of the inner call.
 * The exception table range and all instruction addresses stay unchanged. */
void __real_xec(Prog *process);
void __wrap_xec(Prog *process)
{
    static int patched;
    if (!patched)
    {
        static const int pattern[] = {IMFRAME, ILEA, IMCALL, IMFRAME, INEWA,
            IINDL, IMOVW, IMOVP, IMOVP};
        Module *module = process->R.M->m;
        int matches = 0;
        for (int index = 0; index <= module->nprog - 9; index++)
        {
            Inst *code = module->prog + index;
            int step = 0;
            while (step < 9 && code[step].op == pattern[step])
            {
                step++;
            }
            if (step == 9)
            {
                Inst nested[3];
                memmove(nested, code, sizeof(nested));
                memmove(code, code + 3, 6 * sizeof(*code));
                memmove(code + 6, nested, sizeof(nested));
                matches++;
            }
        }
        if (matches != 1)
        {
            abort();
        }
        patched = 1;
    }
    __real_xec(process);
}


/* Confirm that the altered program reaches the shared pending-frame path. */
uchar *__real_dvm_runtime_exception_pending(Prog *process, uchar *last);
uchar *__wrap_dvm_runtime_exception_pending(Prog *process, uchar *last)
{
    uchar *remaining = __real_dvm_runtime_exception_pending(process, last);
    if (last != remaining)
    {
        fprint(2, "FRAME pending argument retired in shared mode\n");
    }
    return remaining;
}


/* Record transfer of the typed owner while search remains shared. */
int __real_dvm_runtime_exception_convert(Prog *process);
int __wrap_dvm_runtime_exception_convert(Prog *process)
{
    int converted = __real_dvm_runtime_exception_convert(process);
    if (converted)
    {
        fprint(2, "FRAME exception converted in shared mode\n");
    }
    return converted;
}


static __thread int unicode_message; /* Keep Unicode verification separate from the fault. */
static __thread int use_emergency_message; /* A forced failure on this guest. */

/* Refuse two shared message allocations to exercise the retained fallback. */
void *__real_dvm_heap_pool_try_alloc(Pool *pool, ulong size);
void *__wrap_dvm_heap_pool_try_alloc(Pool *pool, ulong size)
{
    static int refused; /* Protected by the test mutex. */
    pthread_mutex_lock(&mutex);
    int inject = refused < 2 && !unicode_message;
    refused += inject;
    pthread_mutex_unlock(&mutex);
    if (inject)
    {
        use_emergency_message = 1;
        fprint(2, "FRAME shared message allocation refused\n");
        return NULL;
    }
    return __real_dvm_heap_pool_try_alloc(pool, size);
}


static __thread Prog *monitored_guest; /* Borrowed only during this guest's message call. */

/* Confirm that profiling sees the initialized, rooted message after reentry. */
static void monitor_message(int event, void *heap, ulong bytes)
{
    DVM_HeapOwnerStats state;
    (void)bytes;
    if (event || !monitored_guest || monitored_guest->exval == H ||
        D2H(monitored_guest->exval) != heap)
    {
        return;
    }
    if (!dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE || ((Heap *)heap)->t != &Tstring)
    {
        abort();
    }
    fprint(2, "FRAME rooted message observed by heap monitor\n");
}


/* Count successful shared allocations through the exception entry point. */
int __real_dvm_runtime_exception_message(Prog *process, const char *message);
int __wrap_dvm_runtime_exception_message(Prog *process, const char *message)
{
    static unsigned calls; /* Protected by the test mutex. */
    pthread_mutex_lock(&mutex);
    if (!calls)
    {
        heapmonitor = monitor_message;
    }
    int unicode = ++calls == 2;
    pthread_mutex_unlock(&mutex);
    monitored_guest = process;
    unicode_message = unicode;
    int allocated = __real_dvm_runtime_exception_message(process, unicode ? "A\303\251" : message);
    unicode_message = 0;
    monitored_guest = NULL;
    if (use_emergency_message)
    {
        static String *first; /* Identity only. The test guests stay live at the barrier. */
        String *value = process->exval;
        if (!allocated || value == H || value->len != (int)strlen(message) ||
            memcmp(value->Sascii, message, value->len))
        {
            abort();
        }
        pthread_mutex_lock(&mutex);
        if (first == value)
        {
            abort();
        }
        first = value;
        pthread_mutex_unlock(&mutex);
        use_emergency_message = 0;
        fprint(2, "FRAME TLS emergency message verified\n");
    }
    if (unicode && allocated)
    {
        String *value = process->exval;
        if (value->len != -2 || value->Srune[0] != 'A' || value->Srune[1] != 0xe9)
        {
            abort();
        }
        fprint(2, "FRAME Unicode exception message verified\n");
    }
    if (allocated)
    {
        fprint(2, "FRAME exception message allocated in shared mode\n");
    }
    return allocated;
}


/* Wait on one test-only counter. All guests own separate TLS contexts. */
static void meet(unsigned *count, const struct timespec *deadline)
{
    pthread_mutex_lock(&mutex);
    ++*count;
    pthread_cond_broadcast(&changed);
    while (*count < GUEST_COUNT)
    {
        if (deadline)
        {
            if (pthread_cond_timedwait(&changed, &mutex, deadline))
            {
                break;
            }
        }
        else
        {
            pthread_cond_wait(&changed, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
}


/* Delay each guest before and after the real admission. The copy still uses
 * the production executor, range guards and exclusive return path. */
int __wrap_dvm_native_owner_enter_shared(DVM_HeapOwner *owner)
{
    Prog *guest = dvm_native_context_process(dvm_native_context_current());
    int result;
    if (!guest || guest->pid == 1)
    {
        return __real_dvm_native_owner_enter_shared(owner);
    }
    meet(&arriving, NULL);
    result = __real_dvm_native_owner_enter_shared(owner);
    if (result)
    {
        struct timespec deadline;
        /* A queued exclusive ticket can separate SHARED groups. Never
         * make that ticket wait indefinitely on a test-only barrier. */
        if (clock_gettime(CLOCK_REALTIME, &deadline) == 0)
        {
            deadline.tv_sec++;
            meet(&admitted, &deadline);
        }
    }
    return result;
}


/* Delay collector requests until the artificial barriers release. The normal
 * collector can then run and drain ownership during the rest of the guest run. */
int __wrap_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    int ready;
    pthread_mutex_lock(&mutex);
    ready = admitted >= GUEST_COUNT;
    pthread_mutex_unlock(&mutex);
    return !ready || __real_dvm_gc_workers_request(workers, kind);
}
