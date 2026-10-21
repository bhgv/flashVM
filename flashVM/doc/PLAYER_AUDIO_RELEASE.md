# Optional player stop release — v0.587

[Русский](PLAYER_AUDIO_RELEASE.ru.md).
The v0.586 phone WAVs contain stop-by-resource transitions from about 6026 to
zero. Ordinary stereo and ADPCM tails end around 10 and 3 respectively. The
player therefore opts into short stop releases instead of filtering all PCM.
This is a bounded host policy, not a claim of sample-exact Flash stop semantics
or confirmed click-free physical playback. Original immediate APIs stay intact.

## Stop-by-resource voices

[`FVM_SWF_SOUND_MIXER_DECLICK`](../include/flashvm/swf_sound_mixer.h#L23) has value 2 (0x2) in `FVM_SWFSoundMixerConfig.flags`,
combinable with [`FVM_SWF_SOUND_MIXER_CONVERT`](../include/flashvm/swf_sound_mixer.h#L21). Zero/CONVERT alone retain the
old stop behavior. Unknown bits still return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) on create;
the public config and geometry structs have not changed size.

With DECLICK, a successful stop-by-ID submission marks matching voices for
floor(output_rate/200) output frames (about five milliseconds, minimum two).
The source continues with a linear gain from one to exactly zero, then the slot
is retired. Conversion and direct reads apply the same output-clock envelope,
including reads split across buffers. Repeated stops do not restart a release.
This policy changes no natural source endings, loops, source snapshots or other
voices. It adds no allocation, callbacks, managed references or device access.
An exhausted source can finish earlier; no artificial source extension is promised.

Releasing voices count toward active_voices and capacity. They do not suppress
a later SyncNoMultiple start, but a full slot table still returns
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), without stealing a voice. The original
[`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L131) remains immediate, including with DECLICK enabled.
Strict geometry/rate validation is unchanged without CONVERT. All operations
still require host serialization; submission uses the source context's owning
thread. The player selects both flags; applications must opt in explicitly.

## fvm_swf_sound_output_set_release

`fvm_swf_sound_output_set_release(output, frames)` configures optional native
accepted-PCM history before first submission in the current output epoch.
frames=0 disables and frees the ring; nonzero must be 2..the configured pump block.
Null/busy output, a submitted epoch or an invalid frame count returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). The function enters no backend callback and changes no
mixer or device state. The caller must preserve exclusive producer ownership.

Success [`FVM_OK`](../include/flashvm/flashvm.h#L42) copies the count and owns a ring of (target+block)*channels
shorts. Failure [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) preserves the previous setting/history;
unexpected native exceptions become [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Replacement is transactional.
Maximum geometry is bounded by existing block/queue/channel limits. Only
successful queue writes enter history, so retry does not duplicate samples.
Normal output with frames=0 retains the old allocation and copying behavior.
The player configures 240 frames at 48000 Hz, before starting its worker.

## fvm_swf_sound_output_stop_release

`fvm_swf_sound_output_stop_release(output)` requires successful nonzero release
configuration. Null/busy/unconfigured output returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).
All backend callbacks share the ordinary reentry guard. It allocates nothing,
retains no VM pointers and must be serialized with mixer/output operations.
It can run after source VM destruction because the native snapshots/history are
independent; do not close its mixer/device during the call.

The function observes software queue depth and identifies the last accepted
sample preceding that queue. It clears the backend and stops all software voices,
then resets the software clock epoch. If running with an observed nonzero
boundary, it submits a linear release from that level to exactly zero. The
backend is not paused/resumed by this operation, avoiding an inserted paused
silent block. A paused output remains paused and synthesizes no tail. No consumed
sample or a zero boundary also requires no tail. Success returns [`FVM_OK`](../include/flashvm/flashvm.h#L42).
The source voices disappear immediately; only the bounded release PCM may remain.

Observe/clear errors preserve pending source/PCM ownership and return the callback
status (exceptions map to [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) or [`FVM_ERROR`](../include/flashvm/flashvm.h#L43)). After a successful
clear the old sound is gone even if the subsequent release write fails. That
exact release remains pending for pump retry; counts advance only on acceptance.
Some backends, including the fault-latching SDL adapter, require the original
hard stop/reset before recovering from a write failure. That discards the tail.
The original [`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L114) still pauses, clears and resets immediately,
never synthesizing a release; it remains the recovery/terminal cleanup operation.

## Queue boundary and hardware limits

The selected boundary is a software dequeue observation, not the DAC position.
Already delivered device buffers cannot be rewritten. A consumer can advance
between observation, clear and write: the generic backend ABI provides no atomic
replacement transaction. Hardware buffering/conversion and races therefore limit
physical continuity; new phone listening is required. The short release adds at
most its configured PCM duration after the already delivered buffers. This is an
explicit presentation policy, not a new guarantee about AVM1/audio scheduling.

Diagnostic WAVs concatenate accepted blocks, including ones later cleared. A
release after clear can therefore look discontinuous relative to the preceding
WAV block even though that preceding block was discarded from playback. Interpret
clear/write markers and the chosen input boundary; do not treat the concatenation
as a hardware recording. Capture format, queue reserve and source tones are unchanged.

## Verification and tutorials

The extended [C89 queue tutorial](../examples/multimedia/swf_audio_queue/README.md)
demonstrates optional configuration, release stop and the original hard stop on
an explicit silence-only model. It is ownership/control evidence, not an audible
fade example. The real [player tests](../examples/player/animation_audio/README.md)
exercise the five-millisecond policy with the unchanged SWFs in both VM modes.
Regressions cover exact direct/converted endpoints, split reads, no-multiple,
legacy immediate behavior, ring wrap, clear/write failures, reentry, allocation
failure and allocation-free release. Native AudioTrack and the v0.585 signer are
retained. Phone acceptance of the new endings remains pending.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../include/flashvm/swf_sound_mixer.h#L21) |
| `FVM_SWF_SOUND_MIXER_DECLICK` | macro / define | [include/flashvm/swf_sound_mixer.h:23](../include/flashvm/swf_sound_mixer.h#L23) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:131](../include/flashvm/swf_sound_mixer.h#L131) |
| `fvm_swf_sound_output_set_release` | function | [include/flashvm/swf_sound_output.h:116](../include/flashvm/swf_sound_output.h#L116) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:114](../include/flashvm/swf_sound_output.h#L114) |
| `fvm_swf_sound_output_stop_release` | function | [include/flashvm/swf_sound_output.h:122](../include/flashvm/swf_sound_output.h#L122) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:28](../include/flashvm/swf_sound_mixer.h#L28) |
<!-- source-links:end -->
