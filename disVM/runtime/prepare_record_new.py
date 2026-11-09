"""Select guarded record constructors after validating the retained bodies."""
import hashlib
import re

EXPECTED = {'new': '5de4e3aa3cd1b9cf998676decf15c972e05d79a7b8a0e70e63a6314d2dbb2f98', 'newz': '77da78be5cd19e9925128b9f8f7f598b388cd7aeb233dc30edd83ff63dc08125'}  # SHA256 of each exact adapted opcode body.


IMPORTED = "5a6c439637da7a1f19732b3b454fed77a8674f838a4570203b0ffbe7eeecb718"  # Exact adapted MNEWZ body.


SHARED_RECORD = r"""
/* EN: Keep the constructor's legacy retirement isolated from SHARED dispatch.
 * RU: Изолировать старое освобождение записи от диспетчеризации в SHARED. */
static int admitted_record_new(void *argument)
{
    int clear = *(int *)argument;
    WORD captured;
    const char *problem = dvm_runtime_bytes_snapshot(DVM_R.s, &captured, sizeof(captured));
    if (problem)
    {
        error((char *)problem);
    }
    dvm_runtime_record_new(DVM_R.d, moduletype(DVM_R.M, captured), clear);
    return 1;
}


/* EN: Accept an already shared dispatch and promote only on fallback.
 * RU: Принять текущий SHARED и повысить допуск только при резервном пути. */
static void shared_record_new(int clear)
{
    WORD index;
    if (dvm_runtime_record_shared_index(DVM_R.s, &index) &&
        dvm_runtime_record_try_shared(DVM_R.d, moduletype(DVM_R.M, index), clear))
    {
        return;
    }
    dvm_runtime_gc_dispatch_finish();
    int completed = kprotect(admitted_record_new, &clear);
    dvm_runtime_gc_dispatch_restore();
    if (!completed)
    {
        krethrow();
    }
}


"""


def adapt(source):
    """Insert worker-mode dispatch or raise on source drift."""
    for name, digest in EXPECTED.items():
        match = re.search(r"OP\(" + name + r"\)\n\{.*?\n\}", source, re.S)
        if not match or hashlib.sha256(match.group().encode()).hexdigest() != digest:
            raise ValueError("record constructor drift: " + name)
        body = match.group()
        at = body.index("\n{") + 2
        clear = 1 if name == "newz" else 0
        selection = f"""
    if (dvm_runtime_guarded_copies)
    {{
        if (dvm_runtime_gc_dispatch_is_shared())
        {{
            shared_record_new({clear});
            return;
        }}
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {{
            error("record constructor requires managed dispatch");
        }}
        if (selected == 2)
        {{
            int clear = {clear};
            admitted_record_new(&clear);
            return;
        }}
    }}
"""
        helper = SHARED_RECORD if name == "new" else ""
        source = source[:match.start()] + helper + body[:at] + selection + body[at:] + source[match.end():]
    source = adapt_array(adapt_imported(source))
    anchor = '#include "interp.h"'
    if source.count(anchor) != 1:
        raise ValueError("record declaration include drift")
    return source.replace(anchor, anchor + '\n#include "record_new.h"', 1)

def adapt_imported(source):
    """Keep the imported module owner live through resolution and publication."""
    match = re.search(r"OP\(mnewz\)\n\{.*?\n\}", source, re.S)
    if not match or hashlib.sha256(match.group().encode()).hexdigest() != IMPORTED:
        raise ValueError("imported record constructor drift")
    helper = r"""
/* [GC:] Borrow the held module and the caller's rooted destination. */
typedef struct ImportedRecord
{
    Modlink *module; /* Borrowed from an exclusive rooted module hold. */
    void *destination; /* Owning result slot in the current rooted frame. */
    void *index_cell; /* Borrowed native WORD operand in the rooted frame. */
} ImportedRecord;


/* Resolve the descriptor under its live module owner, then construct it. */
static int imported_record_new(void *argument)
{
    ImportedRecord *request = argument;
    WORD index;
    const char *problem = dvm_runtime_bytes_snapshot(request->index_cell, &index, sizeof(index));
    if (problem)
    {
        error((char *)problem);
    }
    dvm_runtime_record_new(request->destination, moduletype(request->module, index), 1);
    return 1;
}


/* [GC:] Hold the module across guarded index capture, monitor and cleanup. */
static void guarded_imported_record(void *source, void *index, void *destination)
{
    DVM_HeapHold module = {0};
    ImportedRecord request;
    int completed;
    const char *problem = dvm_runtime_heap_hold(source, &Tmodlink, 0, &module);
    if (problem)
    {
        error(problem == exNilref ? exModule : (char *)problem);
    }
    request.module = module.value;
    request.destination = destination;
    request.index_cell = index;
    completed = kprotect(imported_record_new, &request);
    dvm_runtime_heap_finish(&module, completed);
}
"""
    body = match.group()
    at = body.index("\n{") + 2
    selection = r"""
    if (dvm_runtime_guarded_copies)
    {
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {
            error(exHeap);
        }
        if (selected == 2)
        {
            guarded_imported_record(DVM_R.s, DVM_R.m, DVM_R.d);
            return;
        }
    }
"""
    return ('#include "shared_access.h"\n#include "heap_hold.h"\n'
            + source[:match.start()] + helper + body[:at] + selection + body[at:]
            + source[match.end():])


