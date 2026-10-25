# Unique-part receive windows

[Русский](RECEIVE_WINDOWS.ru.md), [adaptation](CHANNEL_ADAPTATION.md),
[media parts](MEDIA_SLOTS.md). `smeasure.h` turns explicit verified host arrivals
into the existing compact tag-zero feedback schema. It is standalone C89, with
no sockets, allocation, timer, VM roots or process-global state.

## Timing, identity and categories

Initialize a separate [FVM_SMeasure](../include/flashvm/smeasure.h#L22) for one trusted stream, epoch and window ID,
with 1..4096 expected parts and a positive relative deadline in milliseconds.
The host assigns zero-based part IDs; all arrival times use the same monotonic
origin as the deadline, never a remote presentation timestamp or wall clock.
Times and identities are Uint32; choose a new window before elapsed time wraps.
No packet may change expected count or bind a new epoch implicitly.

The fixed 1024-byte packed category table counts distinct parts, not datagrams.
Verified usable arrivals at or before the deadline are on_time; later ones are
late. A damaged arrival may be recorded only if an independently trusted envelope
identifies its part. Never read an index from a failed checksum/header: leave the
part missing if no trusted identity exists. UDP/IP may discard damage entirely;
the host sees those packets only as missing at close.

Repeated arrivals do not inflate counts. A timely successful repair replaces
damaged with on_time, and a late successful repair replaces damaged with late.
On_time cannot be downgraded by later duplicates, and late cannot be downgraded
by corrupt duplicates. Counts remain exclusive; inferred missing is expected
minus on_time, late and damaged. Previously accepted data must not be credited
twice under another identity. Authentication and byte validation remain host duties.

Close only at/after the deadline and the last recorded arrival. The copied
[FVM_SAdaptFeedback](../include/flashvm/sadapt.h#L39) freezes history: subsequent packets and a second finish return
CONFLICT. FILE repairs after close may complete an object but cannot improve this
window's deadline statistics. RTT is an explicit independently measured known/value
pair; the module sends no probes. Unknown RTT prevents growth in the adaptation
policy. Missing, damaged and late all contribute to its unusable percentage.

| API | Ownership, errors and execution |
| --- | --- |
| [`fvm_smeasure_init`](../include/flashvm/smeasure.h#L38) | Validate identity/count/deadline and replace complete caller state atomically; INVALID preserves old state. No storage is borrowed. Reinitialization explicitly abandons an earlier window. |
| [`fvm_smeasure_record`](../include/flashvm/smeasure.h#L49) | Consume one trusted part, valid flag 0/1 and monotonic relative arrival time. INVALID for out-of-range metadata; CONFLICT for closed/regressing time; errors preserve state. Serial owner, bounded constant work. |
| [`fvm_smeasure_finish`](../include/flashvm/smeasure.h#L57) | Freeze one owned feedback copy. INVALID for bad RTT/arguments; CONFLICT for early/regressing/already-closed calls. Errors preserve state/output. Feed the copy to [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L135) or [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L118); neither path implicitly changes framing. |

State, packet data and outputs must be disjoint; serialize all calls for each
window. The small state is freely copyable for snapshots; no self-references or
borrowed VM objects exist. Do not confuse it with the stable-address adaptation
policy. Maximum sample count bounds storage; larger transfers use separate windows
and explicitly chosen aggregation, not truncated counters.

## Build and evidence

```sh
cmake -S flashVM -B build-receive -DFVM_BUILD_TESTS=ON
cmake --build build-receive --target flashvm_srecord_measure_contract flashvm_srecord_oracle
ctest --test-dir build-receive -R flashvm_srecord_measure --output-on-failure
```

Native contract covers exact deadline, duplicate/repair upgrades, backwards time,
close/failure atomicity, last bitmap index and maximum Uint32 timestamp. An
independent Python category model checks 48000 arrivals across 80 windows and
canonical feedback bytes using arbitrary-precision integers/zlib. These are
bounded observation contracts, not a production listener, congestion controller,
authenticated transport, codec or full platform/SWF acceptance.

## Verified media receiver adapter (v0.748)

[FVM_SMeasureMedia](../include/flashvm/smeasure.h#L65) binds one pristine initialized [FVM_SMediaReceiver](../include/flashvm/smedia.h#L53). It copies
agreed mode, stream, epoch, object, presentation time, total, profile and tag,
and borrows the native receiver and its existing object storage through finish.
Further acceptance after changing object/profile returns CONFLICT. Do not reinitialize
or separately mutate the borrowed receiver before closing; keep all objects and
input/output buffers disjoint and serialize their calls on the same owner.

[`fvm_smeasure_media_accept`](../include/flashvm/smeasure.h#L90) first validates the host arrival time, then verifies
the slot and accepts actual data through [`fvm_smedia_accept`](../include/flashvm/smedia.h#L96). Only successful
acceptance credits the verified part. Identical duplicates remain one sample;
different valid duplicate bytes are CONFLICT. Corrupt or unrelated slots preserve
adapter, receiver, image and complete output. They remain inferred missing because
a bad header does not establish a part identity. The host may explicitly use
[`fvm_smeasure_record`](../include/flashvm/smeasure.h#L49) on the embedded measurement only when a separate trusted
envelope identifies damage; batch position alone is not a media part ID.

Close at the actual host deadline. Afterwards use the ordinary receiver for FILE
repairs; the saved report remains historical. Closing itself touches only copied
window state and performs no I/O. No authentication, pacing, congestion policy,
MTU discovery or network listener is installed by these functions.

| API | Ownership and result |
| --- | --- |
| [`fvm_smeasure_media_init`](../include/flashvm/smeasure.h#L81) | Bind an initialized pristine receiver and derive exact expected count from its agreed total/profile. No new image allocation. INVALID for bad descriptors, CONFLICT if parts were already accepted; preserve output on error. |
| [`fvm_smeasure_media_accept`](../include/flashvm/smeasure.h#L90) | Borrow one slot only during the call; verify identity/checksum/duplicate data before native copy and count. Reject closed/regressing/mutated-object/profile with CONFLICT; codec errors propagate atomically. complete changes only on successful receiver acceptance. |
| [`fvm_smeasure_media_finish`](../include/flashvm/smeasure.h#L96) | Freeze an owned feedback copy with host elapsed time and explicit RTT. Same error/atomicity contract as ordinary finish; after success the receiver borrow ends, but no storage is freed. |

The [C89 media_parts tutorial](../examples/embedding/media_parts/README.md)
now captures a FILE report before repair and a separate LIVE report with unknown
RTT. The [real player frame test](../frontend/tests/frame_parts_example.py)
receives actual exported parts on IPv4 UDP loopback, uses a monotonic host clock,
measures a TCP request/echo RTT and closes the observation at its real deadline.
The first window deliberately loses two of six parts (one unidentifiable corrupt
slot, one omission); later windows deliver all eleven and six parts. Native
feedback bytes match independent Python integer/zlib encoding. Feeding those
measured reports into the player export reproduces the normal→small→normal→large
profile sequence, in FILE and LIVE modes, with every RGBA byte checked.

Loopback is bounded transport evidence, not Internet/device acceptance. Its
100 ms test observation deadline and loopback datagram sizes are test choices,
not recommended production timing or path-MTU values. RTT and scheduling may vary
on other hosts; production hosts choose their own clock/deadline policies.

Validation at v0.748: 59 selected native checks, 14 shared-host checks and the standalone C++98/measurement suite pass. New measurement contracts pass ASan/UBSan; affected protocol/cache paths also pass real interpreter/JIT frame integration. Leak detection is disabled by this restricted host; the whole VM is not instrumented. Strict C89 measurement contracts link and C++98 consumers compile for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW. Foreign binaries are not executed. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:135](../include/flashvm/sadapt.h#L135) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:118](../include/flashvm/sadapt.h#L118) |
| `FVM_SAdaptFeedback` | struct | [include/flashvm/sadapt.h:39](../include/flashvm/sadapt.h#L39) |
| `FVM_SMeasure` | struct | [include/flashvm/smeasure.h:22](../include/flashvm/smeasure.h#L22) |
| `fvm_smeasure_finish` | function | [include/flashvm/smeasure.h:57](../include/flashvm/smeasure.h#L57) |
| `fvm_smeasure_init` | function | [include/flashvm/smeasure.h:38](../include/flashvm/smeasure.h#L38) |
| `fvm_smeasure_media_accept` | function | [include/flashvm/smeasure.h:90](../include/flashvm/smeasure.h#L90) |
| `fvm_smeasure_media_finish` | function | [include/flashvm/smeasure.h:96](../include/flashvm/smeasure.h#L96) |
| `fvm_smeasure_media_init` | function | [include/flashvm/smeasure.h:81](../include/flashvm/smeasure.h#L81) |
| `fvm_smeasure_record` | function | [include/flashvm/smeasure.h:49](../include/flashvm/smeasure.h#L49) |
| `FVM_SMeasureMedia` | struct | [include/flashvm/smeasure.h:65](../include/flashvm/smeasure.h#L65) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:96](../include/flashvm/smedia.h#L96) |
| `FVM_SMediaReceiver` | struct | [include/flashvm/smedia.h:53](../include/flashvm/smedia.h#L53) |
<!-- source-links:end -->
