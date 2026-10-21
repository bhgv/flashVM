# Root PCM header sequences in the SDL player
v0.819 validates MP3 header/side-information CRC and preflights every statically
referenced root MP3 once per sound ID before unmuted device acquisition. Stop-only
references and mute do not decode. [Admission and limits](MP3_CRC.md).


v0.780 adds supported root MP3 streams and MP3 event decoding; bounded native preparation, exact queue length and independent PCM checks. Mixed root stream/event movies, negative initial latency and reference MP3 seek/A/V remain open. Free-format support was added in v0.806–0.811; short events without enough framing evidence remain unsupported. [Free-format contract](FREE_FORMAT_MP3.md). [Current MP3 contract](MP3_AUDIO.md).

Since v0.758 the actual desktop player accepts the original single validated root
SoundStreamHead/Head2
segment in PCM_NATIVE or PCM_LE format, with no root StartSound events. It converts native
source samples to 48000 Hz interleaved stereo S16 and feeds the real SDL queue.
In the original v0.758 checkpoint, codecs, multiple headers and mixed movies failed
explicitly
during preflight, before window/device creation. --mute continues to validate and omit
root streaming controls without opening audio. Strict core scene import remains strict;
the player uses the exact-source root-stream omission flag and separately snapshots the
validated source before starting its worker.

## Aggregate source preparation (v0.767)

The default player now bounds the segment table, retained snapshots and next-source
expansion together to 64 MiB before native startup. Preflight includes temporary
PCM copies as well as ADPCM decode. [Budget contract and checks](PLAYER_STREAM_BUDGETS.md).
The per-snapshot-only statements in older checkpoints below are historical.

## Bounded finite-run stream completion (v0.765)

