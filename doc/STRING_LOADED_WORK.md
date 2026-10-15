# Loaded String traversal work — v0.382

Loaded String.split no longer rejects the 65,537th result field. Its existing
AS limit conversion and SWF5 delimiter rules remain; unloaded host split retains
the separate 65,536-field allocation bound. No truncation substitutes for an
error. Ordinary memory/Array representation limits still apply.

Loaded split/indexOf/lastIndexOf enter the shared native execution-work scope
before receiver/argument coercion, so nested callbacks use the enclosing AVM1
budget. Split charges each scan position, delimiter comparison and emitted
UTF-16 unit. Encoding a supplementary character charges both units. Search
charges each candidate position and compared unit, including failed long-prefix
comparisons. Finite instruction limits and counter saturation return FVM_ERROR; zero remains
unlimited. Worker cancellation retains the shared periodic polling policy.
Unloaded host traversals retain their previous accounting policy.

[GC:] Split roots its unpublished Array through charged encoding and own stores.
Error/OOM unwinds partial work and root/depth scopes. Existing native method
barriers still block collection from conversion callbacks; a work poll does not
collect. Completed pinned results survive actual collection, and failed calls
can be retried. Public callback error-output policy is unchanged.

This is budgeted naive matching, not a new linear-time search algorithm. Input/
separator decoding, allocation helpers and other String methods are not newly
charged per unit. Full Flash matching/coercion/player conformance remains open.
No public ABI, callback IDs or graph formats change.

Tests: string_loaded_work covers host/SWF4–8 65,537 fields, old host rejection,
limits, long-prefix forward/backward searches, nested/unlimited/saturated budgets,
GC and a two-unit supplementary copy budget. string_loaded_work_allocation sweeps
split and both search allocations with GC/retry and restored native/root state.
string_loaded_work_callbacks verifies receiver→limit→delimiter conversion,
nested search budget failure, each conversion exception and the existing native
GC barrier. jit_string_work compares split/search success, budget failure and
retry through interpreter, explicit and automatic native CallMethod in SWF4–8.

Primary source comparison: [pinned Ruffle String split](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/string.rs).
This confirms the AS-limit/delimiter model; it is not a fresh original Flash
Player oracle pass. All nine roadmap stages remain open.

Validation v0.382: all 796 AGG / 796 Cairo / 777 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (two AGG, one minimal; Cairo passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new work/allocation/callback/native cases
plus legacy split/search/receiver and character-allocation checks. Stack ASan
and LeakSanitizer remain disabled. The revised String constants header passes
a strict C89 syntax probe. No new original external SWF oracle or foreign-OS
run is claimed. All nine roadmap stages remain open.
