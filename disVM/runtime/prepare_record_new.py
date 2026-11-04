"""Select guarded record constructors after validating the retained bodies."""
import hashlib
import re

EXPECTED = {'new': '5de4e3aa3cd1b9cf998676decf15c972e05d79a7b8a0e70e63a6314d2dbb2f98', 'newz': '77da78be5cd19e9925128b9f8f7f598b388cd7aeb233dc30edd83ff63dc08125'}  # SHA256 of each exact adapted opcode body.


IMPORTED = "5a6c439637da7a1f19732b3b454fed77a8674f838a4570203b0ffbe7eeecb718"  # Exact adapted MNEWZ body.


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
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {{
            error(exHeap);
        }}
        if (selected == 2)
        {{
            WORD captured;
            const char *problem = dvm_runtime_bytes_copy(DVM_R.s, &captured, sizeof(captured));
            if (problem)
            {{
                error((char *)problem);
            }}
            dvm_runtime_record_new(DVM_R.d, moduletype(DVM_R.M, captured), {clear});
            return;
        }}
    }}
"""
        source = source[:match.start()] + body[:at] + selection + body[at:] + source[match.end():]
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
    const char *problem = dvm_runtime_bytes_copy(request->index_cell, &index, sizeof(index));
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
    """Use the checked C dispatch for guarded constructors on hosted leaf backends."""
    anchor = "leafop(LeafOp *p, int op, int wordbytes)\n{\n"
    if source.count(anchor) != 1 or "Guarded allocation uses C" in source:
        raise ValueError("record leaf classification drift")
    selection = """    /* Guarded allocation uses C, including on an RC1 destination. */
    if (dvm_runtime_guarded_copies && (op == INEW || op == INEWZ || op == IMNEWZ || op == INEWA || op == INEWAZ ||
        op == INEWCB || op == INEWCW || op == INEWCL || op == INEWCF ||
        op == INEWCP || op == INEWCM || op == INEWCMP))
    {
        return 0;
    }
"""
    return '#include "shared_bytes.h"\n' + source.replace(anchor, anchor + selection, 1)

ARRAY = "705c18a3915a741a703ba881e09c14e12e84806792d273f8d70fd75ab9f020e6"  # Exact adapted array coordinator before guarded selection.


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
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {
            error(exHeap);
        }
        if (selected == 2)
        {
            WORD captured_index;
            WORD captured_count;
            Type *element;
            const char *problem = dvm_runtime_bytes_copy(DVM_R.m, &captured_index,
                sizeof(captured_index));
            if (problem)
            {
                error((char *)problem);
            }
            element = moduletype(DVM_R.M, captured_index);
            problem = dvm_runtime_bytes_copy(DVM_R.s, &captured_count, sizeof(captured_count));
            if (problem)
            {
                error((char *)problem);
            }
            dvm_runtime_array_new(DVM_R.d, element, captured_count, clear);
            return;
        }
    }
"""
    return source[:match.start()] + body[:at] + selection + body[at:] + source[match.end():]
