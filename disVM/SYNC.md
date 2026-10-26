# Current synchronization — v0.1639

Experiment `80f859e`: exact fixed operand/frame guards and AMD64/ARM64 qword
MOVM/HEADM kernels were imported from their owning Inferno commit. Native copies
retain the 0..256 domain and C fallback for larger records. Backward/forward
blocks and 0..7-byte tails preserve overlap order; no empty payload is accessed.
The three new C/native-emulation drivers are registered in our CTest selection.
[Upstream contract](upstream/libinterp/tests/README.fixed-slots-copy-chunks.md).

No additional owning native defect was confirmed during this import. Our earlier
reverse branch/JMP repair remains in experiment; no duplicate reverse commit was
created. Compiler 0.22 lexical arm/scoping extensions remain FlashVM-only.
[Compiler guide](../fir/v1/doc/TERMINAL_ARMS.md).

AMD64/ARM64 availability remains 164/173, with partial operand coverage. Emulated
ARM64 checks do not establish hardware execution or full Inferno boot.

---

# Current synchronization — v0.1638

Experiment: `02b7b7f96183d30f6c75b7415197ecc986e5d411`. 531 exact core/reference
files and 174 bootstrap files. Our reverse integer-branch/JMP preflight is
published in experiment and imported byte-for-byte. It bounds used comparison
and target slots, retains false-target/JMP-source nonaccess, computes the ARM64
inverse relocation and preserves native FP/PC state on decline.
[Owning contract](upstream/libinterp/tests/README.branch-slot.md).

New countof/in/delete language operations, FIR IDs and hosted Bigtable helpers
are FlashVM extensions and were not sent to Inferno. [Compiler contract](../fir/v1/doc/TABLE_MUTATION.md).
The real ide.dis application entry runs Limbo policy on this DisVM and FlashVM
rendering through typed channels. [Application guide](../ide/dis/app/README.md).

Native availability remains AMD64/ARM64 164/173, with partial operand coverage.
No native ISA/container/WORD ABI change is introduced. ARM64 checks are instruction
emulation; hardware execution and full Inferno boot remain separate acceptance.

Incoming e14f034 adds independent integer branch C/RX and JIT checkpoints.
Incoming 02b7b7f guards nine numeric commands and accepts all normal exact
CVTFX power scales. All four additional native/emulation drivers are registered.

---

# Current synchronization — v0.1637

Experiment: `14c3d2c2736936eade4eadfa25e87c8ce5e404ec`. 519 exact core/reference files and 174 bootstrap
files. Incoming 7254e9e checks eighteen transfer/conversion paths; 2190eea checks
24 integer ALU/division paths. Their production implementations are retained.
Our initially independent division repair was not published over the concurrent,
broader 2190eea implementation.

Our reverse 14c3d2c adds six real-branch source/conditional-target guards and
independent twelve-command division/real checks. False predicates ignore targets;
taken targets fit before PC publication. ARM64 relocates the inverse branch over
the complete guarded sequence. The same reverse commit preserves transfercheck
and integercheck when standalone numeric oracles are embedded in native WORD
model diagnostics. [Arithmetic contract](upstream/libinterp/tests/README.arithmetic-slot.md).
[Transfer](upstream/libinterp/tests/README.transfer-slot-preflight.md) ·
[Integer ALU](upstream/libinterp/tests/README.integer-slot-preflight.md).

All three independent checkpoint families are registered: 190 selected DisVM
tests. [New commented Limbo example](../fir/v1/examples/tables/division.b) covers
MIN/-1, signed remainder and an unselected zero-divisor arm. SDK acceptance uses
fresh C99 compiler/reference clients, original/optimized FIR and both VM modes.

Native availability stays AMD64/ARM64 164/173; operand coverage is partial. Native
ABI/private capacity stay unchanged. Arithmetic spans do not prove arbitrary
mapped allocation ownership. ARM64 evidence is instruction emulation. Compiler
language extensions remain in FlashVM; no full Inferno boot/hardware claim.

