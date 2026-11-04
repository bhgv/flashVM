"""Actual generic Heap leases: roots, strong owners, graph Cycles and error retirement."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Holds;
include "sys.m";
Holds: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    items := "held" :: nil;
    if (hd items != "held")
    {
        raise "held list";
    }
    sys->print("PASS lease guest\\n");
}
'''
WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "heap_hold.h"
#include "list_hold.h"
#include "root_registry.h"
#include "pointer_builder.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int live_capture; /* Private omission scope, never production admission. */
static __thread int testing; /* Only this diagnostic's root/owner protocol. */
static __thread int fail_after = -1; /* Native reservation allocation refusal. */
static __thread int mark_failure; /* One real capture's barrier refuses. */
static __thread int publish_failure; /* One real capture's root publication refuses. */
static __thread unsigned int marks; /* Actual successful-capture mark calls. */
static __thread int release_error; /* Consume actual owner, then throw once. */
static __thread DVM_HeapHold *releasing; /* Borrowed synchronous cleanup lease. */
static __thread void *released_value; /* Genuine retained value through destroy. */
static __thread unsigned long prior_roots; /* Independent roots outside the retiring lease. */
static __thread String **example_cell; /* Borrowed source slot through synchronous example call. */
static __thread int example_calls; /* Normal/throwing hosted example callbacks. */
static __thread int example_throw; /* Actual VM error requested by example callback. */
static __thread int body_error; /* Request an actual protected primary VM error. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);
unsigned long __real_dvm_refcount_load_u(const unsigned long *);
void __real_destroy(void *);
void dvm_example_with_heap_reference(const void *,Type *,int,void (*)(void *,void *),void *);
int __real_dvm_gc_field_mark(int *,const int *,int *);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *,void *const *,size_t);


/* Exact semantic stop precedes any unrooted graph collection or owner loss. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL heap hold: %s\n",message);
        exit(10);
    }
}


/* Only private source omissions query this diagnostic scope; production never does. */
int dvm_heap_hold_test_scope(void)
{
    return live_capture;
}


/* A capture's actual header count read must follow real slot exclusion. */
unsigned long __wrap_dvm_refcount_load_u(const unsigned long *counter)
{
    if (testing && live_capture)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"reference count read remains guarded");
    }
    return __real_dvm_refcount_load_u(counter);
}


/* No allocator attempt may occur with any object guard held during capture. */
void *__wrap_malloc(size_t size)
{
    if (testing)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()),"no allocation under guards");
        if (fail_after == 0)
        {
            fail_after = -1;
            return NULL;
        }
        if (fail_after > 0)
        {
            fail_after--;
        }
    }
    return __real_malloc(size);
}


/* Reservation calloc shares the exact same failure schedule as native malloc. */
void *__wrap_calloc(size_t count,size_t size)
{
    if (testing)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()),"no allocation under guards");
        if (fail_after == 0)
        {
            fail_after = -1;
            return NULL;
        }
        if (fail_after > 0)
        {
            fail_after--;
        }
    }
    return __real_calloc(count,size);
}


/* Native root metadata retirement happens only after complete capture exclusion. */
void __wrap_free(void *value)
{
    if (testing)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()),"no free under guards");
    }
    __real_free(value);
}


/* Observe actual barrier while still guarded; refusal must undo extra retain. */
int __wrap_dvm_gc_field_mark(int *color,const int *mutator,int *propagated)
{
    if (testing)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"mark remains guarded");
        marks++;
        if (mark_failure)
        {
            return 0;
        }
    }
    return __real_dvm_gc_field_mark(color,mutator,propagated);
}


/* Root registration occurs before slot unlock, including zero roots for H. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch,void *const *keys,size_t count)
{
    if (testing)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"root remains guarded");
        if (publish_failure)
        {
            return 0;
        }
    }
    return __real_dvm_runtime_root_batch_publish(batch,keys,count);
}


/* Real consuming retirement must see cleared storage, no guards and no own root.
 * A deliberate secondary error occurs AFTER actual destroy consumed its owner. */
void __wrap_destroy(void *value)
{
    int throwing = releasing && value == released_value;
    if (throwing)
    {
        check(!releasing->value && !releasing->roots &&
            !dvm_native_context_object_count(dvm_native_context_current()) &&
            dvm_runtime_root_count(D2H(value)) == prior_roots,"consumed lease before retirement");
    }
    __real_destroy(value);
    if (throwing && release_error)
    {
        release_error = 0;
        error("heap hold release diagnostic");
    }
}


/* Return1 or raise an actual primary error; argument is unused. */
static int body(void *unused)
{
    (void)unused;
    if (body_error)
    {
        error("heap hold primary diagnostic");
    }
    return 1;
}


/* Actual completed kprotect result, not a manufactured body_ok flag. */
static int finish(void *unused)
{
    int ok;
    (void)unused;
    ok = kprotect(body,NULL);
    dvm_runtime_heap_finish(releasing,ok);
    return 1;
}


/* Capture and check barrier/root/owner evidence before allowing collection. */
static void capture(void *cell,Type *type,DVM_HeapHold *hold)
{
    void *value;
    unsigned long owners;
    unsigned long roots;
    memmove(&value,cell,sizeof(value));
    owners = heapref(value);
    roots = dvm_runtime_root_count(D2H(value));
    marks = 0;
    testing = live_capture = 1;
    check(!dvm_runtime_heap_hold(cell,type,0,hold),"capture succeeds");
    check(!dvm_native_context_object_count(dvm_native_context_current()),"capture releases guards");
    testing = live_capture = 0;
    check(hold->value == value && hold->roots && marks == 1 &&
        heapref(value) == owners+1 && dvm_runtime_root_count(D2H(value)) == roots+1,
        "real capture owner and root");
}


/* The documented callback receives a borrowed real String outside all guards. */
static void example_body(void *value,void *argument)
{
    String *text = value;
    int *throwing = argument;
    check(text->len == 4 && !memcmp(text->Sascii,"held",4) &&
        !dvm_native_context_object_count(dvm_native_context_current()) &&
        dvm_runtime_root_count(D2H(text)) == 2,"hosted example borrowed root");
    example_calls++;
    if (*throwing)
    {
        error("heap hold example diagnostic");
    }
}


/* Use the exact documented C helper in a real protected native invocation. */
static int invoke_example(void *unused)
{
    (void)unused;
    dvm_example_with_heap_reference(example_cell,&Tstring,0,example_body,&example_throw);
    return 1;
}


/* Actual String/List objects, immediate construction pins and real Cycles/Fast.
 * Native local bytes are never fabricated Heap headers. Preserve Type baselines. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapHold text_hold = {0};
    DVM_HeapHold list_hold = {0};
    DVM_HeapHold empty = {0};
    DVM_HeapHold refused = {0};
    String *text;
    List *list = H;
    void *nil_value = H;
    void *null = NULL;
    void *text_key;
    void *list_key;
    int strings;
    int lists;
    int pointers;
    int index;
    __real_dvm_runtime_root_registry_probe(workers);
    strings = dvm_refcount_load_i(&Tstring.ref);
    lists = dvm_refcount_load_i(&Tlist.ref);
    pointers = dvm_refcount_load_i(&Tptr.ref);
    text = c2string("held",4);
    ptradd(D2H(text));
    dvm_runtime_pointer_cons(&text,&list);
    ptradd(D2H(list));
    text_key = D2H(text);
    list_key = D2H(list);
    check(dvm_runtime_heap_hold(&text,&Tlist,0,&refused) == exHeap &&
        !refused.value && !refused.roots && heapref(text) == 2,"typed refusal unchanged");
    check(dvm_runtime_heap_hold(&null,NULL,1,&refused) == exNilref &&
        dvm_runtime_heap_hold(&nil_value,NULL,0,&refused) == exNilref,"C NULL and forbidden H refuse");
    for (index = 0; index < 2; index++)
    {
        testing = 1;
        fail_after = index;
        check(dvm_runtime_heap_hold(&text,NULL,0,&refused) == exNomem && fail_after == -1 &&
            !refused.value && !refused.roots && heapref(text) == 2,"reservation refusal balanced");
        testing = 0;
    }
    for (index = 0; index < 2; index++)
    {
        testing = 1;
        mark_failure = index == 0;
        publish_failure = index == 1;
        check(dvm_runtime_heap_hold(&text,NULL,0,&refused) == exHeap &&
            !refused.value && !refused.roots && heapref(text) == 2 &&
            dvm_runtime_root_count(text_key) == 1,"capture refusal balanced");
        testing = mark_failure = publish_failure = 0;
    }
    for (index = 0; index < 2; index++)
    {
        DVM_HeapHold cleanup = {0};
        capture(&text,NULL,&cleanup);
        releasing = &cleanup;
        released_value = text;
        prior_roots = 1;
        release_error = 1;
        body_error = index;
        check(!kprotect(finish,NULL) && !cleanup.value && !cleanup.roots && heapref(text) == 2 &&
            !strcmp(getup()->env->errstr,index ? "heap hold primary diagnostic" : "heap hold release diagnostic"),
            "primary and retirement diagnostics");
        releasing = NULL;
        released_value = NULL;
    }
    body_error = 0;
    example_cell = &text;
    check(kprotect(invoke_example,NULL) && example_calls == 1 && heapref(text) == 2 &&
        dvm_runtime_root_count(text_key) == 1,"normal hosted example cleanup");
    example_throw = 1;
    check(!kprotect(invoke_example,NULL) && example_calls == 2 && heapref(text) == 2 &&
        dvm_runtime_root_count(text_key) == 1 && !strcmp(getup()->env->errstr,"heap hold example diagnostic"),
        "throwing hosted example cleanup");
    example_throw = 0;
    example_cell = NULL;
    capture(&text,&Tstring,&text_hold);
    capture(&list,&Tlist,&list_hold);
    check(dvm_runtime_heap_hold(&text,NULL,0,&text_hold) == exHeap,"live lease refuses overwrite");
    testing = 1;
    marks = 0;
    check(!dvm_runtime_heap_hold(&nil_value,NULL,1,&empty) && empty.value == H && empty.roots && !marks,
        "active empty H lease");
    testing = 0;
    dvm_runtime_heap_finish(&empty,1);
    check(!empty.value && !empty.roots,"nil finish clears lease");
    ptrdel(D2H(text));
    ptrdel(D2H(list));
    destroy(text);
    destroy(list);
    text = H;
    list = H;
    check(dvm_runtime_root_count(text_key) == 1 && dvm_runtime_root_count(list_key) == 1,
        "only lease roots remain");
    for (index = 0; index < 32; index++)
    {
        if (index == 16)
        {
            dvm_runtime_heap_finish(&text_hold,1);
            check(!dvm_runtime_root_count(text_key) && dvm_runtime_root_count(list_key) == 1,
                "indirect graph root only");
        }
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual Cycles");
    }
    {
        String *payload;
        List *node = list_hold.value;
        memmove(&payload,node->data,sizeof(payload));
        check(payload->len == 4 && !memcmp(payload->Sascii,"held",4) && heapref(payload) == 1,
            "indirect String survives Cycles");
    }
    dvm_runtime_heap_finish(&list_hold,1);
    check(!list_hold.value && !list_hold.roots,"final list lease consumed");
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual final Fast");
    check(!dvm_runtime_root_count(list_key) && !dvm_runtime_root_count(text_key),"final retirement roots drained");
    check(dvm_refcount_load_i(&Tstring.ref) == strings && dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers,"Type baselines restored");
    puts("PASS actual Heap holds: String/List/H,32 Cycles,indirect graph,Fast,error conservation");
}
'''


def main():
    """Link exact matching objects; run both guest modes/polls and source omissions."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    symbols = ['dvm_runtime_root_registry_probe','malloc','calloc','free','destroy',
               'dvm_gc_field_mark','dvm_runtime_root_batch_publish','dvm_refcount_load_u']
    with tempfile.TemporaryDirectory(prefix='actual Heap holds ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        example = root.parent/'examples/heap-hold/with_reference.c'
        if not example.exists():
            example = root.parents[1]/'examples/heap-hold/with_reference.c'
        # Compile the complete teaching translation unit once with exact headers.
        relink(runtime,work,example,0,[])
        # The execution fixture already supplies those non-idempotent native
        # headers; deduplicate includes only, keeping example bodies unchanged.
        example_body = re.sub(r'^#include[^\n]*\n','',example.read_text(),flags=re.M)
        wrapper.write_text(WRAPPER+'\n'+example_body)
        binary = relink(runtime,work,wrapper,0,symbols)
        for mode in ('--jit','--no-jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=60,
                                     env=dict(os.environ,TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if run.returncode or 'PASS actual Heap holds:' not in output or 'PASS lease guest' not in output:
                    raise RuntimeError(output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'heap_hold.c').read_text()
            controls = [
                ('dvm_refcount_retain_u(&heap->ref)','DVM_REFCOUNT_LIVE','real capture owner and root'),
                ('dvm_gc_field_mark(&heap->color, &mutator, &nprop)','1','real capture owner and root'),
                ('dvm_runtime_root_batch_publish(roots, &key, 1)','1','real capture owner and root'),
                ('dvm_managed_cells_begin(cell, (void *)cell, &access)','DVM_CELL_ACQUIRED','reference count read remains guarded'),
                ('dvm_cell_access_end(&access)','1','capture releases guards')]
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('Heap hold omission anchor drift')
                private = work/'heap_hold.c'
                private.write_text('extern int dvm_heap_hold_test_scope(void);\n'+original.replace(before,
                    '(dvm_heap_hold_test_scope() ? '+after+' : '+before+')',1))
                bad = relink(runtime,work,wrapper,index+1,symbols,heap_hold_source=private)
                run = subprocess.run([str(bad),'--no-jit','--gc-worker','--guarded-copies',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=60,
                    env=dict(os.environ,TMPDIR=str(work)))
                if run.returncode != 10 or 'FAIL heap hold: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('Heap hold omission missed: '+run.stdout+run.stderr)
    print('PASS actual Heap holds: C/JIT guest,both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; five exact-stage source controls'))


if __name__ == '__main__':
    main()
