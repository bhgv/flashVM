# Native stream queue recovery (v0.769)

[Русский](PLAYER_STREAM_RECOVERY.ru.md) · [PCM/ADPCM playback](PLAYER_PCM_STREAMS.md) · [player example](../examples/player/pcm_stream/README.md)

The desktop native stream worker now detects an **observed empty playing FIFO
with authorized source work remaining**, pauses the backend, primes its existing
reserve and resumes it. This uses the same independent immutable PCM sources,
frame gates and bounded eight-pump worker pass as ordinary playback. It does not
create silence, decode on the owner thread, change movie speed or stretch samples.
No new command-line option or public host ABI is required.

The observation requires all of: acknowledged unpaused state, zero queued frames,
positive accepted-frame history in the current seek epoch, and current or successor
block metadata at or before the owner's authorized frame. Empty successor headers
are skipped within the existing maximum source count. This metadata query never
advances a converter or authorizes future frames. Initial preparation, final drain,
future-frame waits, MovieClip stop, StopSounds and host suspension do not start an
observed starvation episode. This detects worker observations, not every device
underrun: brief gaps between observations may be missed, and authorized zero-length
markers can count as source work. It is not a DAC/hardware starvation measurement.

A remembered frame-gate/EOF wait also suppresses a new edge on the first later
authorization; delayed movie authorization is not classified as a producer failure.
An explicit host pause or navigation starts this waiting state as well.

A failed backend pause returns its error before counting or pumping. Acknowledged
pause starts one episode and increments the existing `audio_underruns` statistic;
stream increments saturate at native INT_MAX. Failed writes retain exact retry
bytes and the open episode, so retries do not count again. Existing worker failure
latching still applies: normal playback reports a backend error rather than hiding
it by automatically retrying errors. On successful priming and acknowledged resume,
the episode closes. Short authorized tails resume below the usual reserve when a
frame gate or EOF prevents further priming. Later empty observations can start a
new episode. Navigation resets, native stop and explicit host pause cancel the edge
state without rewinding the cumulative count.

With the existing explicit `FVM_AUDIO_TRACE` opt-in, bounded RAM evidence includes
`stream_starved` and `stream_recovered` operations. The recovery value is the
observed queued-frame count; timestamps are worker observation times, not elapsed
hardware underrun durations. File output occurs after worker join. Without capture,
these calls allocate nothing and perform no file I/O. Owner/render code continues
to poll only the error latch and publish native scalar policy under the audio mutex.

The same metadata helper is used by finite-run completion, preserving its read-only
behavior and future-frame exclusion. Run diagnostics remain cumulative over root
clearing as before. The event-sound producer retains its existing starvation policy.
The internal frontend types/functions are not a new standalone public C ABI.

## Checks and limitations

`frontend/tests/stream_refill.cpp` supplies deterministic queue consumption to the
actual C89 worker implementation through a test-only translation unit. It checks
initial priming, acknowledged pause before writes, failed pause without progress,
failed-write retry without duplicate edges, recovery, a second edge, future-frame
idle, count saturation, movie stop/cancel and final EOF. The source VM is destroyed
before native production. No timing sleeps or physical device are needed.

Build/run alongside the real SDL dummy-worker integration:

```sh
cmake --build build-player --target flashvm_frontend_stream_refill flashvm_frontend_stream_worker
ctest --test-dir build-player --output-on-failure -R 'flashvm_frontend_stream_(refill|worker|pending)'
```

This completes a bounded queue-recovery behavior, not mixed stream/event playback,
MP3 decoding, audio-driven movie scheduling, drift correction or platform/device
acceptance. All nine roadmap stages remain open.

## Implementation / Реализация

| Function / Функция | Source / Исходник |
| --- | --- |
| `stream_authorized` | [frontend/common/audio.c:168](../frontend/common/audio.c#L168) |
| `refill_stream` | [frontend/common/audio.c:198](../frontend/common/audio.c#L198) |
| `fvm_desktop_audio_stream_pending` | [frontend/common/audio.c:625](../frontend/common/audio.c#L625) |

## Revision validation

The final native selection passed 113 tests; the shared-host selection passed 26.
Both include the deterministic refill and production-trace checks, plus actual
SDL dummy-worker, finite completion, memory-budget and CLI stream scenarios.
Edited native/owner modules and seven focused fixtures passed ASan/UBSan. The
350 ms event-worker timing assertion failed during concurrent work and passed
when rerun alone without code changes; it is not a device timing acceptance.
Allocation failures were isolated through budget 11. Leak enumeration is disabled
on this restricted host, and linked VM/SDL dependencies are not fully instrumented.
The complete ARM64 Linux/musl player links with target-built static SDL/AGG; its
ELF CPU identity was checked. ARM64 execution and physical devices remain untested.

Instrumented actual CLI checks also passed exact FWS/CWS PCM and format changes,
all supported whole-packet ADPCM code widths/channels in interpreter/native JIT,
finite completion and the bundled tutorial against an independent FFmpeg oracle.