---

# Current synchronization — v0.1636

Experiment: `d899383090f6013e8be41da7e3e2c09888856b0b`. 507 exact core/reference files and 174 bootstrap
files. No new incoming commit followed 9de5fe9 at the initial synchronization.
This series returns our confirmed decoded-slot fix for eight integer shifts to
Inferno and imports the same committed sources without adapter divergence.

SHLB/SHRB/SHLW/SHRW/LSRW/SHLL/SHRL/LSRL validate the native count-slot span
before loading WORD. Invalid count values keep priority over value/result errors.
For valid counts both remaining spans fit before the value read. C raises bounds
for malformed slots; AMD64/ARM64 decline without stores. All operands precede the
possibly overlapping result publication. Native pointer width stays separate
from WORD32/WORD64. [Contract](upstream/libinterp/tests/README.shift-slot.md).

New registered C/native-emulation checkpoints cover 2,944 error/atomicity cases
and 51,200 image/read/write/count/FP cases. All 24 previous-version command and
backend probes reproduce the original null count-slot read. Existing integer
and shift/division fixtures retain their arithmetic/alias coverage and now
supply the production bounds-error declarations for extraction.

Availability remains AMD64/ARM64 164/173 active opcodes, with partial operand
coverage. Arithmetic spans cannot prove arbitrary mapped allocation ownership.
ARM64 instruction emulation is separate from physical ARM64/full Inferno boot.
No language-extension export or native WORD ABI change is part of this series.

---

# Current synchronization — v0.1635

Experiment: `9de5fe916f66efbafa91a7661a82b0b67f2272d3`. 503 exact core/reference
files and 174 bootstrap files. Incoming 808a4f6 and 70611aa repair C/AMD64/ARM64
string predicate and owner preflight: conditional branch targets remain unread
on false predicates; source lifetime errors precede destination-owner reads and
reference/GC publication. SELF ignores its unused source slot.

Our reverse commit ca53cc6 registers an explicit optimized-Python owner checkpoint
in Inferno. It forces both architectures and clears interactive family filters.
FlashVM now registers both full owner emulation and the optimized checkpoint in
its CMake test suite; the upstream fixture was previously absent there.
[Owner contract](upstream/libinterp/tests/README.owner-preflight.md) ·
[String branch contract](upstream/libinterp/tests/README.string-branch-preflight.md).

Native availability remains 164/173 on AMD64 and ARM64. Arithmetic pointer-span
checks do not establish mapped allocation ownership. ARM64 instruction emulation
is not physical ARM64 execution or a full Inferno boot. Compiler table/any/int
extensions were not exported; the native WORD ABI remains unchanged.

The final follow-up 85cd4e5 additionally imports decoded-slot preflight for
HEADB/HEADW/HEADL/HEADF/LENL/HEADM/MOVM/TCMP. It preserves zero-copy nonaccess,
source/count error ordering and write-free native decline. Native and emulated
list-slot checkpoints are registered in our CMake suite. Upstream explicitly
validated our incoming optimized owner checkpoint.
[List/copy/type contract](upstream/libinterp/tests/README.list-slot-preflight.md).

Our second reverse commit 9de5fe9 repairs the raw-copy regression fixture's stale
64-unit limit after ARM64 HEADM guards grew. The fixture now uses the existing
128-unit private capacity, checks every insufficient size, exact output and a
trailing canary, and invalidates the complete code window. Production capacity
and ABI are unchanged. All 411,264 differential copy cases pass under Python -O
and UBSan across AMD64/ARM64/RV64.

---

# Current synchronization — v0.1634

Experiment: `cd402a0ac5a8a2a8b18d3d60d6229e4bc2788a26`. 496 exact core/reference files and 174 bootstrap files.
Imported SLICEC/LENC C/AMD64/ARM64 preflight validates decoded slots, required
metadata and backward reference cells before access. Invalid end geometry
rejects before owner reads; zero-start prefixes keep metadata-only behavior.
Native decline remains write-free. [Contract](upstream/libinterp/tests/README.slice-length-preflight.md).

