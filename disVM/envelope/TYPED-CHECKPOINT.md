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

## Inputs and trust

`tests/peer_call_semantics.py` accepts a corpus path. Only image names and exact
length/SHA256 identities come from its manifest; hashes identify bytes, not
provenance/authentication. `inspect_image(bytes)` has no manifest argument.
Private locals, budgets, result values, labels and `private_accept` cannot seed
any fact. Each output explicitly has `owning_execution_supported:false`.

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
unsupported. Library-only entry, ModuleData64/DATA initialization/layout, nonempty
imports, managed used frames, arbitrary CFG and other opcodes are outside this
checkpoint. Their classification does not declare the general wire invalid.
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
