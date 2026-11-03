"""Move reviewed hosted external-root metadata into its native registry."""
import hashlib

EXPECTED = '7f1499b20e5feb857b2c80a74e159d6841b48408308d96ce86511dc599d68f2b'  # Reviewed original ptradd/ptrdel/ptrmark cluster.


# source is private retained gc.c text. Reject drift before dropping its old
# table. New native module synchronizes metadata; GC still owns stopped-world
# marking and source publication/lifetime. No Heap layout or wire change.
def adapt(source):
    begin = source.index('typedef struct Ptrhash Ptrhash;')
    end = source.index('\n};', begin)+3
    source = source[:begin]+source[end:]
    begin = source.index('static\tPtrhash\t*ptrtab[PTRHASH];')
    end = source.index('\nvoid\nnoptrs(', begin)
    old = source[begin:end]
    if hashlib.sha256(old.encode()).hexdigest() != EXPECTED:
        raise ValueError('external-root cluster changed; review registry adapter')
    body = '''/* [GC:] Add fails only before publication; raise outside every native lock. */
void
ptradd(Heap *value)
{
    if(!dvm_runtime_root_add(value)) {
        error("ptradd malloc");
    }
}


/* [GC:] Caller preserves referent/collector protocol; metadata removal is native. */
void
ptrdel(Heap *value)
{
    (void)dvm_runtime_root_remove(value);
}


/* [GC:] Bounded nonallocating visitor, only under stopped-world collector role. */
static void
mark_external_root(void *value, void *argument)
{
    Heap *heap = value;
    (void)argument;
    Setmark(heap);
}


/* [GC:] Registry lock stabilizes entries, not Heap lifetime or mutator admission. */
static void
ptrmark(void)
{
    dvm_runtime_roots_visit(mark_external_root, nil);
}
'''
    source = source[:begin]+body+source[end:]
    return '#include "root_registry.h"\n'+source
