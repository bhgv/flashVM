# Sys descriptor ownership and retirement — runtime0.72

`--gc-worker --managed-owner-probe` additionally exercises actual retained Sys
construction paths. This is a diagnostic, not a new guest API or parallel mode.

`mem2array` must acquire one Tarray and one Tbyte owner and preserve input bytes.
`Sys_tokenize` must return two verified strings and acquire one Tlist and Tptr
owner per node. Source and delimiter C locals have explicit collector roots
across allocations. The generated native frame declaration is used directly;
no duplicate packed layout or new serialized representation is introduced.

After consuming outputs and removing temporary roots, the controller leaves
admission and joins the real Fast collector. Static descriptor counts must
return exactly to their saved baselines. The diagnostic does not dereference
retired arrays, lists or strings. Strong ownership and roots remain distinct.
Static descriptors still require balanced owners; static storage does not
justify skipping increments/decrements.

`test_service_counts.py RUNTIME COMPILER MODULE_INCLUDES` relinks private copies
against matching console objects. Its four controls omit respectively Tarray,
Tbyte and Tptr retains, or a Tbyte release. Missing retains stop at the exact
predicate before an unsafe release; missing release is detected after joined
Fast retirement. Only explicit status10 plus the expected predicate counts;
compiler/linker errors, signals, timeouts and sanitizer reports do not count.
The collector-visible test activation is atomic. Production objects are untouched.

Remaining parallelism boundary: NativeWorld still serializes managed admission.
This verifies the Sys owners implemented in runtime0.71, not concurrent arbitrary
payload access. Native object guards and owner slots do not automatically cover
all interpreter/JIT accesses. Collector graph traversal, allocator/publication,
roots and generated-machine-code ownership still require coordinated work.

In pinned ba26accb, x64 `x64pointercons` still emits ordinary add operations for
Heap.ref and Type.ref (amd64-emit.h lines2497/2512); ARM64 pointer construction
also uses load/add/store (arm64-emit.h around2070–2095). Existing serialized
admission is still required. A future fix should use checked native ownership
helpers or an explicitly proven equivalent atomic protocol on every supported
backend, preserving refusal, roots, marking and allocation failure semantics.
Merely adding a lock prefix on one backend does not solve overflow, source-slot
lifetime, publication or GC. No upstream snapshot was silently patched here.
