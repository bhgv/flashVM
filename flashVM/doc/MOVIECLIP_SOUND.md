# MovieClip root sound delivery

## Supported path

MovieClip imports root DefineSound (14) and StartSound (15). Register
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21) before advancing a sound frame. Its
[`FVM_MovieClipSoundCallback`](../include/flashvm/movieclip.h#L18) receives `(context, clip, event_index, userdata)` at
actual root frame entry. The zero-based index addresses the same loaded SWF's
[`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L62) and [`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L66). The host chooses its
mixer, output geometry and device queue. No hidden audio device/thread is created.
This extends [incremental mixing](SWF_SOUND_MIXER.md), not automatic device-clock
synchronization. [Runnable C89 example](../examples/multimedia/swf_event_mixer/README.md).

## Loading and source identity

For movies containing these tags, first call [`fvm_load_memory`](../include/flashvm/flashvm.h#L72) with the same SWF,
then [`fvm_movieclip_load_swf`](../include/flashvm/movieclip.h#L102). FWS and equivalent CWS share canonical FWS bytes.
A mismatch or absent context source returns INVALID_ARGUMENT with null clip output.
The sound metadata parser validates definitions and all selected root events,
including resource resolution and SOUNDINFO syntax; importing does not decode or
play audio. Unsupported sound codecs can therefore be imported but a host mixer
can reject their later execution. Dictionary IDs are shared with visual symbols;
a sound cannot be placed on the display list or redefine another character.
Sprite sound controls/definitions and SoundStreamHead/Block remain UNSUPPORTED.
Movies without sound retain the old independent importer behavior.

[GC:] The imported frame graph retains ordered scalar ordinals and a complete
canonical source snapshot encoded as ASCII hex, like transport-safe action data.
It contains no native callback/mixer pointer. Source comparison occurs at each
sound-bearing frame entry, before delivering any of its events, and takes time
linear in canonical file length. It compares the actual bytes, not a hash or an
address/generation stamp that could survive incorrectly through graph transport.
Hex uses twice the source byte length plus managed/container overhead; import also
has transient decoded/source-string storage. Existing decode/graph limits apply;
this is not a new aggregate memory-budget guarantee. A future prepared source
index may reduce this cost without weakening source checks.

Reloading unrelated bytes leaves old visual graphs alive but their sound delivery
returns INVALID_ARGUMENT. Loading the original canonical file again permits a
new explicit frame entry to execute. The callback cannot reload the source during
delivery. Hosts must pin/root the clip until their operations finish.

## Callback registration contract

`fvm_movieclip_set_sound_callback(context, callback, userdata)` borrows callback
and userdata until replacement or context destruction; it allocates nothing and
never owns/frees userdata. Null callback unregisters and ignores userdata.
A valid owning-thread context returns OK. Null, terminal GC/destroy state or
registration during a sound callback returns INVALID_ARGUMENT without replacing
the previous registration. This is a per-context setting, independent of every
clip graph and audio channel.

The callback runs synchronously on the VM thread, under the existing native-call
GC barrier. It can query metadata and submit to a host-owned mixer. It must not
mutate the clip/frame graph, execute script or destroy runtime/host resources in
use. Context destruction is already refused during the native barrier. Recursive
[`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L125), callback replacement and [`fvm_load_memory`](../include/flashvm/flashvm.h#L72) are rejected
with INVALID_ARGUMENT during sound delivery. Host userdata must outlive the call.
A C++ bad_alloc becomes OUT_OF_MEMORY; other escaped exceptions become ERROR;
returned VM statuses are propagated unchanged. Busy state resets on every exit.

## Execution order and errors

Actual frame entry applies the visual scene, marks the entry consumed, delivers
its StartSounds in source order, then executes that frame's DoAction blocks.
Normal child advance/onEnterFrame behavior follows the existing MovieClip path.
Visual reconstruction for goto/loop never executes sounds of skipped frames.
A stopped already-entered frame does not repeat its sounds on later host ticks.
An explicit seek back to the same frame creates another entry; a natural loop
also executes its destination frame's sounds again. Sound flags remain the mixer's
responsibility; the callback is a notification, not an implicit voice owner.
Interpreter and JIT frame actions share this host dispatch path.

A sound entry without a registered callback returns UNSUPPORTED; sound is not
silently discarded. Any source mismatch, callback error or OOM aborts that advance
before its DoAction blocks. Visual changes and successful earlier host callbacks
are not rolled back. The entry is already consumed: automatic retry could duplicate
voices or external I/O. Abort playback or explicitly seek/retry with host recovery;
registration alone does not replay a failed entry. This API provides no hardware
queue rollback, frame skipping, resampling or hard real-time deadline guarantee.
AVM1 StopSounds still affects registered context audio channels; clearing an
independent software mixer/device queue requires an appropriate host stop bridge.

## Graph transport and checks

Frame/source data serialize as ordinary managed graph edges; callback code/userdata
never serialize. A receiver loads the matching canonical SWF and installs its own
callback/mixer. The original source context can then be destroyed. Regression tests
cover this path through GC, root pinning and an explicit retry after missing callback.
Other tests cover Stop, goto, same-frame re-entry, natural loops, ordered controls,
source replacement, exception/reentry guards, import OOM, duplicate IDs, invalid
flags, attempted sound placement, unsupported sprite controls, equivalent FWS/CWS,
and actual interpreter/native JIT entries. The C89 tutorial tests both manual and
timeline-driven controls against exact native PCM bytes. These software checks do
not constitute audible device or Windows/macOS/Android acceptance.
Sound ExportAssets/linkage is explicitly UNSUPPORTED until Sound.attachSound
support exists; it is never installed as an undefined visual symbol.
Звуковой ExportAssets/linkage пока явно UNSUPPORTED: Sound.attachSound ещё
не реализован этим изменением. Проверяется отдельным malformed/unsupported fixture.

[Ordinal snapshots](SOUND_SNAPSHOT.md) — v0.637.

Canonical validation of every sound-bound entry, including empty frames: [v0.640](SOUND_SNAPSHOT.md#canonical-frame-binding-v0640).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:125](../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:102](../include/flashvm/movieclip.h#L102) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `FVM_MovieClipSoundCallback` | callback typedef | [include/flashvm/movieclip.h:18](../include/flashvm/movieclip.h#L18) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:62](../include/flashvm/swf_sound.h#L62) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:66](../include/flashvm/swf_sound_mixer.h#L66) |
<!-- source-links:end -->
