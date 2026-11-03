# Native MCALL ownership and count observations — runtime0.71

The native MCALL gate can acquire another owner between validation and frame
publication. The old local `references++` followed by a byte write could erase
that acquisition. Hosted executor code now retains through the checked atomic
Heap helper and pairs C return with its checked release. The return path still
does not destroy on LAST: its existing deferred-zero/C-stack policy is preserved.
An early validation read is not an ownership reservation.

The source preparer additionally converts 25 direct Heap and 20 direct Type
comparisons in the reviewed executor, one byte-field Type observation and four
Heap count reads. Sys core converts its one full header observation, four
byte-field Heap reads, debug count observation, three Type retains and six
Type preflights. Every expected occurrence is checked; source drift refuses.
Pinned upstream files and native header layouts remain unchanged.

`dvm_runtime_heap_count` reads only ref through the atomic/fallback primitive.
Its source must already be live, mapped and protected. Geometry refusal leaves
private output unchanged; geometry does not establish provenance or lifetime.
Count equality does not grant COW exclusivity. Other header fields and payload
remain subject to existing admission and lifetime rules.

Run `test_mcall_counts.py` with matching console runtime, Limbo compiler and
module include directory. A private linker wrapper adds an independent Module
owner at the actual Sys.sprint gate; the real executor must preserve it through
C return. A separately compiled private executor restores only the stale entry
write and must fail at the exact lost-owner predicate with status10 before the
uncertain owner is released. Compilation errors, signals and timeouts do not
count as detection. Production objects remain unchanged. This exercises C
execution; emitted JIT instructions are not covered by this control.

This closes reviewed count operations, not the parallel managed-heap project.
NativeWorld still serializes managed admission. Allocator/GC graph traversal,
root publication, payload access and remaining C/JIT operations require their
own protocol and evidence. No shared Dis64 ID, wire layout or file activation
changes are introduced.
