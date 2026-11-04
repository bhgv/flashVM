# Next bounded file-call goldens: FlashVM review of INFERNO-0099

Status: review/acceptance plan, no new loader admission or shared IDs. The agreed
bounded scalar local-call design remains a design; current readers still refuse
file FRAME/CALL/RET/LEA outside their admitted subset.

FlashVM agrees to begin with canonical module entry plus a single int32 helper
returning42. The second golden should distinguish two sequential root-to-leaf
calls from a nested helper-to-helper chain. Inferno's current `dis64-bound-call.h`
explicitly permits only linear root-to-leaf composition: helpers may not call.
Two root calls can exercise invalidation/rebinding of the reserved outgoing
result cell without broadening that proof. A nested acyclic chain needs its own
validator/runtime-lifetime proof before it can be an accepted executable golden.

FlashVM can consume identical immutable container bytes for structural inspection
and assert current executable refusal. That is not independent executable-call
validation: the envelope tool currently shares the pinned header-only codec and
does not create a running module/entry/EXIT scheduler. Existing pilot C/RX handler
checks are separate evidence, not a production Dis64 VM. Do not count a shared
codec round-trip or trusted construction as independent whole-file acceptance.

Remaining concrete execution/validation obligations include:

- An untrusted-file derivation of canonical function ownership, exact call target,
  frame/signature binding and local scalar kinds. The private bound-call model's
  trusted compiler local-kind declarations cannot simply become file authority.
- Definite assignment and exact result kind/width, including caller result-tail
  invalidation at new FRAME, successful-return-only publication and every path to
  callee RET. Int32/Bool width equality is not kind equality.
- Explicit checked LEA execution/JIT-cache mode: full8-byte address only to the
  prepared callee's byte32 result slot, exact caller-owned outgoing result tail;
  ordinary WORD32 LEA semantics remain unchanged. Numeric loads/stores cannot
  acquire access to reserved pointer cells40/48/56 or result-address slots.
- Actual whole-file allocation, entry/CALL/RET/EXIT lowering, root/frame/Module
  lifetime and error retirement. Decoding an EXIT or calling a numeric native
  handler alone is not this lifecycle.

For each first golden, exchange canonical hex bytes and SHA256, exact function/
frame/signature catalog and expected result42, plus a manifest of single-purpose
mutations and expected pre-execution rejection stage. Include CALL into an
interior, mismatched catalog/frame/signature, uninitialized argument/result,
reserved-cell numeric access, result width/kind mismatch and LEA destination/
mode confusion. File/payload/section lengths and checksums must remain otherwise
valid so a rejected mutation tests the intended validator rather than framing.

Keep acceptance closed until independent raw-byte validators and actual C/RX
module lifecycle results agree. Native ARM64/x64 and RV64/QEMU owning acceptance
remain separate from FlashVM's local x64 runs. No new opcode/section/feature ID,
native ABI or cross-VM pointer representation is requested by this review.


## INFERNO-0100 immutable corpus receipt

Peer2b4a93a85b176ed0b32df1c94e93ac661397fcfa adopts two SEQUENTIAL root
calls and publishes25 immutable hex images with a length/SHA256 manifest. Four
private accepts include answer42, sequential chain42, reordered sections and
an accepted static image whose callee divides by zero;21 are private refusals.
No new checksum extension or executable production-file permission is added.

Our independent Python outer-directory inspection and actual pinned inspector
agree on23 canonical outer directories and2 malformed envelopes. All25 exact
identities match; all25 current scalar-gate refusals preserve existing rewrite
output. The private21 semantic refusal reasons are NOT independently validated
by our still-closed scalar gate. Trusted manifest numeric locals are never used
by this inspection as executable authority. Reproduce with explicit tools/corpus:

```sh
python3 disVM/envelope/tests/peer_call_goldens.py PATH_TO_DIS64INSPECT \
    PATH_TO_INFERNO/utils/tests/goldens/dis64-calls
```

Separately, the peer's complete-image private harness was replayed on local x64:
4 accepts/21 refusals,132 actual C/RX/extent/quanta/saved/error runs,5 strict
source controls, default O3 nonrecovering UBSan. This remains owning-source
private runtime evidence under bounded transports; it is not an independent
FlashVM call validator/loader implementation or new physical-target acceptance.
The remaining untrusted local-kind, typed-result/LEA and whole-file lifecycle
obligations above remain before changing public admission.


## INFERNO-0101 raw-byte kind inference receipt

Peer51aebfc3eabded32f8013b7b715d55f818c052dd adds a conservative private
inference API taking raw image bytes without external numeric-local declarations.
Signature parameter seeds apply even when unused. Fixed Int32/Big/Real spans
reject overlaps/reinterpretation; Bool refuses conservatively rather than being
inferred from W width. Inferred read kind is not definite initialization: the
existing full call/return/setup verifier still must succeed before publication.
That separation and the absence of a new trusted wire sidecar are appropriate
for this bounded next prerequisite; they do not by themselves open the loader.

Our explicit `--infer` inspection checks the14 new manifest identities, all14
canonical outer directories and all14 present scalar-gate refusals with output
preservation. The manifest is checked not to contain local declarations. Original
25 identities/directories/refusals still agree. This is outer/identity inspection,
NOT an independent implementation of the new inferred kind/typed proof.

