# Unicode legacy display-path names — v0.378

Loaded SWF4–6 display-path resolution no longer rejects non-ASCII bytes. Child
names and fallback property/method/accessor spellings use the pinned Flash
lowercase map per UTF-16 unit. The folded-name helper allocates no memory (resolver token/spelling copies remain
fallible), preserves counted
NUL/lone surrogates and treats scalar UTF-8 versus paired WTF-8 as the same units.
Dotted I can match ASCII i despite different byte lengths. Supplementary letters
are not scalar-folded; Deseret capitals/lowercase remain distinct. SWF7–8 and
unloaded host comparisons retain their existing case-sensitive behavior.

The same lowercase lookup now serves existing ASCII keyword recognition; child
priority, receiver identity, path delimiters and scoped roots remain unchanged.
Case-equivalent duplicate property/child selection retains existing traversal
order; complete duplicate-name/player semantics are not certified. Other levels,
removed references, source bytecode encodings and full property casing remain
open. No graph format, callback identity or public ABI is added.

Comparison sources at pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd:
[display child lookup](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object/container.rs),
[WStr comparison](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/wstr/src/ops.rs) and
[unit folding](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/wstr/src/utils.rs).
This is implementation comparison, not a new original Flash Player oracle pass.

Tests: movieclip_unicode_paths covers host/SWF4–8 Latin/Cyrillic/Greek/dotted I,
NUL, lone/paired surrogates, Deseret, aliases and child priority. The callbacks
fixture collects real garbage inside own/inherited script getters while retaining
a temporary intermediate object, and checks exceptions/errors. Allocation sweeps
cover child/property paths and verify allocation-free folded comparison under a
zero allocation allowance. jit_unicode_targets compares SetTarget2 SWF4–8 and
UTF-8 literal SetTarget SWF6–8 across interpreter/explicit/automatic native modes,
including throwing probes and restoration. All roadmap stages remain open.

Validation v0.378: all 781 AGG / 781 Cairo / 762 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and failed-test
reruns (two per profile). Seven restricted heap/global ASan+UBSan checks pass:
four new Unicode path/target checks plus existing resolver, JIT target and point
case checks. Stack ASan and LeakSanitizer remain disabled. Both revised scoped
constant headers pass strict C89 syntax checking. No new external SWF oracle
or foreign-OS runtime acceptance is claimed.