The incoming commit explicitly integrates our previous reverse commits 9f00116
and 826dae5. Their independent INDC and private-capacity tests remain selected.
No new upstream-owning defect was confirmed for reverse publication. Our typed
int/big-shift compiler extension remains outside Inferno; the native WORD ABI
is unchanged. AMD64/ARM64 availability remains 164/173 active guarded opcodes,
not complete operand coverage or physical ARM64/full Inferno boot acceptance.
Historical reports follow.

---

# Current synchronization — v0.1633

Experiment: `826dae5c387528f3120728f0bf61204b76639658`. 495 exact core/reference files and 174 bootstrap files.
Incoming ADDC, decimal and INDC/INSC repairs preflight decoded slots, String
headers and backward reference cells before reads/publication on C/AMD64/ARM64.
Native paths retain write-free decline; availability remains 164/173 active
opcodes, not complete operand coverage.

The reverse publication is 9f00116: our independent C/native INDC regressions,
checkpoint registration and expanded private-capacity sweeps. Upstream b6c160b
landed a concurrent, broader INDC/INSC implementation while our expected-head
publication was in progress. That implementation was preserved; the earlier
local functional commits were not published. The returned tests pass against
b6c160b. [Independent contract](upstream/libinterp/tests/README.indc-preflight.md) ·
[Owning repair](upstream/libinterp/tests/README.string-cell-preflight.md) ·
[Decimal](upstream/libinterp/tests/README.decimal-preflight.md) ·
[ADDC](upstream/libinterp/tests/README.append-preflight.md).

Our acyclic table CFG remains a local compiler/runtime capability; no table
language extension is exported to Inferno. AMD64 RX and ARM64 instruction
emulation remain distinct from physical ARM64/full Inferno boot acceptance.
The second reverse publication, 826dae5, fixes the adjacent decimal-span driver's stale 64-unit buffer and verifies NULL sizing plus every insufficient capacity
against production's existing 128-unit limit. Historical reports follow.

---

# Current synchronization — v0.1632

Experiment: `583c3ce6ea8629aa37a1e0237a2b8d99f4a39004`. 488 exact core/reference files and 174 bootstrap files.
AMD64/ARM64 CASEC accepts guarded lower-label identity/equal-empty matches at
the first binary-search midpoint in tables up to 4095 rows. Other comparisons
retain C fallback; availability stays 164/173 active native opcodes.

Production C string comparison/duplication now guards wrapping String headers,
heap-owner underflow and complete ASCII/Rune source spans before field reads,
allocation or copying. The new string-header/source regression is selected and
registered alongside existing native RX/emulation tests.
[Contract](upstream/libinterp/tests/README.string-case-native.md).

The complete core passes 176/176; runtime/IDE passes 117/117. AMD64 native RX
execution and ARM64 instruction emulation remain separate from physical ARM64
acceptance/full Inferno boot. No new upstream-owning defect was found for reverse
publication. Our big comparison/source decision extension stays outside Inferno.
Historical synchronization reports follow.

---

# Current synchronization — v0.1631

Experiment: `a3fb27ec812fd0c7d544451e2a003d6a4bf84034`. 486 exact core/reference files and 174 bootstrap files.
AMD64/ARM64 CASEC validates the complete declared count-cell span before reading
it and rejects narrow nonempty cell4 tables before loading rows/default targets.
Single-row dispatch now also accepts distinct empty String owners and H/empty
pairs after checked length/capacity metadata. Availability stays 164/173 active
opcodes: this extends guarded input coverage, not a complete native string search.

The bootstrap retains exact big case assembly cell padding, NaN payloads and
quoted data/source paths. Both production Str copies receive radix/digit and
decimal exponent fixes. New acceptance drivers retain the production assembly
serializer and both Str integer bodies; they are not a full assembler or a
self-hosted Limbo module run. [Assembly](upstream/limbo/tests/README.asm-roundtrip.md) ·
[Str](upstream/appl/lib/tests/README.string-radix-host.md) ·
[CASEC](upstream/libinterp/tests/README.string-case-native.md).

