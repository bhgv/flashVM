# Feedback-driven bulk adaptation

[Русский](CHANNEL_ADAPTATION.ru.md). `sadapt.h` provides an allocation-free C89
policy between host link measurements and [tag-zero size negotiation](MEDIA_SLOTS.md).
It chooses a supported profile; it neither measures sockets nor changes live
framing itself. Ordinary compact control messages remain unchanged.

## Measurements and supported profiles

The host supplies an increasing ladder of one through eight fixed bulk profiles:
same already-bound tag, strictly increasing slot_bytes, part_bytes and record_limit.
Every entry must satisfy the existing scontrol capacity/reserve rules. The ladder
must fit both peers and the actual output storage/path policy. The module copies
it; no input-table borrow survives init. One initial index describes the currently
committed profile. Separate stream directions/epochs need separate policies.

A report identifies stream, epoch and a strictly increasing nonzero Uint32 window.
It counts expected parts, distinct on_time usable parts, distinct late parts and
distinct damaged parts. Categories must not overlap or exceed expected; missing
is inferred. Duplicates are not samples. All parts not usable before the host's
deadline count as unusable, including late, damaged and absent. The host knows
expected counts from trusted object metadata/deadlines; an unknown gap in feedback
IDs does not imply packet loss. UDP/IP may discard corrupt datagrams before the
application sees them; such losses belong in the missing count.

RTT is explicit milliseconds with a known flag. Unknown RTT requires zero value,
prevents growth and does not prevent loss-driven shrinking. Reports must arrive
through authenticated host policy; CRC cannot authenticate feedback. Wrong epoch,
old/duplicate window or an observation while negotiation is pending returns
CONFLICT without changing state/output. Malformed counts return INVALID.

## Decisions, hysteresis and negotiation

A bad window reaches configured unusable percentage or high known RTT. Repeated
bad windows propose one smaller ladder entry. A good window stays below the
configured low unusable percentage and low known RTT; a longer stable run may
propose one larger entry. Intermediate/unknown-quality windows reset streaks.
At ladder bounds the policy keeps that bound. Percent comparisons are exact at
integer boundaries using division/remainders, not overflowing Uint32*100.

Configuration initializes illustrative defaults: bad >=10%, good <=1%, high RTT
>=250 ms, low RTT <=50 ms, two bad/four good windows, two-window cooldown and at
least 64 expected parts per window. These are configurable example defaults,
not universal network recommendations. Insufficient windows break streaks but
do not spend cooldown. During cooldown full windows are consumed without tuning.

A proposed candidate remains pending and does not change current. The host sends
OFFER_AT and commits a correlated ACK for a future object, then calls adaptation
commit with the exact selected ladder profile. A changed/clamped selection is
CONFLICT: do not silently label it as the candidate. Reject/rebuild a supported
ladder after reconciling actual peer limits. Rejected negotiations preserve current
and start cooldown. No feedback queue, implicit retry, clock or packet sender exists.
A lost control connection requires session reconciliation before resuming data.

