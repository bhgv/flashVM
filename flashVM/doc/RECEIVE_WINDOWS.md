# Unique-part receive windows

[Русский](RECEIVE_WINDOWS.ru.md), [adaptation](CHANNEL_ADAPTATION.md),
[media parts](MEDIA_SLOTS.md). `smeasure.h` turns explicit verified host arrivals
into the existing compact tag-zero feedback schema. It is standalone C89, with
no sockets, allocation, timer, VM roots or process-global state.

## Timing, identity and categories

Initialize a separate [FVM_SMeasure](../include/flashvm/smeasure.h#L20) for one trusted stream, epoch and window ID,
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
| [`fvm_smeasure_init`](../include/flashvm/smeasure.h#L30) | Validate identity/count/deadline and replace complete caller state atomically; INVALID preserves old state. No storage is borrowed. Reinitialization explicitly abandons an earlier window. |
| [`fvm_smeasure_record`](../include/flashvm/smeasure.h#L38) | Consume one trusted part, valid flag 0/1 and monotonic relative arrival time. INVALID for out-of-range metadata; CONFLICT for closed/regressing time; errors preserve state. Serial owner, bounded constant work. |
| [`fvm_smeasure_finish`](../include/flashvm/smeasure.h#L44) | Freeze one owned feedback copy. INVALID for bad RTT/arguments; CONFLICT for early/regressing/already-closed calls. Errors preserve state/output. Feed the copy to [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L117) or [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L106); neither path implicitly changes framing. |

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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:117](../include/flashvm/sadapt.h#L117) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:106](../include/flashvm/sadapt.h#L106) |
| `FVM_SAdaptFeedback` | struct | [include/flashvm/sadapt.h:39](../include/flashvm/sadapt.h#L39) |
| `FVM_SMeasure` | struct | [include/flashvm/smeasure.h:20](../include/flashvm/smeasure.h#L20) |
| `fvm_smeasure_finish` | function | [include/flashvm/smeasure.h:44](../include/flashvm/smeasure.h#L44) |
| `fvm_smeasure_init` | function | [include/flashvm/smeasure.h:30](../include/flashvm/smeasure.h#L30) |
| `fvm_smeasure_record` | function | [include/flashvm/smeasure.h:38](../include/flashvm/smeasure.h#L38) |
<!-- source-links:end -->