Finite `--frames`/`--once` playback now retains final-frame visibility and then
waits for already-authorized stream blocks and accepted queued PCM to finish.
[fvm_desktop_audio_stream_pending](../frontend/common/audio.h#L102) copies this condition under the producer mutex;
it never pumps PCM, allocates, decodes, advances a movie or authorizes a future
frame. Empty inactive headers are skipped in a bounded scan; later sources are
inspected at block zero because the worker re-primes them when reached.
Errors preserve the output argument. An observed worker/backend error propagates.

No stream, host suspension, authored stop or StopSounds cancellation needs extra
completion. Event-sound output keeps its existing final-frame policy. A future
stream block beyond the current movie gate does not keep a finite run alive.
Normal stream completion includes FIFO drain, not merely source EOF/submission;
this software observation is still not a DAC/speaker clock.

The additional deadline is [FVM_DESKTOP_AUDIO_TAIL_MS](../frontend/common/audio.h#L20) (2000 ms), starting after
the later of the scheduled final-frame deadline and entry to tail processing.
This cooperative deadline cannot preempt a blocked SDK/host callback.
Pending work at expiry fails playback with `stream completion deadline exceeded`
and performs normal joined cleanup. Queue/backend failures likewise fail the run.
Thus finite slow/oversized/stalled streams may need a different authored frame
bound/rate; there is no silent-success truncation at the deadline. This does not
implement general audio-driven movie pacing, seek correction or time stretching.

Owner TURN notifications and window events continue during completion; STOP,
window close, root replacement, pause and background suspension can end/skip the
extra wait. PCM stays on the worker. No public host descriptor or VM ABI layout
changes. The previous severe-starvation truncation report below is historical:
this bounded completion replaces that unconditional final-frame shutdown.

The real-queue [stream_pending.cpp](../frontend/tests/stream_pending.cpp) checks authorization, empty/current/inactive
sources, unchanged producer position, accepted queue data, pause/cancel and error
preservation with a quiesced worker. The deterministic [tail_stream.py](../frontend/tests/tail_stream.py) runs the
actual owner loop against a protocol presenter and checks completion at 180 ms,
close before 130 ms, backend failure, and exact timeout at 2100 ms after a 100 ms
frame. It is explicitly not a device test. The CLI oracle also forces `--fps 1000`
for PCM/ADPCM and a one-frame future gate, requiring complete authorized PCM
while excluding later blocks. Existing tail background tests retain
pause/resume checks; their fake endpoint now supplies all current native audio
observations and a non-dereferenced device marker.

## Whole-packet ADPCM in the player (v0.764)

Root ADPCM is now accepted alongside PCM within the existing 64-header limit,
still without root StartSound mixing. Code widths 2–5, mono/stereo and all supported
source rates use the [explicit whole-packet contract](STREAM_ADPCM.md). Partial
packets/other codecs remain unsupported. Raw/muted stream inspection stays opaque;
muted ADPCM does not acquire snapshots or decode compressed payloads.

Before opening a window/device, [fvm_desktop_audio_validate](../frontend/common/audio.h#L76) temporarily prepares
and closes ADPCM snapshots. It can now allocate/decode and return OUT_OF_MEMORY;
malformed padding and unsupported packet geometry propagate. Later audio acquisition
checks header policy and prepares each retained snapshot once, before opening its
SDL queue. This intentionally repeats ADPCM decoding between preflight and acquisition
so the preflight does not retain hidden ownership. Preparation is startup work,
not work done by the native producer. The existing per-preparation budget is not
a combined cap on all 64 retained segments.

Retained ADPCM snapshots expose normalized PCM_NATIVE/S16. The ordinary converter,
frame gate, queue continuation, stop/pause/navigation and joined cleanup paths are
shared with PCM. No decoder, VM access or allocation is added to the worker.
The public player-host descriptors remain unchanged. The new [ADPCM movie and
regenerator](../examples/player/pcm_stream/README.md#whole-packet-adpcm-variant-v0764)
are linked from the example catalog; FFmpeg is needed only to regenerate/check them.

[stream_adpcm.py](../frontend/tests/stream_adpcm.py) independently packs constant differential codes for all eight
width/channel combinations and checks every accepted sample after integer rational
conversion. FWS/CWS, interpreter/native JIT and a PCM successor exercise the actual
CLI (32 runs). Optional `--ffmpeg` independently decodes the bundled encoder-generated
movie and checks all 71332 accepted stereo frames. The worker fixture also exercises
ADPCM across real stop/play, GC, StopSounds and backward/same-frame/forward navigation.
The failure sweep starts with ADPCM and includes already-prepared later PCM headers.
These tests use dummy SDL audio/video, not physical window/speaker acceptance.

Under severe build-host starvation a finite run may close before every authorized
PCM frame has been produced. One pre-existing finite PCM tutorial oracle exposed
this during concurrent cross linking; the isolated rerun is recorded separately.
This change does not implement audio-driven movie pacing or an end-of-run drain
handshake and does not claim those timing limitations are solved.

## Header sequences (v0.761)

The player now accepts up to [FVM_DESKTOP_AUDIO_MAX_SEGMENTS](../frontend/common/audio.h#L22) (64) validated PCM
headers, still without root StartSound. Every header must be PCM_NATIVE or PCM_LE;
compressed codecs, root event/stream mixing and excess header count fail before
window/device acquisition. Muting still validates arbitrary indexed root segments
without creating snapshots or opening audio. The current limit bounds startup
preparation and navigation scans, not the 9P packet size or media channel geometry.

The owner allocates a zero-initialized array of [FVM_DesktopStreamSegment](../frontend/common/audio.h#L26) and
prepares all snapshots/converters before starting a worker. Each entry owns its
snapshot and independent phase grid. On partial preparation failure, close joins
any worker first, closes every nonnull entry once and frees the array; the old
stream_source field is only a borrowed first-entry alias. No snapshot leaks into
VM roots, callbacks or the SDL device. Startup validates/indexes per segment and
may allocate; worker transitions allocate nothing and never read a context.

On source EOF, [fvm_swf_stream_output_continue](../include/flashvm/swf_stream_output.h#L89) preserves accepted queue bytes,
pause and cumulative submitted/played clocks. The device remains open once.
Output stays 48000 Hz stereo S16 while source width, mono/stereo and rate may
change per header. Phase carries across blocks within one header and restarts at
zero for a new header. Before activating an inactive successor, its cursor resets
to block zero so earlier loops/seeks cannot leave stale EOF positions. Header
transitions, empty/skipped markers and PCM preparation each consume one of the
same [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L18) steps per worker iteration. A chain of 64 empty
headers cannot become an unbounded tight loop. stream_info reports aggregate EOF
only when the active source is the last header and is exhausted; FIFO drain and
StopSounds cancellation remain separate concepts.

Explicit/backward/skipped navigation selects the last header at/before the target
frame (source order breaks equal-frame ties), or the first future header before
any header has begun. [fvm_swf_stream_output_bind_frame](../include/flashvm/swf_stream_output.h#L96) stages pause/clear and
rebinds the selected converter only after acknowledged clear. It chooses the
first block at/after the target inside that segment. Gaps still hold future data;
no silence is synthesized. Later successors are re-primed when reached. This is
an explicit deterministic prefix-discard policy, not certification of historical
Flash header/seek semantics, drift correction or compressed stream decoding.

[stream_segments.py](../frontend/tests/stream_segments.py) independently constructs FWS/CWS with U8 mono 11025, an empty
header, S16 stereo 22050 and S16 mono 44100 sources, actual frame actions, gaps and
an empty/short block. It compares every accepted sample to integer per-header
rational arithmetic, including a second actual MovieClip loop and exactly three
segment transitions per traversal with no intermediate FIFO clear/device reopen.
The bundled pcm-segments.swf also changes format midway and independently checks
all 96000 output frames. [stream_worker.cpp](../frontend/tests/stream_worker.cpp) repeats actual stop/play/GC/StopSounds,
backward/same-frame navigation and an explicit forward cross-header seek; preflight
checks accept 64 headers and reject 65 before native setup. Dummy output does not
certify audible hardware, other operating systems or other CPUs. Older validation
counts below describe v0.759 only; they are not fresh v0.761 results.

The [stream_prepare_failure.cpp](../frontend/tests/stream_prepare_failure.cpp) fault sweep checks cleanup after partial source preparation, including several
already-owned snapshots; ASan exception sweeps use one process per allocation budget on
this restricted host.

## Build and run

Use a standalone FlashVM build, its documented SDL2 static SDK and a rendering profile
from [player builds](PLAYER_TEST_BUILDS.md); the core alone still needs no SDL. Enable FVM_ENABLE_SDL_AUDIO and
FVM_BUILD_DESKTOP_FRONTEND. After building flashvm_player:

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

Use JIT only on an available backend. The [reproducible example](../examples/player/pcm_stream/README.md) changes its background and tone after one
second. It is a runnable Python-generated SWF, not a compiler implementation. The
structured [fvm_desktop_run](../frontend/common/player.h#L39) and [fvm_desktop_run_memory](../frontend/common/player.h#L47) entry points use the same player path; no CLI-only state or ABI
fields were added. Existing source-base and load policy is unchanged. Replacement/unload
still uses the strict existing import policy; child/sprite streams are unsupported.

## Owner and worker

The owner imports/pins the root and calls [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83) at host service points.
Selected-but-not-entered frames authorize nothing. The scalar entered frame and local play
flag are published under the audio mutex; no context, clip or managed value is read by the
worker. The worker owns the prepared snapshot, converter position and retry buffer, and
calls [fvm_swf_stream_output_pump_until](../include/flashvm/swf_stream_output.h#L64) at most [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L18) times per service iteration. The SDL backend copies accepted PCM; it
never calls into VM/GC. Queue targets use the actual obtained driver quantum. Short
authorized blocks and EOF resume with a nonempty queue even below the usual reserve,
avoiding a startup deadlock when a frame contains little audio.

Host/session/background suspension and the clip's stop flag both pause the queue and
preserve accepted bytes. play resumes authorized data. StopSounds pauses, clears and
cancels the stream until a detected navigation reset. There is no stream stop-release
ramp. Pending, backward or skipped navigation invokes [fvm_swf_stream_output_seek_frame](../include/flashvm/swf_stream_output.h#L83) under the same mutex, clearing
old queued/pending bytes and starting phase zero at the first block at/after the selected
frame. Gaps hold a later block; they do not synthesize silence. Source EOF and drained
queue are distinct.

Since v0.759 the owner also reads [fvm_movieclip_get_navigation](../include/flashvm/movieclip.h#L93). A changed per-instance token forces the same
clear/seek even when a goto selected and entered the same frame entirely between
observations. Explicit nextFrame/prevFrame also change it; ordinary ticks/play/stop do
not. Pending/backward/skipped frame checks remain in place, including natural loop rewind.
Observation is still not a complete navigation history: intermediate targets and effects
are not replayed. Clock drift/drop/stretch, stream+event mixing,
MP3, partial ADPCM packets, video synchronization and reference Flash seek semantics remain
open. No claim of all Flash streaming behavior is made. Root unload detaches delivery,
joins the worker and closes snapshot/queue/device ownership before the context is
destroyed; GC runs only on the owner at normal safe points.

## Intrinsic navigation token (v0.759)

`fvm_movieclip_get_navigation(context, clip, &token)` copies an unsigned long token. Null output, malformed/foreign/prototype state or
GC/teardown phases return INVALID_ARGUMENT and preserve the output.
Owner-thread/same-context/retained-clip restrictions match [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83). No allocation, getters,
callback, advancement or managed pointer escapes. Compare equality for the same retained
clip/content only. New and old graphs without the optional private record return zero.
Validated explicit navigation increments the token, including a same-frame goto; [FVM_MOVIECLIP_NAVIGATION_MAX](../include/flashvm/movieclip.h#L85) wraps
to one. The optional numeric record follows ordinary graph serialization, but neither
transfer nor reload creates a globally unique identity. Host content lifetime/transport
epoch remains separate. MAX transitions between two observations can wrap; this is not a
monotonic clock or complete navigation log.

The first navigation lazily allocates its private numeric slot, so old graphs remain
accepted and ordinary construction/ticks gain no new allocation. Failure before
publication preserves the token. A later native seek allocation/error can retain an
already published token, like other partial committed seek effects; the conservative host
response is to clear stale PCM. Unknown labels/invalid targets do not publish a token. [movieclip_playback_info.cpp](../tests/movieclip_playback_info.cpp)
verifies initial/legacy zero, no-allocation reads, failed lookup, next/previous, wrap,
corrupt-output atomicity and allocation failure. [stream_worker.cpp](../frontend/tests/stream_worker.cpp) proves a goto+actual entry between
observations resets the queue even with an identical frame number.

## Internal host API and ownership

These functions are in [audio.h](../frontend/common/audio.h), implemented in [audio.c](../frontend/common/audio.c). They are private frontend helpers, not a
new stable plugin ABI. The existing public player C entry points retain their signatures.

| Operation | Contract |
| --- | --- |
| [fvm_desktop_audio_validate](../frontend/common/audio.h#L76) | Owner-thread root count/format/event policy; OK for no streams, UNSUPPORTED for unsupported combinations or excess header count, malformed stream status propagates. No native device or retained ownership is acquired; ADPCM preflight may temporarily allocate/decode. |
| [fvm_desktop_audio_open_paused](../frontend/common/audio.h#L85) | Acquire snapshot/converter/FIFO/device/mutex/worker and stop binding only for actual audio. Supplied pause is 0/1. Errors propagate; caller must close even after partial acquisition. Snapshot is independent of VM lifetime, but owner unregisters stop binding before context destruction. |
| [fvm_desktop_audio_timeline](../frontend/common/audio.h#L81) | Owner reads retained same-context clip/playback/navigation token and publishes copied policy under mutex. VM query, seek and backend errors propagate. Navigation clear/pause has staged external effects; it is not rollback. No worker VM call. |
| [fvm_desktop_audio_stream_info](../frontend/common/audio.h#L97) | Stream-only copied [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L11) under mutex; invalid input/error preserves descriptor. May wait for worker lock/backend observation. It reports FIFO dequeue position, not DAC/movie synchronization. |
| [fvm_desktop_audio_pause](../frontend/common/audio.h#L92) / [fvm_desktop_audio_tick_paused](../frontend/common/audio.h#L90) | Preserve host pause independently of local play; pause never clears bytes, tick polls latched worker error. Both require serial host use; worker is internally serialized. |
| [fvm_desktop_audio_clear](../frontend/common/audio.h#L105) / [fvm_desktop_audio_close](../frontend/common/audio.h#L107) | Owner detaches binding/callback, joins worker, closes snapshot and real device, then destroys mutex. Clear retains cumulative diagnostics. Never destroy a context or free userdata before detachment/quiescence. |

The [memory_movie.c](../frontend/tests/memory_movie.c) regression also plays a copied mutable `char buf[]` PCM movie through [fvm_desktop_run_memory](../frontend/common/player.h#L47) with a
deliberately non-file source label and verifies the borrowed bytes are unchanged. It uses
dummy audio when SDL audio is enabled.

## Evidence and limits

[stream_worker.cpp](../frontend/tests/stream_worker.cpp) exercises actual SDL dummy queue output with a real pinned MovieClip: initial hold,
frame gaps, slow owner, stop/play, GC, EOF, StopSounds, backward navigation, host
suspension and joined cleanup, in interpreter/JIT selections. [stream_audio.py](../frontend/tests/stream_audio.py) independently creates
FWS/CWS with actual AVM1 frame actions and compares every accepted PCM sample from the
real CLI trace against 9600 exact stereo frames. [stream_preflight.py](../frontend/tests/stream_preflight.py) checks supported and
unsupported/malformed/muted combinations with deliberately unavailable drivers on rejected
paths. Dummy audio validates queue/control behavior, not audible hardware, other operating
systems or other CPUs. FVM_AUDIO_TRACE explicitly opts into bounded RAM capture and file
output on close; it records accepted blocks, including bytes later discarded by a clear,
not speaker playback. All roadmap stages remain open.
At the original checkpoint audio_underruns counted event-producer observations only.
Since v0.769, [native stream recovery](PLAYER_STREAM_RECOVERY.md) also counts observed
empty-playing transitions with authorized work and re-primes before resume. Neither
path measures hardware underrun duration or infers DAC behavior from an empty FIFO.

Native v0.759 validation: 176 selected player/frontend/stream/MovieClip checks and
16 shared-host checks passed. The standalone minimal profile passes the playback
observation and actual timeline gates without the desktop frontend. ASan/UBSan
instrument the edited native stream, MovieClip and frontend modules with linked
ordinary core/SDL dependencies; this is not full-VM instrumentation or leak acceptance.
Strict C89/C++98 objects compile for ARM64 Linux, RISC-V64 Linux and Windows x64;
foreign executables/devices were not run. The two-second example is also parsed
independently and all 96000 accepted stereo output frames match an integer oracle.

Validation for v0.761: 101 selected native player/frontend/stream checks, 18 shared-host
checks and three standalone minimal-profile output/API checks pass. Edited modules
pass ASan/UBSan with ordinary linked dependencies and leak detection disabled on this
restricted host. Repeated exception injection in one ASan process cannot reliably
unpoison stack frames here; isolated budgets 0 through 10 each run in a fresh process,
while the ordinary native CTest exercises the complete in-process failure sweep.
C89/C++98 objects compile for ARM64 Linux, RISC-V64 Linux and Windows x64; this is
not a full foreign player build, target SDK/runtime or physical device acceptance.
## v0.763–v0.765 validation scope

108 selected native and 23 shared-host checks passed, including real CLI PCM/ADPCM,
offscreen embedding, resource/frame channels, host ABI and responsive final completion.
The new compressed core path also passed the 20 core stream/ADPCM suite and exact
independent FFmpeg checks. Edited native modules and fixtures passed ASan/UBSan;
allocation failures were isolated through budget 4 for preparation and 11 for the
multi-header frontend. Entire VM/SDL dependencies were not instrumented and process
leak enumeration is unavailable on this restricted host. A timing-sensitive
sanitizer tutorial run exceeded the explicit completion budget under concurrent
build load; the isolated ADPCM/FFmpeg/fast-deadline rerun passed. Budget exhaustion
remains an explicit playback failure, not a guarantee of success under arbitrary starvation.

Complete minimal cores and three selected ADPCM/C89 executables linked for ARM64,
RISC-V64 and x86-64 Linux/musl. The x86-64/musl tests actually ran on this host;
updated static ARM64 SDL/AGG player linked and passed ELF identity checks. Foreign
CPU execution, physical devices and full Flash/platform acceptance remain open.
The unchanged Windows speech comment-checker failures are recorded in the
[core preparation report](STREAM_ADPCM.md); that broader comment check is not claimed passing.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_navigation` | function | [include/flashvm/movieclip.h:93](../include/flashvm/movieclip.h#L93) |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `FVM_MOVIECLIP_NAVIGATION_MAX` | macro / define | [include/flashvm/movieclip.h:85](../include/flashvm/movieclip.h#L85) |
| `fvm_swf_stream_output_bind_frame` | function | [include/flashvm/swf_stream_output.h:96](../include/flashvm/swf_stream_output.h#L96) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:89](../include/flashvm/swf_stream_output.h#L89) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:83](../include/flashvm/swf_stream_output.h#L83) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:11](../include/flashvm/swf_stream_output.h#L11) |
<!-- source-links:end -->