Changing size alone is not congestion control and cannot guarantee improved
loss/throughput. Retain pacing, congestion policy and path-MTU limits separately.
Do not increase Internet UDP datagrams merely because a loopback test succeeds.
Primary background: [RFC 8085](https://www.rfc-editor.org/info/rfc8085/).

## Tag-zero feedback service

Service selector 1 remains legacy bindings; selector 2 remains limits. Selector
3 is feedback, command 1. Its compact payload contains eleven canonical unsigned
S integers, in order:

`3, 1, stream, epoch, window, expected, on_time, late, damaged, rtt_known, rtt_ms`

It is an ordinary variable-length checked S-record with tag 0, never padded.
Read consumes one complete record, leaving trailing records for the host. Unknown
services/commands return UNKNOWN for explicit routing; malformed known reports
return INVALID. Neither decoding nor applying the policy grants native authority.

| API | Ownership / result |
| --- | --- |
| [`fvm_sadapt_config_init`](../include/flashvm/sadapt.h#L96) | Fill defaults only; host supplies ladder/identity/initial index. No allocation or startup. |
| [`fvm_sadapt_init`](../include/flashvm/sadapt.h#L100) | Validate full ladder/config and copy native state atomically. Keep state at a stable address: its copied config points into its own ladder; do not memcpy/move a live policy to a different address. |
| [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L106) | Validate and consume one report; copy KEEP/INSUFFICIENT or a pending SHRINK/GROW hint. Failure preserves state/decision. No negotiation, callbacks, I/O or geometry change. |
| [`fvm_sadapt_commit`](../include/flashvm/sadapt.h#L111) | Acknowledge the exact pending geometry after control ACK; update current and begin cooldown. Wrong selection/no proposal CONFLICT. |
| [`fvm_sadapt_reject`](../include/flashvm/sadapt.h#L114) | Abandon pending candidate, preserve current and begin cooldown; no remote rollback. |
| [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L117) | Encode compact tag-zero report from validated counts into caller storage; output/used atomic. |
| [`fvm_sadapt_feedback_read`](../include/flashvm/sadapt.h#L119) | Verify one record and copy report only on OK; prefix/checksum errors propagate, output/used atomic. |

All caller state, input reports and output decisions must be disjoint and used
serially. No VM/GC references or process-global mutable state are retained.

## Build and validation

The module belongs to the detachable flashvm_srecord archive and core SDK;
no SDL or VM startup is required. Native C89 contract covers pending/reject,
cooldown, insufficient samples, unknown RTT, max Uint32 thresholds and service
framing. Independent Python big integers/zlib generate exact wire bytes and model
1200 observations, including commit/rejection and duplicate atomicity. C++98
public-header linkage is retained.

```sh
cmake -S flashVM -B build-adapt -DFVM_BUILD_TESTS=ON
cmake --build build-adapt --target flashvm_srecord_adapt_contract flashvm_srecord_oracle
ctest --test-dir build-adapt -R flashvm_srecord_adapt --output-on-failure
```

The generic policy does not install a network listener, quality sampler, new
video/audio codec or automatic production-player adaptation. A host must connect
measurements, control transport and frame-boundary activation explicitly.

## Feedback-driven real frame replay (v0.746)

The [executable frame_parts tutorial](../examples/player/frame_parts/README.md)
now accepts `--feedback reports.srecords`. Its bounded input consists of exactly
three compact feedback reports, decoded completely before opening output. Each
report must identify stream 7, epoch 1, window 1/2/3. During execution its expected
count must equal the preceding exported image's part count. This checks causal
sample extent; it cannot authenticate a file or prove measurements are truthful.

The four actual SWF images start at 1024/768. The independent test supplies a
controlled window with six expected, four on-time and one damaged part (one
missing), then clean eleven-part and six-part windows, known RTT 20 ms. The
policy selects 512/400, 1024/768, then 4096/4000. For this short replay only,
one bad/good window is sufficient, sample minimum is one and cooldown zero.
These explicit overrides are not production network tuning advice.

Each candidate travels through compact OFFER_AT/ACK at the next image boundary;
only the selected committed profile is passed to adaptation commit and the actual
retained-frame part writer. The transcript includes incoming feedback before
metadata for frames 2–4, so it represents both directions, not a one-way media
container. Without the option the original scripted three-frame mode is retained.

The independent reader checks every RGBA byte and wire field, then transmits the
actual parts on UDP loopback with damage/omission/reordering/duplicates. FILE
repairs only missing pieces via TCP; LIVE never requests repair. Negative cases
cover malformed/oversize/trailing reports, wrong identity/window/sample extent,
and CLI errors. Input decoding failures preserve an existing destination; a
failure after opening output removes that invocation's partial file.

This is explicit host-feedback replay, not an installed network-quality sampler,
congestion controller, authenticated remote service or video codec.

Validation at v0.746: 54 selected native regression checks and 14 shared-host checks passed. New policy/cache/protocol paths and the actual interpreter/JIT frame integration passed ASan/UBSan (leak detection disabled by the restricted host). Strict C89 link checks and C++98 consumers build for ARM64 Linux/musl, RISC-V64 Linux/musl and Windows x64/MinGW; foreign binaries were not executed. All nine roadmap stages remain open.
[Receive-window collector / Сборщик окон приёма](RECEIVE_WINDOWS.md) supplies unique counts with host deadlines.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sadapt_commit` | function | [include/flashvm/sadapt.h:111](../include/flashvm/sadapt.h#L111) |
| `fvm_sadapt_config_init` | function | [include/flashvm/sadapt.h:96](../include/flashvm/sadapt.h#L96) |
| `fvm_sadapt_feedback_read` | function | [include/flashvm/sadapt.h:119](../include/flashvm/sadapt.h#L119) |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:117](../include/flashvm/sadapt.h#L117) |
| `fvm_sadapt_init` | function | [include/flashvm/sadapt.h:100](../include/flashvm/sadapt.h#L100) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:106](../include/flashvm/sadapt.h#L106) |
| `fvm_sadapt_reject` | function | [include/flashvm/sadapt.h:114](../include/flashvm/sadapt.h#L114) |
<!-- source-links:end -->
