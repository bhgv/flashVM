# Synchronized external-root metadata — runtime0.78

Hosted ptradd/ptrdel/ptrmark now use a separate native registry. Its hash buckets,
active entries and reusable-node list have their own port mutex. A fully
initialized mutex is published once by native acquire/release pointer operations
or the port registry-lock fallback. The mutex and metadata cache keep their
existing process lifetime; no concurrent close or new VM-instance boundary is
introduced. Pinned upstream and Heap/Type/wire layouts remain unchanged.

Adding a cached node uses one critical section. If native metadata storage is
needed, the root mutex is released before malloc and reacquired for publication.
Allocation refusal publishes nothing; ptradd raises its original VM error only
AFTER the native function returns with no lock held. Removal unlinks one matching
occurrence and clears its key before reuse. Duplicate registrations form a
multiset, not independently addressable handles or extra strong references.

The native algorithm never dereferences keys or invokes referent destruction.
Locked traversal borrows each key only during a bounded nonallocating visitor;
that visitor must return normally and cannot wait, reenter the registry, call
the VM or throw. Actual ptrmark's visitor only performs Setmark under the existing
stopped-world collector protocol. Mutex publication never waits for root exclusion
while holding the port registry mutex. Callers must not hold that registry mutex
or recursively enter this registry.

This is the process-global hosted GC root table. In a running VM every key must
be a valid independently protected managed Heap header. Opaque native-key tests
run separate processes with no collector; they are not managed heaps or a new
permission to register arbitrary pointers in a VM. Registration/count observation
does not retain a referent, reserve a root, prove COW uniqueness or grant payload
access. Root publication and source lifetime still require managed admission or
stopped-world exclusion. A mutex alone does not make roots added after a collector
snapshot safe. General managed admission remains serialized.

The independent C test races cold mutex initialization and real bucket/free-list
updates with8 OS writers,16000 remove/add pairs and1000 locked traversals. Keys
are immutable native stack objects kept alive through complete joins. Both atomic
and forced-lock mutex publication backends run. A serial fresh-process test
refuses metadata allocation after an existing registration and checks exact
restoration, duplicates and final removal. Actual-source controls remove table
publication/removal or move malloc under the mutex; each requires explicit
semantic10 before unsafe continuation. Compiler/signal/timeout failures never
count as detection. Six controls cover both publication backends.

A separate full-runtime probe uses an actual managed String: duplicate root
counts2->1 while Heap ownership stays1,32 real Cycles preserve contents, last
root removal plus real Fast restores the static Type baseline. It uses no bare
native managed-pointer workers. Existing managed-ticket root-removal controls
still prove that losing real roots permits real GC reclamation.

```sh
python3 disVM/runtime/test_root_registry.py
CFLAGS='-O1 -g -fsanitize=thread -fno-omit-frame-pointer' \
    python3 disVM/runtime/test_root_registry.py --positive-only
```

Joined positives are TSan evidence for metadata and its port locks. Deliberate
negative stops are excluded. Actual VM tests cover the adapter/collector link;
these do not certify a parallel allocator, concurrent marking, graph mutation,
moving pins or every C/JIT access. The source SDK includes the native module,
adapter, probe and runnable standalone checks.
