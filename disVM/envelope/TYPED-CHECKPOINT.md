# Independent wire-derived scalar call checkpoint

Envelope tools0.4 / workspace v0.1805. This is an offline independent validator
for a deliberately bounded subset, not a loader or execution permission. The
native DisVM stays runtime0.106 with NativeWorld unchanged. Shared Dis64 IDs,
Frame64 layout, default43/173, WORD32 LEA and immutable named `dis64-calls` launch/
cache agreement are unchanged.

## Inferno coordination

Inferno experiment records the shared address rule in its
[Dis64 contract](https://github.com/bhgv/Inferno-OS-bhgv/blob/8a3e31b936e647b702cb3de1ef49f23b723e90f5/doc/dis64-contract.md)
and the paired runtime evidence in its
[FlashVM journal](https://github.com/bhgv/Inferno-OS-bhgv/blob/8a3e31b936e647b702cb3de1ef49f23b723e90f5/doc/dis64-flashvm-journal.md).
The future checked form derives a full eight-byte address from the validated
caller result span and writes it only to prepared-callee byte 32, independent
of numeric WORD width. The address is transient runtime state. It has no
serialized pointer representation or managed pointer-map bit. The independent
checks below apply this rule within their bounded scalar subset. The journal's
runtime replay has its own execution scope.

The [Inferno experiment checkpoint](https://github.com/bhgv/Inferno-OS-bhgv/commit/0fe09afa6b8ca6ac6e93c52581547bf42ff5b89d)
resolves the canonical `$Sys.millisec` import in a trusted Sys fixture.
A local Linux x64 replay with source-built Limbo and strict UBSan passed
the 13-entry registration, 31 allocation refusals, serialized import
checks, production callback and startup refusal controls. The fixture
supplies the result cell. File execution still needs VM-owned result
binding and the corresponding LOAD/MFRAME/MCALL admission.

The [Inferno loader ownership checkpoint](https://github.com/bhgv/Inferno-OS-bhgv/commit/f3da3d19d3bf6580fdd5d43fedeccfbf13e1f6c4)
fixes reference retirement on refused builtin LOAD. Local x64 UBSan
replay of `readmod-io.py` passed 3,307 and 3,305 checks across the two
startup profiles. The checkpoint adds no wire fields.

The [Inferno three-target checkpoint](https://github.com/bhgv/Inferno-OS-bhgv/commit/fd5221c5bb83987eff78348169509d27449dbbdf)
configures paired console and Sys clock gates on ARM64, x64 and RV64.
The RV64 job uses QEMU. A local x64 UBSan replay of that revision passed
the clock call in four C/JIT stack modes, both 48-epoch provider runs
and eight refusal/cleanup cases. The cross-target workflow introduces
no wire changes.

### Reply to Inferno on imported nonvoid results

Inferno [INFERNO-0148 and INFERNO-0150](https://github.com/bhgv/Inferno-OS-bhgv/blob/9f4edc3981ac1407b2eaab98c88f9e99b1a453bc/doc/dis64-inferno-journal.md)
ask how the producer represents the result destination. FlashVM has a
checked local-call binding and offline imported metadata checks. The
imported nonvoid producer and admission path remain open.

A candidate sequence for import 0, `$Sys.millisec: fn(): int`, is:

```text
checked LOAD / resolve $Sys provider
MFRAME import[0], prepared frame
checked result LEA caller.out:Int32 -> prepared result-address cell
MCALL import[0], same prepared frame
read caller.out as Int32 after normal return
```

Checked LEA owns destination binding. It derives a runtime Ref64 from the
validated caller-owned result span and publishes it to the prepared callee
result-address cell (Frame64 byte 32 in the current native representation).
MFRAME and MCALL establish and consume the matching frame and import.
The address is transient and has no managed map bit. Exact imported
operand encoding, feature negotiation and file admission remain open.
Inferno's native binding fixture tests its own frame provenance independently.
A Linux x64 strict-UBSan replay of Inferno `9f4edc3` passed four C/JIT
stack modes for the clock call, return and error-unwind runs over 48 GC
epochs each, and eight consumer refusal and cleanup cases.

[INFERNO-0153](https://github.com/bhgv/Inferno-OS-bhgv/commit/a0fec548a4e6dc32b1cbb584c9339a2617939a47)
serializes the same MFRAME → indirect LEA → MCALL binding pattern for
the private clock pilot. Its fixture stores the prepared frame at caller
byte 40 and takes the address of caller byte 72 in an 80-byte frame,
placing it at callee byte 32. This matches the FlashVM local convention
for the reserved final eight-byte result slot and the result address
in the prepared frame. For the FlashVM imported producer, the current
candidate is to reuse that indirect LEA addressing form and check both
the caller result span and the prepared frame before publishing the Ref64.
The Inferno console pilot supplies its provider out of band. FlashVM's
candidate includes checked provider resolution, so the two complete
instruction streams are distinct. On Linux x64, the strict-UBSan
replay of this revision ran the serialized clock image in four C/JIT
stack modes and passed both 48-epoch provider runs and eight refusal cases.


## Inputs and trust

`tests/peer_call_semantics.py` accepts a corpus path. Only image names and exact
length/SHA256 identities come from its manifest; hashes identify bytes, not
provenance/authentication. `inspect_image(bytes)` has no manifest argument.
Private locals, budgets, result values, labels and `private_accept` cannot seed
any fact. Each output explicitly has `owning_execution_supported:false`.

Outer framing follows the shared directory rules. Optional DEBUG,
PROVENANCE and unknown optional sections do not supply semantic facts.
A framed CONTROL section is reported as unsupported by this checkpoint.
CODE framing rejects opcode bytes outside the shared Inferno ISA range.
An in-range opcode still needs a supported semantic rule. The structural
binding pass checks immediate import ordinals for the direct MFRAME and MCALL
forms used by the Inferno paired console at
[5ac85ea](https://github.com/bhgv/Inferno-OS-bhgv/commit/5ac85ea708e3ff525c786c0ab66e56a43fb4bbef).
The optional offline command
`python3 tests/peer_call_semantics.py --pair consumer.dis64 provider.dis64`
checks one explicit provider against all consumer imports. It compares exact
Rune32 module and symbol contents and scalar signatures. A successful report
lists export ordinals, entry PCs and frame IDs in consumer import order.
A failed match publishes no partial coordinates. Provider discovery,
frame-to-call pairing and runtime admission stay outside this command.
A canonical mandatory extension manifest is reported as unsupported before
any payload analysis. The report lists each required name, exact major and
minimum minor. Malformed manifests or section geometry are rejected at the
outer stage, without publishing extension records.

FlashVM features that preserve existing bytecode behavior need no wire
extension. An optional section can carry ignorable metadata. A feature whose
absence changes execution needs an explicit mandatory extension contract.
The shared Dis64 container already provides the manifest for that contract.
An older reader can identify the requirement and refuse the image before
linking or interpreting code. An extension name in a manifest grants no
runtime capability. A future reader must implement the named semantics and
check version compatibility explicitly.

Parsing reuses FlashVM's independent outer/schema/binding readers, not Inferno's
private C validator/writer. The pass consumes existing sections/code/signatures,
checks structural bindings, derives exported function intervals and module entry,
then infers typed scalar spans and initialization. No peer implementation is
imported and no trusted local-kind sidecar is invented.

`peer_call_roles.py` owns catalog/role/slot constraints. `peer_call_flow.py` owns
straight-line transfer rules. The driver owns limits, identities and complete
report publication. A later refusal publishes no partial function facts.

## Checked bounded contract

* Module entry must be an exported start with exactly matching frame and void/
  no-argument signature. Export aliases at a shared PC must agree. Intervals are
  partitioned by exported starts; no unowned prefix or function-interior CALL.
* Used frames have the existing64-byte header and no managed payload map in this
  scalar checkpoint. Frame geometry is distinct from inferred scalar kinds.
  Unused descriptors are structurally parsed but gain no semantic role proof.
* Signature parameters seed exact canonical aligned fields from byte64, even
  when unused. Int32 is4 bytes; Big/Real are8. Their kinds never collapse to
  host WORD width. Inferred overlapping/reinterpreted spans refuse. A kind fact
  never initializes a value; every local read and implicit RMW LHS needs a prior
  write or incoming signature seed.
* FRAME uses immediate type ID and reserved temporary40. Argument writes may
  target only the callee's canonical parameter prefix; CALL consumes that exact
  prepared frame, matches the target's frame/signature and requires every typed
  argument initialized. Helpers cannot call; this pass supports sequential
  root-to-leaf composition, not nested/recursive activation ownership.
* A new FRAME invalidates the caller's old outgoing result. Checked result LEA
  writes the full Ref64 address only to prepared-frame byte32 and addresses
  exactly the caller's reserved last8-byte outgoing tail. Ordinary numeric
  access cannot become access to header32/40/48/56 or manufacture that address.
  Ordinary WORD32 LEA is outside this checked form, not redefined by it.
* A nonvoid callee RET needs an exact-width/kind indirect result store on its
  normal straight-line path. CALL's abstract result becomes readable only after
  a verified normal return; no arithmetic safety/termination is claimed. The
  divide-zero image passes static rules but remains closed for execution.
* EXIT/RET must match root/helper role and terminate the owned interval. Control
  flow, other addressing/opcodes and explicit middle operands outside the
  implemented transfer rules are unsupported, not silently accepted.

Bool is not inferred merely because it has Int32 width; its producer proof is
unsupported. Structural binding checks the exact ModuleData64 descriptor extent
and scalar DATA records, including alignment, width, overlap and mapped-reference
exclusion. No module storage is materialized. Library-only entry, ModuleData64
execution, nonempty imports, managed used frames, arbitrary CFG and other opcodes
are outside this checkpoint. Their classification does not declare the general wire invalid.
Local inferred call facts do not prove allocation capacity/provenance, root
publication, GC barriers, activation/module/code lifetime, exception retirement
or actual C/RX module execution. Those remain separate gates before admission.

## Verdicts on the exact25 images

[Archived per-image report](reports/inferno0120-semantics.json) includes image
identities, checker-source hashes, stages/reasons, inferred slots and call facts.
Manifest SHA256 is `fd22e66dbdaf55fbbb8c1751a5e8d34d7b7c18dbababc44260a505ff9b6265b0`.
The report checkpoint is Inferno experiment82d5275be9404068d716232b9f8102ea1511fc17.

| Status | Count | Meaning |
|---|---:|---|
| bounded-checks-passed | 7 | Implemented scalar role/flow rules pass; owning execution remains closed |
| bounded-refusal | 12 | Function/frame/argument/local/init/result/LEA rule violated in this bounded contract |
| unsupported | 4 | Two Bool signatures, library-only profile, module-data layout need additional proofs |
| malformed outer | 2 | Truncated section geometry or trailing bytes |

The first four bounded passes are answer42, chain42, reordered42 and divide-zero.
Three images named as private refusals also pass these bounded checks:
`reject-managed-type` has an unused managed descriptor, `reject-type-budget` has65
types and `reject-imports` has an empty IMPORTS section. They are not universal
wire/semantic errors. Unused descriptor roles, general managed support and owning
execution remain unproved; private policy is not imported to manufacture matching
verdicts. Both decoders still agree on23 canonical and2 malformed outers, and the
actual scalar execution gate still refuses all25 while preserving prior output.

## Evidence and reproduction

```sh
python3 disVM/envelope/tests/test_peer_call_semantics.py
python3 disVM/envelope/tests/test_peer_call_semantics.py --corpus PATH_TO_GOLDENS
python3 disVM/envelope/tests/peer_call_semantics.py PATH_TO_GOLDENS
```

39 independently built raw-wire cases exercise role/width/alias/init/result/LEA
mutations, Int32 stride4, Big width, Real parameter seeding, packed/mixed incoming
arguments, section reorder, unused descriptors/65 types and unsupported branches.
Four omitted validator obligations must make the test oracle fail at exact named
stages (initialization, frame agreement, LEA role, return initialization). Child
exit10 plus the exact oracle marker are required; signals/errors never count as
control detection. Corpus metamorphism renames all files and poisons locals,
private policies/results/labels; per-image semantic verdict/facts remain identical.
Identity corruption still refuses. No Inferno checkout is needed for synthetic
CTest controls.

Offline resource ceilings are tool policy, not shared-format/private budgets:
4MiB images/manifests,1024 corpus images/functions/import records,8192 code rows,
4096 types/strings,256 parameters and1024 inferred local slots per function.
Hex text has a separate16MiB read ceiling. Above-limit inputs remain unsupported
or fail identity-input preflight; no partial semantic summary/admission is published.

The installed runtime/editor SDK contains the complete Python closure at
`source/dis64-checkpoint/`, this guide and the archived report under `docs/dis64/`.
Run `python3 source/dis64-checkpoint/test_peer_call_semantics.py` from SDK root.
Providing a peer corpus remains an explicit caller action; no signed-in session,
network lookup or hidden peer checkout is required by the installed checker.
