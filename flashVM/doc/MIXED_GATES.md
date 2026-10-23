# Mixed event scheduling through stream gates

The native and desktop mixed paths now keep event voices moving while a base
stream waits for a future movie frame or the root is authored-stopped. This
extends the v0.986 final-EOF-only integration. It does not add child stream
scheduling or reference Flash A/V/seek semantics.

## Native scheduler

[`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L111) coordinates one native production step for
an existing stream and its exact [`FVM_SWFSoundMixQueue`](../src/swf/sound_mix_queue.cpp#L13). Required live owners must
share the adapted callback table, geometry, active shared clock and accepted epoch.
The host-selected unsigned movie frame authorizes base; zero holds all base frames
while permitting event voices. The operation never chooses elapsed movie time from
a tick count. Caller serializes stream, converter, snapshot, mixer, mix queue and
exclusive backend externally; no ownership transfers or VM/GC access occur.

The scheduler first accepts a retained TAIL retry with its original bytes, even if
the gate now allows base. Otherwise it prefers authorized base work. If the next
source frame is held or source EOF is reached, it mixes voices over zero base.
Backpressure on an authorized base is not a gate and does not permit substitution.
A retained BASE retry remains held when its original frame becomes disallowed;
neither its voice advancement nor bytes may be silently undone or remixed.

| API | Result and output contract |
| --- | --- |
| [`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L111) | Optional accepted is cleared first. OK accepts one block or consumes one zero-frame source marker. Invalid stream, reentry, wrong backend or unequal epochs: INVALID_ARGUMENT. Missing shared clock, queue headroom, exhausted voices or a held BASE retry: NOT_FOUND; inspect producer/mix pending state rather than treating this alone as EOF. Native/backend statuses propagate, callback exceptions are ERROR. No allocation, VM, GC, implicit pause/clear or automatic A/V retiming. |
| [`fvm_swf_sound_mix_queue_pending`](../include/flashvm/swf_sound_mix_queue.h#L31) | Observe BASE/TAIL ownership without callbacks. A rejected event-only block lives here, independently of the stream's base PCM buffer. |
| [`fvm_swf_stream_output_info`](../include/flashvm/swf_stream_output.h#L126) | Fresh copied accepted/dequeued counters include successful base and event-only blocks. Source chunk/EOF and base pending retain their original meaning. Serialize with all scheduling/control calls; output is unchanged on error. |

Zero-base output uses the existing block/target and both exact clocks; it preserves
source converter position, phase, chunk and source EOF. Its final padding is real
accepted output, not extra source samples. No block is generated when voices and
mixed retry are exhausted, so a held source cannot create endless silent output.
Failed writes retain exact composition and increment neither accepted count;
a valid queue observation may still refresh played time. Continue using this
scheduler after a write rejection, or cancel with acknowledged seek/clear.
Direct unrelated writes or mix-tail calls invalidate exclusive accounting.
The older [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L92) keeps its observed-EOF precondition.

## Desktop frame and pause policy

The worker switches exhausted stream segments before synthesizing event-only work
for a successor's gate, and never switches away from a retained TAIL retry. A
completed-frame flag is separate from base authorization: while VM callbacks
prepare all frame events, both producers remain held. After frame completion,
`fvm_desktop_audio_timeline` publishes readiness. Authored root Stop then sets only
the base gate to zero; event voices may prime, resume and drain the real queue.
Host/session pause still suspends the complete output, and NoMultiple includes
accepted/pending ID time until dequeue.

`fvm_desktop_audio_stream_pending` includes authorized source work, accepted FIFO,
live events and processable mixed retry even under authored Stop. A retained BASE
whose original frame is held cannot progress; once accepted lead drains it does
not keep finite completion waiting forever. Resume or acknowledged seek/clear is
required to resolve that retained block. Already accepted mixed lead may drain on
root Stop; it cannot be selectively unmixed. The ordinary stream-only pause policy
is unchanged.

Context StopSounds retains its staged full cancellation. Failed clear preserves
source/mix retries and deadlines; successful clear resets both epochs and voices.
Later events can play without reactivating cancelled base. Seek/root replacement
still uses the existing cancellation policy, not reference event preservation.
Source sample time is not advanced by gate output: a later stream resumes its
retained grid after already queued event time. No drift correction, timestamp
alignment or sample-accurate seek is inferred from these queue clocks.

## Executable evidence and limits

The C89 `swf_stream_mix --gated` tutorial exports 17 exact stereo frames after VM
teardown: eight event-only frames, then nine base frames. Every write is deliberately
rejected; retry opens the gate to prove TAIL priority. The seven-mode checker uses
an independent rational PCM oracle. Native tests cover both gate directions,
NOT_FOUND write failures, throwing callbacks, reentry, no allocations, seeded
2^53 clock boundaries and 256 epochs on four real independent OS workers. The
white-box boundary test is not a long-duration device run.

Desktop tests cover held BASE/accepted lead, failed StopSounds clear, GC, stopped
root events and interpreter/native JIT entries. A real SDL producer verifies host
pause, stopped-root drain and later Play with unchanged source phase. Independent
CLI WAV tests cover FWS/CWS, both VM modes, authored Stop/future source gates and
mute without any audio device. The explicit SDK has dummy audio/video only; no
physical device/window, cross-platform or timing acceptance is implied.

Child dictionaries/timelines, complete codec/reference latency, sample-clock A/V
alignment and drift/seek, all JIT backends, broader concurrency/GC/debugger acceptance
and all nine roadmap stages remain open. Historical checkpoints follow.


v0.987: Refactor native stream work beneath one callback barrier while preserving base retry, frame gates and EOF-tail behavior.

v0.988: Add paired pump_mix_until: choose authorized base or event-only work while retaining source position and coherent clocks.

v0.989: Verify tail retry before newly authorized base, held base retention, callback recursion and backend exception containment.

v0.990: Route mixed worker steps through paired gate scheduling, preserve retry priority and activate source successors before synthetic events.

v0.991: Separate completed-frame publication from base authorization; root Stop holds stream phase while events continue and host pause still suspends the queue.

v0.992: Exercise seeded 2^53 frame boundaries, backend/clock rejection and zero-allocation gate/retry/clear paths after native preparation.

v0.993: Run 256 native source-teardown epochs with rejected tail retries, alternating gates, exact PCM witnesses and coherent dequeue/ID state.

v0.994: Keep held BASE retries intact, drain accepted lead, preserve failed clear and admit fresh stopped-root events only after staged cancellation.

v0.995: Add a seventh gated export mode with exact 17-frame PCM, held converter witness and deliberately rejected tail-to-base gate transitions.

v0.996: Exercise real SDL host pause, authored Stop, GC/native-JIT execution, event-only drain and subsequent base resume without converter rewind.

v0.997: Check exact event-only WAV for FWS/CWS in both VM modes under future stream gates and root Stop, plus mute without an audio device.


## v0.998 verification

Mixed event gates v0.987–v0.998: paired pump_mix_until advances voices at future stream gates and authored root Stop while preserving source phase, exact BASE/TAIL retry ordering and both clocks. Completed-frame publication still freezes production during callbacks; host pause holds the complete queue. A held BASE cannot be replaced and needs resume or acknowledged cancellation; accepted mixed lead is not selectively recalled. Validation passes 172 native regression/tutorial/CLI tests, 37 selected ASan/UBSan cases plus nine explicit --no-throw runs, and eight SDK-free core/tutorial checks. Ordinary builds cover allocation/exception paths; LeakSanitizer is disabled. Native tests include seeded 2^53 bounds, zero-allocation scheduling and 256 epochs on four independent OS workers; not unsynchronized shared-owner or long-duration device acceptance. Real root actions/native JIT/GC, a real SDL stopped-root producer, exact FWS/CWS CLI WAV/mute and the seven-mode C89 rational PCM oracle pass. A renamed detached component configures with an explicit SDK and its copied seven-mode checker passes against the native executable; no separate detached build. SDL is dummy-only. Child streams/dictionaries, full codec/reference latency, sample-clock A/V/drift/seek, all backend/platform/device acceptance and all nine stages remain open. [Contract](MIXED_GATES.md).


v0.998: Complete C89/API and desktop contracts for authored Stop, ordered retry, gated PCM and remaining reference/device limits.


## Adaptive headroom since v0.1000

Paired gate/EOF scheduling uses [`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L262) for fresh
event-only work. It fills available FIFO room up to the configured block. A retained
short TAIL stays indivisible and wins over newly authorized base. Both clock copies
use the final successful admission observation, without a fallible post-write poll.
The older explicit exact-size tail API is unchanged. See [full contract](ADAPTIVE_TAILS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:262](../include/flashvm/swf_sound_mix_queue.h#L262) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:126](../include/flashvm/swf_stream_output.h#L126) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:92](../include/flashvm/swf_stream_output.h#L92) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:111](../include/flashvm/swf_stream_output.h#L111) |
| `FVM_SWFSoundMixQueue` | struct | [src/swf/sound_mix_queue.cpp:13](../src/swf/sound_mix_queue.cpp#L13) |
<!-- source-links:end -->
