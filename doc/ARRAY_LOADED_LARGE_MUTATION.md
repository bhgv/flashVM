# Large loaded generic Array mutations — v0.376

Loaded SWF4–8 generic shift/unshift/reverse/splice no longer reject source lengths
above 65,536 solely because of traversal size. Existing charged loops enforce
the shared AVM1 instruction budget and worker stop polling instead. Default
budget, configured memory limits, integer/output bounds and allocation failures
still apply; zero instruction limit retains its existing unlimited meaning.

Raw own-data traversal, flags, holes, accessor bypass and borrowed/native length
rules are unchanged. A prototype can force a real Array into generic dispatch;
large sources on this path now follow the same loaded rules. A failed mutation
can retain previous writes; roots/depth are restored and partial effects are not
rolled back. Host/unloaded generic mutations and generic sorting retain their
independent 65,536 bounds, named in array_traversal_constants.h.

array_loaded_large_mutation covers budget exhaustion above the old boundary
in SWF4–8, unchanged host rejection, and successful 65,537-slot borrowed/native
shift/unshift/reverse/splice, including inherited accessor bypass and retained/
removed tails. array_loaded_large_mutation_allocation injects every allocation
boundary until the configured budget is reached, collects after failure and
retries each method. All roadmap stages remain open; these are targeted checks,
not complete large-array or Flash Player conformance.

Validation v0.376: all 775 AGG / 775 Cairo / 756 minimal Linux Release checks
have passing evidence after nine ELF permission repairs and failed-test reruns
(4 AGG, 2 Cairo, 3 minimal). Five restricted heap/global ASan+UBSan checks pass,
including both new large-mutation checks, legacy unshift/splice flags and negative
end operations. Stack ASan and LeakSanitizer remain disabled. The new scoped
constant header also passes a strict C89 syntax probe.

As of v0.377 loaded sorting also uses charged captures instead of the 65536
restriction; host sorting limits remain. See [large sort](ARRAY_LOADED_LARGE_SORT.md).
