# Large loaded sorting and charged captures — v0.377

Loaded SWF4–8 native and borrowed sort/sortOn no longer reject a positive length
solely because it exceeds 65,536. Native sort links an empty temporary root frame
before incrementally capturing its own values. Each capture is charged before
allocation; partially built snapshots unwind on budget or allocation failure.
The existing item-building and quicksort passes remain charged. Descending
reversal and returned-index stores are now charged too, with a rooted unfinished
index array. This increases the work cost of existing loaded sorts.

The historical loaded left-pivot comparison order is retained, including its
quadratic worst case. Large ordered/equal inputs can exhaust the configured
budget; zero retains unlimited meaning. Output is not reordered before capture/
comparison succeeds, while existing callback side effects and partial writeback
remain nontransactional. Host native/generic sorting retain distinct 65,536
bounds. Native negative signed storage remains explicitly unsupported; generic
negative-length handling and no-op overloads retain their prior rules.

array_loaded_large_sort checks SWF4–8 native/borrowed sort and sortOn budget
failures, four successful 65,537-entry permutation/index sorts, source retention
for RETURNINDEXEDARRAY, output order, GC/root unwind and unchanged host bounds.
array_loaded_large_sort_allocation injects every allocation boundary until the
budget is reached in four native/borrowed sort/sortOn paths, collects and retries.
These checks do not close full Array/Flash compatibility or any roadmap stage.

Validation v0.377: all 777 AGG / 777 Cairo / 758 minimal Linux Release checks
have passing evidence after five ELF permission repairs (2/2/1) and six failed-test
reruns, including the dependent timezone profile runner. Six restricted heap/
global ASan+UBSan checks pass: both new large-sort checks, legacy allocation,
comparator, snapshot and no-op checks. Stack ASan and LeakSanitizer remain
disabled. The revised scoped constant header passes strict C89 syntax checking.
Original external SWF traces were not rerun; no new external oracle pass is claimed.