All 175 core tests have successful results. Initial assembly-driver failure was
a missing selected g_fmt.c dependency; the final exact snapshot includes it and
the driver passes. AMD64 RX execution and ARM64 instruction emulation are distinct
from physical ARM64 acceptance. No new upstream-owning defect was identified for
reverse publication. Our FIR scalar big CFG/verifier changes are local compiler
work; no table-language extension is exported to Inferno.

---

# Current synchronization — v0.1630

Experiment: `1ae1470f6960173a2330f212ee76d32843ae2736`. 478 exact core/reference files and 174 bootstrap files.
Imported guarded AMD64/ARM64 ICASEC paths accept empty tables or exact lower-label
owner identity in one-row tables. Availability is 164/173 active opcodes, not
full native string-table search. Other inputs preserve checked C behavior.

Full-width counts, String headers, PC bounds and long decline branches are
validated before REG.PC publication. There are no runtime callbacks, allocation,
GC/refcount writes or FP changes. [Contract and evidence](upstream/libinterp/tests/README.string-case-native.md).

The complete core suite passed 173/173. Native AMD64 RX and ARM64 instruction
emulation remain separate from physical ARM64 acceptance. No new owning defect
was identified for reverse publication. The language extension stays outside
Inferno. Historical reports follow.

---

# Current synchronization — v0.1629

Experiment: `5c79135244b0c2e651d200ec3caa4b589833dc60`. 474 exact core/reference files and 174 bootstrap files.
Imported AMD64/ARM64 paths cover raw/mapped NEWCM/NEWCMP and pointer-free mapped
CONSMP. Availability is 163/173 active commands; guarded cases may retain C.
Physical ARM64 execution remains separate from instruction-emulation evidence.

The refresh includes protected string case tables, mapped-head retain rollback,
and numeric case/goto readers matching declared IBY2WD cells. Meaningful scalar
width, cell width, VM WORD and native pointer width remain independent contracts.
The first snapshot exposed five Python fixture ABI failures; upstream b579b5f
already repaired that layout. Our returned 5c79135 expands and documents the
fixture fields for review, preserving b579b5f's concurrent functional work.

[PC cells](upstream/libinterp/tests/README.pc-table-cells.md) ·
[Mapped lists](upstream/libinterp/tests/README.mapped-cons-native.md) ·
[Channels](upstream/libinterp/tests/README.payload-channel-native.md).
The complete refreshed core suite passed 171/171. No Limbo table extension was
exported to Inferno. Historical synchronization reports follow.

---

# Current synchronization — v0.1626

Priority: x86-64 and ARM64. Experiment: `875d12263efa72bab84d27f0d4215713e7da9465`.
Imported cc51c63 adds five scalar-channel constructor paths: NEWCB/NEWCW/NEWCL/
NEWCF/NEWCP. AMD64/ARM64 availability is 160 of 173 active opcodes, not complete
operand coverage. RV64 code is unchanged in this series.

Our returned TCMP fix guards backward Type-field addresses in the C interpreter
and both priority JITs. Small nonzero owners previously underflowed the address
or read the Type field at nil; UBSan reproduces the defect against the parent.
Native decline preserves the VM image and performs no ownership/GC changes.
The generic emulation fixture also includes the new channelentry ABI field.

[Scalar channels](upstream/libinterp/tests/README.scalar-channel-native.md) ·
[TCMP contract](upstream/libinterp/tests/README.typecheck-prefix.md).
453 exact core/reference files and 174 bootstrap files are selected. No table
language extension was exported to Inferno. ARM64 instruction emulation does not
establish physical hardware execution. Historical reports follow.

---

# Current synchronization — v0.1625

Selected experiment: `b2da16226dfc5bc0e0c754ae5d95fcde6cc1afb9` (three commits after b9a569e).
The Dis snapshot contains 446 byte-identical files; the temporary bootstrap has 174.

