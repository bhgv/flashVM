# Desktop root stream/event composition

Current gate/authored-Stop scheduling: [contract](MIXED_GATES.md). The final-EOF-only and root-pause limitations below describe the original v0.986 integration checkpoint, superseded for mixed scheduling.

Current scope: the desktop owner accepts supported root stream segments together
with root StartSound voices. The stream writes base PCM through one tracked native
composition adapter into the existing SDL queue. The mixer adds all current event
voices before a single S16 saturation. Existing event-only and stream-only paths
remain separate. This is bounded root integration, not complete SWF multimedia acceptance.

## Ownership and frame publication

`fvm_desktop_audio_open_paused` snapshots bounded root sources on the VM owner,
creates the mixer and the optional [`FVM_SWFSoundMixQueue`](../src/swf/sound_mix_queue.cpp#L14), begins its empty shared
epoch, enables ID tracking, then initializes the stream producer against that
adapter. Only the native worker renders copied sources; it retains no VM/GC pointer.
Every mixer/producer/control operation shares the existing SDL mutex.

`fvm_desktop_audio_validate` and acquisition use the same checked predictor for the
separate `FVM_DESKTOP_AUDIO_MIX_BYTES` ceiling (default 1 MiB). It includes adapter
owner, both PCM buffers and ID deadlines, excluding allocator overhead, mixer/voices,
source snapshots, trace and device. The existing event and prepared-source budgets
remain independent. A deficient composition ceiling fails preflight before native
window/device acquisition. Partial acquisition is released by normal owner cleanup.

The real root callback prepares temporary voices outside the native mutex, then
adopts them under it. NoMultiple and per-ID stop use the tracked queue, including
pending/accepted time after mixer voice retirement. A navigation reset is staged
before new frame events are admitted. While callbacks are preparing those events,
base and tail production are held at frame zero. The owner calls
`fvm_desktop_audio_timeline` after the complete frame to publish actual authorization.
Thus a fast worker cannot render the new base before the frame's event batch arrives.
Do not substitute a tick count or call this publication midway through delivery.

## EOF tails and controls

The worker only yields to event-only tails after final sequence EOF. At intermediate
source EOF it retains the existing source-order/frame gate and phase reset rules.
At future stream gates, event conversion waits with the base producer; independent
wall-clock event advancement through stream gaps remains open. Root authored stop
and host suspension hold this composition under the current stream pause policy.
This does not certify original Flash pause/seek or sample-accurate event placement;
new voices start beyond already accepted/pending PCM.

[`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L106) coordinates a final EOF tail with both queue
epochs without adding fields to public structures. Its stream must have observed
source EOF, own no base retry, use the exact adapted callbacks/userdata/geometry of
the supplied live mix queue, and have the same accepted-frame count as its active
shared clock. It uses that producer's block and target. On success both accepted
counts include the tail and [`FVM_SWFStreamOutputInfo`](../include/flashvm/swf_stream_output.h#L12) remains valid. Source EOF,
converter position and last source chunk remain unchanged. Final block silence is
counted as submitted PCM, not source samples or physical DAC playback.

| API | Status, ownership and execution contract |
| --- | --- |
| [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L106) | Required live stream/mix owners; optional accepted is cleared first. Null, reentry, no observed EOF, base retry, different backend or inconsistent epoch: INVALID_ARGUMENT. Inactive shared clock: NOT_FOUND. No voice or queue headroom: NOT_FOUND. Other backend/mixer statuses propagate; callback exceptions are ERROR. No allocation/VM/GC, ownership transfer or implicit pause/clear. Externally serialize all borrowed dependencies. |
| `fvm_desktop_audio_stream_pending` | Include authorized source work, accepted FIFO, live mixed voices and retained mix retry. Host suspension or authored stop returns zero; after StopSounds, a newly delivered event may keep completion alive while the base remains cancelled. No pumping, decoding or VM access; mutex-serialized and output preserved on failure. |

A rejected tail write advances the mixer once and remains an exact TAIL retry in
the mix queue. The producer's base buffer is not repurposed;
`FVM_SWFStreamOutputInfo.pending_frames` still means base retry only. Query
[`fvm_swf_sound_mix_queue_pending`](../include/flashvm/swf_sound_mix_queue.h#L38) for the retained tail. A valid FIFO observation may
refresh played time on a failing operation; no failed write increments accepted time.
Do not interleave direct mix-tail writes or unrelated queue producers with paired
tails. Settle a tail retry before [`fvm_swf_stream_output_continue`](../include/flashvm/swf_stream_output.h#L186); acknowledged
seek/clear cancels it. Explicit epoch mismatch is rejected before further rendering.

Context StopSounds uses staged stream seek/clear through the adapter. Successful
clear cancels base retry, mixed retry, voices, deadlines and both counters, and holds
the base cancelled until navigation. Failed clear preserves those states (a successful
pause stays visible). New later root events can play after cancellation without
reactivating cancelled sources or later headers. Navigation restarts a fresh epoch;
root replacement detaches callbacks and joins the native worker before freeing owners.

## Validation and remaining work

The executable C89/C++98 fixtures cover exact PCM, rejected base/tail writes, copied
clock agreement, opposite voices before saturation, phase reset across segments,
future gates, queued NoMultiple, failed clear and retry, pause, rewind, actual root
StopSounds actions, GC and interpreter/native-JIT entries. A real SDL native worker
uses an explicit dummy SDK and verifies frame publication before and after source EOF.
Deterministic FIFO fixtures compile the production C89 worker directly; they do not
claim device timing. Source-VM teardown is tested before native conversion completes.

Child dictionaries/timelines, independent event progression through stream gaps,
full codec/reference latency and seek, automatic A/V drift policy, all JIT backends,
concurrency/GC/debugger acceptance, physical devices/platforms and all nine roadmap
stages remain open. Historical checkpoint reports below do not change this scope.


v0.975: Add [fvm_swf_stream_output_pump_mix_tail](../include/flashvm/swf_stream_output.h#L106) for final EOF: exact retry and coherent accepted/dequeued counters; no public struct change.


v0.976: Verify exact backend pairing, rejected/throwing retry, stream reentry and independently advanced epoch rejection.


v0.977: Acquire/close optional native composition ownership and route serialized ID controls; keep mixed preflight closed until integration checks.


v0.978: Allow bounded root mixed acquisition and synchronize navigation before adopting each frame event; preserve legacy event/stream paths.


v0.979: Pump paired tails only at final source EOF, include active/pending events in completion and keep StopSounds-held sources cancelled.


v0.980: Exercise production C89 refill against an explicit software FIFO, retained base retry and exact 5 base-plus-event/4 tail PCM frames.


v0.981: Execute actual root actions through interpreter/native JIT, queued ID suppression, failed clear, GC and post-StopSounds event restart.


v0.982: Cover retained EOF-tail completion, pause without rewind, failed navigation clear and fresh event admission in the new epoch.


v0.983: Withhold base and EOF-tail production during event preparation; verify real SDL worker handoff before and after stream EOF.


v0.984: Apply the same separate composition predictor during preflight and open, with partial-owner cleanup and no endpoint on insufficient budget.


v0.985: Check source transitions, future gates, rewind epochs and exact opposite-voice composition through the production C89 worker after VM teardown.


Continuous stream interpolation may inspect the next immutable sample within the same prepared header even when its block is not yet write-authorized. A frame gate limits prepared/accepted block ownership, not that interpolation lookahead. The corrected ADPCM future-gate oracle computes the complete same-header rational grid and compares its authorized prefix, rather than treating an early host stop as source EOF.


## v0.986 verification

Desktop root composition v0.975–v0.986: supported root streams and StartSound events share a budgeted tracked mixer queue, single saturation, NoMultiple/stop routing, staged StopSounds/seek and final EOF tails with coherent stream/shared clocks. Frame permission is withheld until all owner callbacks complete, including after EOF. Validation totals 163 distinct native regression/tutorial/CLI tests (the existing ADPCM early-stop oracle was corrected to compare the continuous same-header prefix and rerun); 31 selected ASan/UBSan cases plus eight explicit --no-throw runs pass. One sanitizer executable initially lacked execute permission and passed after permission repair. Three additional SDK-free core/tutorial checks pass. FWS/CWS mixed CLI in both VM modes matches 1033 exact stereo frames including padding; root action tests assert actual native JIT entries and GC safe points. The SDL SDK is static dummy-only: no physical-device/window acceptance. LeakSanitizer is disabled; ordinary builds cover exceptions/allocation faults. A renamed detached component configures with an explicit SDL SDK and its copied six-mode PCM checker passes against the native executable; not separately built. Independent events through stream gaps/authored pause, child dictionaries/timelines, reference codec/seek/A-V policy and all nine roadmap stages remain open. [Contract](DESKTOP_MIX.md).


v0.986: Complete root composition contracts, mixed CLI oracle and continuous ADPCM prefix expectations; record tested limits and remaining player work.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:38](../include/flashvm/swf_sound_mix_queue.h#L38) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:186](../include/flashvm/swf_stream_output.h#L186) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:106](../include/flashvm/swf_stream_output.h#L106) |
| `FVM_SWFSoundMixQueue` | struct | [src/swf/sound_mix_queue.cpp:14](../src/swf/sound_mix_queue.cpp#L14) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:12](../include/flashvm/swf_stream_output.h#L12) |
<!-- source-links:end -->
