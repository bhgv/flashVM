# Coherent mixed stream snapshots

[`fvm_swf_stream_output_mix_info`](../include/flashvm/swf_stream_output.h#L162) observes the paired stream and accepted BASE/TAIL
ledger through exactly one real `queued` callback. Separate queries can observe
separate points while the device drains: use this API when category sums must
match source/retry metadata and the common clock in one result.

## Contract

Pass a live initialized stream, its exact adapted mix queue, and disjoint writable
[`FVM_SWFStreamMixInfo`](../include/flashvm/swf_stream_output.h#L28) with only outer struct_size set to sizeof. Nested sizes are
initialized on success. The stream's submitted epoch must equal the queue's;
origin tracking must be enabled. Keep converter, immutable source snapshot, mixer,
backend userdata and retry PCM alive. Externally serialize both owners and queue
lifecycle. No internal lock provides cross-thread ownership.

The returned stream has the same source chunk, pending BASE frames, EOF and pause
metadata as its owner. A retained TAIL is still queried via mix_queue_pending;
it does not become stream pending PCM. Submitted/played/queued category sums equal
the common stream clock. Seconds is common dequeued/output-rate, including TAIL;
it is neither source position nor device/DAC time. BASE includes silence and mixed
events; TAIL includes explicitly chosen event-only padding/release. Rejected PCM
is excluded until acknowledged acceptance.

Exactly one successful native observation updates the queue ledger and stream
clock cache, then copies out. No decode, render, allocation, source advancement,
frame authorization, pause, clear, VM or GC occurs. A callback can block according
to the backend contract. The stream busy guard rejects recursive public calls;
the mix guard also protects the callback. This is not an OS-thread mutex.

Invalid pointers/outer size/reentry, wrong adapted backend, unequal submitted epoch,
regressed cached dequeue or backlog above stream target return INVALID_ARGUMENT.
Disabled origin tracking returns UNSUPPORTED. Backend statuses propagate; callback
exceptions return ERROR. Invalid queue growth/dequeue regression returns ERROR.
Errors preserve caller output and stream cache. A valid queue observation may
already retire its native origin ledger before a stream-target rejection; device
progress cannot be undone. Acknowledged seek/clear resets both clocks; failed clear
retains old pending/epoch with any acknowledged pause visible. Continue preserves
the common epoch, including category totals from preceding source segments.

## Desktop and example

`fvm_desktop_audio_mix_info` holds the producer mutex and applies sequence EOF
policy (a later prepared header means sequence EOF is false). Mixed worker refill,
resume/completion and public stream diagnostics use the same paired observation.
Player --stats reads origin categories from this result. The existing standalone
origin API remains available. No A/V pacing policy changes here.

The C89 swf_stream_mix --origins example demonstrates one result before and after
software drain: common17/0/17 then17/17/0, BASE9 and TAIL8. Its independent ten-mode
checker verifies exact PCM as well as both snapshots. The software consumer is
not a physical playback device.

## Evidence and open boundaries

Native tests cover one-poll FIFO drain, unchanged source cursor/retry bytes, disabled
mode, callbacks/errors/exceptions/reentry, mismatched owners/epochs, pending BASE,
failed clear and acknowledged reset. Four independent OS workers execute 256
finite mixed epochs with exact PCM; allocation denial covers paired queries.
White-box seeds cover ring wrap and the exact 2^53-1 clock limit without claiming
huge PCM or long playback. Desktop workers exercise real SDL dummy callbacks,
Stop/Play, host pause, GC and interpreter/native JIT.

This is coherent bookkeeping, not source-sample mapping, MP3 delay compensation,
automatic A/V drift/seek, child timelines/dictionaries or physical device/platform
acceptance. All nine roadmap stages remain open.

Internal desktop stream observations retain the legacy path for externally prepared composition adapters without origin tracking. Disabled-origin detection has no queued callback; the fallback still polls the FIFO once. The explicit paired API continues to return UNSUPPORTED.

Desktop internal selection uses the immutable prepared composition_origins mode, not a returned callback status. A real backend UNSUPPORTED therefore propagates after exactly one FIFO call; it never triggers a second fallback observation. Manually prepared frontend owners select the mode after enabling origins.

Coherent mixed snapshots v0.1051–v0.1063 add one real FIFO observation for source/retry metadata, the common dequeue clock and BASE/TAIL categories. Desktop refill, resume/completion and statistics use the paired path under the producer mutex; a prepared immutable mode preserves legacy adapters and propagates backend statuses without another poll. Final validation passes 215 selected native regression/tutorial/CLI checks plus two options tests; 68 selected ASan/UBSan checks plus fourteen explicit --no-throw runs; all 17 mixed frontend sanitizer checks rebuilt after the final mode/status fix; and 12 SDK-free core/tutorial checks. Ordinary builds cover exceptions/allocation failures; LeakSanitizer is disabled. Evidence includes one-poll draining FIFO, unchanged source/retry bytes, pending BASE/TAIL, failed clear, reentry, output preservation, exact-clock/ring seeds, allocation denial, 256 independent OS-worker epochs, and real SDL dummy Stop/Play/pause/GC/interpreter/JIT workers. The ten-mode C89 example retains exact PCM and verifies queued/drained paired reports. Source/DAC timestamps, codec latency, automatic A/V/drift/reference seek, child dictionaries/timelines and physical-device/platform acceptance remain open; all nine stages remain open.


v0.1051: Add a single-observation paired stream and BASE/TAIL query without rendering or source advancement.

v0.1052: Verify single-owner rejection, disabled mode, callback errors and exceptions, pending BASE, failed clear and epoch reset.

v0.1053: Four OS workers verify exact PCM and one-poll snapshots across 256 epochs; allocation denial covers snapshots during retained retries.

v0.1054: Add the desktop mutex-protected paired query and use it for mixed player statistics; preserve sequence EOF policy.

v0.1055: Real SDL dummy workers check category/common sums while draining authored-Stop tails and resumed base in interpreter and JIT.

v0.1056: Mixed refill, resume, completion and public stream diagnostics observe the stream cache and origin ledger together.

v0.1057: The origins tutorial reports one-poll queued and drained snapshots; its independent ten-mode PCM checker verifies both reports.

v0.1058: Seeded tests cover one-poll exact-clock boundaries, ring wrap, regression, target mismatch and acknowledged reset.

v0.1059: Specify one-callback snapshot semantics, stream-cache publication, nonrollbackable queue progress and desktop lifecycle ownership.


v0.1060: Keep internal stream observation compatible with prepared adapters that omit optional origins; explicit paired queries still report unsupported mode.


v0.1061: Run exact retry and final drain through both adapter modes; verify explicit paired unsupported preservation and enabled category totals.


v0.1062: Publish immutable observation mode before worker start and preserve a backend UNSUPPORTED after one callback; verify output and poll count.


v0.1063: Record native sanitizer SDK-free and C89 evidence for paired observations, legacy mode and callback status propagation; retain open source-time and A-V acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_output_mix_info` | function | [include/flashvm/swf_stream_output.h:162](../include/flashvm/swf_stream_output.h#L162) |
| `FVM_SWFStreamMixInfo` | struct | [include/flashvm/swf_stream_output.h:28](../include/flashvm/swf_stream_output.h#L28) |
<!-- source-links:end -->