Separately replayed dis64-call-infer.py on local x64, default O3 nonrecovering
UBSan:9 accepts/30 atomic refusals/312 actual C/RX/extent/quanta/saved/error
runs and4 external/seed/publication/kind semantic controls pass. External locals
are poisoned by the peer harness. That is owning-source reproduction under its
bounded transports, not independent FlashVM executable validation, production
namespace/file loading or physical ARM64/RV64 acceptance here. Public calls and
ordinary WORD32 LEA stay unchanged. The `--infer` inspection command takes the
same inspector argument and `utils/tests/goldens/dis64-infer` corpus directory.

## INFERNO-0111 independent partial-schema checkpoint

Rechecked the exact immutable corpus at Inferno
559b08ca24c66fe2678e2207e062b093d841ca3c, requested by receipt f6c1daf.
All25 SHA256/length identities match. Independent outer inspection finds23
canonical directories and2 malformed containers: reject-truncated and reject-trailer.
The actual current dis64inspect refuses all25 scalar-pilot rewrites and preserves
previous output. Its canonical diagnostic remains `unsupported or invalid scalar
pilot payloads`; this combined diagnostic alone does not validate a semantic cause.

The new independent Python partial parser consumes metadata field/profile shapes,
complete structural type/map8 tables and lossless Rune32 string pools directly from
bytes. All23 well-framed images pass these three section schemas. The parser does
not call the shared C codecs or consume manifest locals/private_accept as authority.
Legal managed maps,65 structural descriptors and duplicate all-bits Rune32 values
remain accepted independently of the private np0/type-budget policy. Four legal
boundaries and seven malformed section controls are checked without a peer checkout.

Code/exports schemas, cross-section binding, typed local/call/result/LEA validation
and actual module lifecycle are still unvalidated by this independent parser.
Thus these results do not label all23 complete modules valid or independently
confirm the private21 semantic refusals. Current unsupported executable admission
and damaged wire are reported separately, without importing private fixture policy
as production permission. No IDs/layout/default or named-launch agreement changes.

[Per-image evidence and exact manifest/checker identities](reports/inferno0111-schema.json).
The developer tool needs an explicit immutable corpus directory; it is not a loader:

```sh
python3 disVM/envelope/tests/peer_call_schema.py --self-test
python3 disVM/envelope/tests/peer_call_schema.py PATH_TO_PINNED_DIS64_CALLS
```

The old outer/closed-gate command above remains a separate actual inspector check.

## INFERNO-0112/0113 independent code and binding checkpoint

The new `peer_call_binding.py` layers independent CODE/EXPORTS/IMPORTS parsing
over the pinned partial readers, leaving those existing files byte-identical.
On the same immutable 559b08ca corpus:25 identities,23 bounded structural ID/range
passes, two malformed outer containers. No private semantic labels become policy.

Checks cover exact instruction framing/counts, signed7/14/30 operands including
legal nonminimal forms, export/import records and scalar-signature-v1 shapes,
Rune32 name IDs/content duplicates, entry/frame IDs/ranges and frame size/alignment.
Only known immediate CALL/JMP PC and FRAME type references receive static ID/range
checks. Supported addressing is direct forms plus the existing bounded indirect-FP
destination; other forms stop as unsupported schema, never as legacy fallback.
Module data is bounded by its type size; exact allocation layout is unvalidated.
Metadata-entry/export-frame agreement, opcode/local-slot semantics, function
ownership, typed calls/results/LEA and actual owning execution remain unvalidated.
Thus structural success does not confirm the peer's private21 semantic outcomes.

Self-tests cover seven compact boundary/nonminimal vectors,16 malformed binding
mutations, unsupported modes, wrong corpus identity and inert manifest policy/locals
labels. The actual C inspector's closed gate remains a separate check; no wire,
IDs, default admission or immutable named-launch/cache agreement changes.

```sh
python3 disVM/envelope/tests/peer_call_binding.py --self-test
python3 disVM/envelope/tests/peer_call_binding.py PATH_TO_PINNED_DIS64_CALLS
```

[Per-image report, exact peer/manifest/checker identities](reports/inferno0112-binding.json).
INFERNO-0113 native Draw attach ownership fixes are acknowledged as owning-source
work; no Draw/Tk/Prefab implementation or concurrency change is imported here.

## INFERNO-0120 independent typed checkpoint

The new [bounded semantic pass](TYPED-CHECKPOINT.md) derives function/frame roles,
scalar kinds, definite initialization and result/checked-LEA facts from wire
sections, without private sidecar locals. The exact25 images yield7 bounded
passes,12 bounded semantic refusals,4 unsupported cases and2 malformed outers.
Three private-refusal labels pass the bounded function checks because unused
managed descriptors,65 types and an empty IMPORTS section are not universal
semantic errors. Unused descriptor semantics remain unproved. Bool and nonempty
module data are explicitly unsupported. Every case retains owning execution
closed. This is the requested independent checkpoint, not whole-file admission
or a replacement for root/frame/module/code-lifetime and C/RX execution proofs.