def adapt_leaf(source):
    """Use checked C dispatch for guarded allocation and channel transfers."""
    anchor = "leafop(LeafOp *p, int op, int wordbytes)\n{\n"
    if source.count(anchor) != 1 or "Guarded allocation and channel transfers" in source:
        raise ValueError("record leaf classification drift")
    selection = """    /* Guarded allocation and channel transfers use their C coordinators. */
    if (dvm_runtime_guarded_copies && (op == INEW || op == INEWZ || op == IMNEWZ || op == INEWA || op == INEWAZ ||
        op == INEWCB || op == INEWCW || op == INEWCL || op == INEWCF ||
        op == INEWCP || op == INEWCM || op == INEWCMP || op == ISEND || op == IRECV))
    {
        return 0;
    }
"""
    return '#include "shared_bytes.h"\n' + source.replace(anchor, anchor + selection, 1)

ARRAY = "705c18a3915a741a703ba881e09c14e12e84806792d273f8d70fd75ab9f020e6"  # Exact adapted array coordinator before guarded selection.


SHARED_ARRAY = r"""
/* EN: Keep legacy array replacement behind its admitted handler.
 * RU: Оставить замену массива в старом обработчике с допуском. */
static int admitted_array_new(void *argument)
{
    int clear = *(int *)argument;
    WORD index;
    WORD count;
    const char *problem = dvm_runtime_bytes_snapshot(DVM_R.m, &index, sizeof(index));
    if (problem)
    {
        error((char *)problem);
    }
    Type *element = moduletype(DVM_R.M, index);
    problem = dvm_runtime_bytes_snapshot(DVM_R.s, &count, sizeof(count));
    if (problem)
    {
        error((char *)problem);
    }
    dvm_runtime_array_new(DVM_R.d, element, count, clear);
    return 1;
}


/* EN: Use the current SHARED phase for a private array and an empty slot.
 * RU: Использовать текущий SHARED для приватного массива и пустой ячейки. */
static void shared_array_new(int clear)
{
    WORD index;
    WORD count;
    if (dvm_runtime_array_shared_operands(DVM_R.s, DVM_R.m, &count, &index) &&
        dvm_runtime_array_try_shared(DVM_R.d, moduletype(DVM_R.M, index), count, clear))
    {
        return;
    }
    dvm_runtime_gc_dispatch_finish();
    int completed = kprotect(admitted_array_new, &clear);
    dvm_runtime_gc_dispatch_restore();
    if (!completed)
    {
        krethrow();
    }
}


"""


def adapt_array(source):
    """Capture the full count and descriptor index before the first callback."""
    match = re.search(r"static void\nnewarray\(int clear\)\n\{.*?\n\}", source, re.S)
    if not match or hashlib.sha256(match.group().encode()).hexdigest() != ARRAY:
        raise ValueError("array constructor drift")
    body = match.group()
    at = body.index("\n{") + 2
    selection = r"""
    if (dvm_runtime_guarded_copies)
    {
        if (dvm_runtime_gc_dispatch_is_shared())
        {
            shared_array_new(clear);
            return;
        }
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {
            error("array constructor requires managed dispatch");
        }
        if (selected == 2)
        {
            admitted_array_new(&clear);
            return;
        }
    }
"""
    return source[:match.start()] + SHARED_ARRAY + body[:at] + selection + body[at:] + source[match.end():]