AMD64/ARM64 now include native ICONSM/ICONSP list payload construction. Raw
copies stage destination ownership until allocation and copying succeed; pointer
payloads retain and mark current owners after the allocator returns. Copies above
256 bytes preserve the checked interpreter fallback. RV64 availability is unchanged.
Strict heap-prefix guards reject the exact boundary that would access address zero.
See [payload ownership and test contracts](upstream/libinterp/tests/README.payload-cons-native.md).

Compiler synchronization includes bounded source-history growth and overflow-safe
coordinate/case lookup, with production fixed-point selection and cast/string
folding regressions. These are imported compiler references and bootstrap fixes;
our Coco/R frontend and its native table Dis backend remain independent.

Seven acceptance drivers are now registered: payload layout, AMD64 native payload,
AMD64/ARM64 emulation, source history, fixed selection, cast folding and string folding.
WORD32/WORD64 layout models and ARM64 instruction emulation do not establish a
physical ARM64 port. No table-language extension is exported to Inferno.

Earlier synchronization reports follow as history.

---

# Current synchronization / Текущая синхронизация — v0.1593

Selected experiment: `74c193724a7352b8fda2724a5cf10029885be68c`.
Includes unique array-view SLICEA leaves and self-load ownership from 4d9ce2e,
plus our returned Loader construction/scalar result-slot fix.
[Module audit](libraries/MODULE_AUDIT.md) · [Upstream Loader contract](upstream/libinterp/tests/README.loader-results.md).
Vendored files remain byte-identical; native ARM64/RISC-V platform execution
is not established by host/emulator fixtures. Earlier reports follow as history.

---

# Dis, JIT and Sys refresh — v0.1540

[Полный русский отчёт](SYNC.ru.md) · [Manifest](UPSTREAM.json)

Source: https://github.com/bhgv/Inferno-OS-bhgv, **experiment**.
Pinned commit: `f948343b711c8300dbf76417254c18a7a8d2f5ff`, eight commits after
previous Dis snapshot `24892373ee09a2a23dc39b9dedc01fb7af61d8c1`.

## Imported and adapted

AMD64/ARM64 real arithmetic, powers/conversions/rounding, signed fixed scales
and minus-one division; emu/os Sys FD ownership/publication and unaligned
connection/result slots; process/heap diagnostics. The native Limbo bootstrap
also retains divisor evaluation and division-by-zero traps when fixed scaling
would otherwise lower to a constant zero.

The selected snapshots contain 192 Dis/reference-test files and 174 temporary
bootstrap files. Original blobs, notices and SHA-256 hashes are retained.
The new devprog bodies and two compiler source files support extracted
regressions only. No whole Inferno, GUI, Math or Geometry module was imported.
The project's Coco/R compiler remains the priority.

CMake stays compatible with 3.10; the Dis component version is now 0.1.4.
Eleven new production regressions and eight foreign-emitter build checks are
registered. TLS register/safepoint generation and the FlashVM owner bridge retain
their ABI. The Sys module header is unchanged; its implementation was refreshed.

## Executed validation, Linux AMD64

- Core build and regressions: 42/42.
- TLS/FlashVM bridge/GC tests plus foreign-emitter checks: 62/62.
- Eight focused Sys/diagnostic drivers with UBSan: 8/8.
- Source-built Limbo bootstrap product test: 1/1.
- Six IDE service modules and their .sbl files rebuilt with that bootstrap.
- Both provenance manifests verified.

Native AMD64 execution, WORD-width fixtures and ARM64/RV64 emitter construction
are distinct evidence. No ARM64/RV64 execution or Windows/macOS/Android runtime
acceptance is claimed. Sys tests inject OS/scheduler boundaries around actual
production bodies. The complete executable Dis host, Sys/VFS registration and
IDE startup remain unfinished. No newly confirmed upstream defect was found.

See the Russian report for source selection, repeat commands and integration limits.
