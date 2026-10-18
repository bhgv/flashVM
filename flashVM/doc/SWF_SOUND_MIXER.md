# Incremental StartSound mixer

## Purpose and ownership

The C89 API in [swf_sound_mixer.h](../include/flashvm/swf_sound_mixer.h) combines
independent [`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L9) snapshots into one native signed-16 stream. Unlike
[`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L97), it keeps voices playing across successive reads and
mixes overlapping voices into the same buffer. Events are delivered explicitly by
the host. Neither MovieClip frame progression nor an OS audio callback invokes it
automatically. The [tutorial](../examples/multimedia/swf_event_mixer/README.md)
shows actual controls and optional file output.

[GC:] The mixer owns only native slot/PCM/envelope storage, with no context pointer,
managed object or borrowed source bytes. Existing voices survive source reload,
GC, runtime and context destruction. Submission needs a live loaded context on
its owning thread. All operations on one mixer require external serialization;
it is neither a thread-safe queue nor a reentrant callback service. No function
calls host output, opens a device or starts a thread. Never close a mixer while
another thread is reading it.

## Geometry and identity

This section describes legacy strict creation. Explicit configured conversion is
available separately: [rate/channel conversion](SWF_SOUND_CONVERSION.md). The old
creation function and info struct retain their ABI and strict behavior.

One mixer has a fixed nominal rate (5512, 11025, 22050 or 44100 Hz), one or two
interleaved channels and 1..[`FVM_SWF_SOUND_MIXER_MAX_VOICES`](../include/flashvm/swf_sound_mixer.h#L9) simultaneous voices
(currently 64). Source PCM8/PCM16/ADPCM is converted by the existing decoder and
styled voice preparation. Other codecs retain their existing UNSUPPORTED/error
behavior. Source rate and channel count must exactly match: there is no implicit
resampling, mono/stereo conversion, master gain or crossfade.

Sound IDs are mixer-wide. Use one mixer per SWF dictionary identity domain. If a
host deliberately submits another movie with the same sound ID, stop/no-multiple
will address the old and new voices together. A mixer contains no source context
identity. Context audio channels and other mixers form separate control domains.
AVM1 StopSounds does not implicitly clear this independent mixer; the host must
call [`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L61) and separately flush any device queue.

## API contracts

### fvm_swf_sound_mixer_create

`fvm_swf_sound_mixer_create(sample_rate, channels, voice_capacity, out)` allocates
a mixer and its fixed slot table; `*out` is cleared before validation. A null out,
unsupported nominal rate, channels outside 1..2 or capacity outside 1..64 returns
INVALID_ARGUMENT. Incompatible host byte/short widths return UNSUPPORTED;
allocation failure returns OUT_OF_MEMORY with no published handle. On OK the
caller owns the handle until close. Creation needs no VM or device. The fixed
mixing scratch is bounded independently of requested output block length.

### fvm_swf_sound_mixer_submit

`fvm_swf_sound_mixer_submit(mixer, context, event_index)` resolves one zero-based
root StartSound using [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L58). Null/terminal/unloaded contexts,
missing events, malformed resources and unsupported decoders propagate the source
API statuses. Missing ordinal is NOT_FOUND. Existing voices and their progress
remain unchanged on failure.

Controls execute in host submission order:

1. A STOP event removes **all** current voices of that sound ID, even if the mixer
   is full or the stop source has incompatible output geometry. It wins when
   both synchronization flags are present. No matching voice still succeeds.
2. NO_MULTIPLE with a matching active voice succeeds without preparing another
   snapshot. Ordinary starts permit multiple copies of the same resource.
3. A new voice in strict mode requires equal source rate/channels (otherwise UNSUPPORTED).
   Preparation applies range, repetitions and envelope using
   [`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L78). Each copy has independent playback/envelope state.
4. An empty interval or explicit zero loop count consumes no slot. Otherwise a
   full mixer returns OUT_OF_MEMORY without voice stealing or replacement.

Preparation can allocate/decode and fail, including when discovering an empty
voice or a full slot table; do it outside real-time device callbacks. Each voice
uses the originating runtime's existing **per-snapshot** memory limit. Capacity
bounds polyphony, not a shared aggregate PCM byte budget: total stored decoded
source intervals can reach capacity times the per-voice limit, plus native
slot/envelope/scratch overhead. The host must choose both limits appropriately.
Loops do not duplicate stored sample data. No clock advances during submit.

### fvm_swf_sound_mixer_read

`[fvm_swf_sound_mixer_read](../include/flashvm/swf_sound_mixer.h#L55)(mixer, frames, destination, capacity_values,
written_values)` fills exactly the requested sample frames. Capacity and written
count **interleaved short values**, so stereo needs `frames * 2`. Written must be
nonnull; it is zeroed before validation. Destination must be nonnull for nonzero
frames. Capacity is checked using division before multiplication, including size
overflow cases. Bad arguments return INVALID_ARGUMENT without changing voices or
destination. Zero frames succeeds without advancing; destination may then be null.
The count output must not alias destination storage.

On OK in strict mode, every active snapshot advances by up to `frames`.
Configured conversion uses the separate source/output clocks and lookahead rules
in [SWF_SOUND_CONVERSION.md](SWF_SOUND_CONVERSION.md). A short/exhausted voice
contributes zeros; no voices also produces full silence and returns OK, never
NOT_FOUND. Exhausted snapshots are released and their slots reused. Per-voice
styled samples are added as signed longs in bounded passes, then the complete
sum is saturated **once** to -32768..32767. For example, 30000 + 30000 - 30000
produces 30000 regardless of insertion order. The 64-voice bound keeps the sum
inside a 32-bit signed long. No allocation, source access or host callback occurs;
finishing voices can deallocate native memory, so this API does not promise a
hard real-time allocator-free callback deadline. Use a host producer/device queue
when that distinction matters. Very large reads still take proportional CPU time.

### fvm_swf_sound_mixer_info

`fvm_swf_sound_mixer_info(mixer, out)` copies [`FVM_SWFSoundMixerInfo`](../include/flashvm/swf_sound_mixer.h#L13) on OK:
`sample_rate`, `channels`, `voice_capacity`, `active_voices`. Set `struct_size`
to exactly sizeof the struct. Null pointers or wrong size return INVALID_ARGUMENT
and preserve output. No allocation, callback or VM access occurs. Active count
excludes empty or fully rendered output voices; converted pending lookahead still
counts even if the underlying decoder reached EOF. It says nothing about samples already
queued to hardware.

### fvm_swf_sound_mixer_stop_all

`fvm_swf_sound_mixer_stop_all(mixer)` discards all software voices and releases
their snapshots. Null returns INVALID_ARGUMENT; a valid empty mixer returns OK.
It is allocation-free and does not stop/flush any external OS/file output. Info
then reports zero active voices; subsequent reads produce silence until another
accepted start. Valid geometry/capacity are retained for reuse.

### fvm_swf_sound_mixer_close

`fvm_swf_sound_mixer_close(mixer)` releases all snapshots and the mixer. Null is a
no-op; a nonnull handle becomes invalid immediately. It returns void, needs no
live source context and enters no callbacks. External device ownership remains
with the host; close the device separately in the order required by its bridge.

## Host scheduling

The [root callback](MOVIECLIP_SOUND.md) delivers actually executed root entries;
blindly submitting every root event on every display tick is incorrect. The host
submits accepted events in order and requests sample frames from its chosen output
clock. Re-submitting an ordinary start creates another voice; pausing reads pauses
the software audio clock. [Configured conversion](SWF_SOUND_CONVERSION.md) handles
explicit rate/channel differences. Sprite sound, stream codecs, band-limited
filtering and automatic device/A/V clock synchronization remain separate work.
Device writes use the host bridge without rollback of a consumed software buffer
after output failure.

## Validation

Regression tests cover mono/stereo and all nominal rates, all three-voice order
permutations, one-pass clipping, 64 voices, slot exhaustion/reuse, stop precedence,
no-multiple, loops, silence tails, invalid/overflowing capacity and unchanged
output/state on errors. ADPCM widths 2..5 across 4096-frame packet boundaries,
loop/envelope progression and chunk sizes 1/7/255/256/257/8192 are compared with
existing complete event rendering. Allocation-failure injection checks create
and submit cleanup plus allocation-free read/info/stop/close; reload, GC and VM
teardown verify native ownership. A strict C89 client exercises every function.
The actual annotated example is tested with exact native PCM file bytes, invalid
CLI and file-open failure. These are software checks, not audible device or
foreign-platform acceptance, and do not close roadmap stages 4, 8 or 9.
## Actual root timeline integration (v0.579)

The [MovieClip sound callback](MOVIECLIP_SOUND.md) can now submit root events at
real frame entry, without replaying skipped frames during seek reconstruction.
The mixer still has no implicit device clock or automatic AVM1 StopSounds binding.

## v0.587 optional stop release

[Player policy, new APIs and limitations](PLAYER_AUDIO_RELEASE.md). Original immediate stop behavior remains the default.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:58](../include/flashvm/swf_sound.h#L58) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:63](../include/flashvm/swf_sound_mixer.h#L63) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:45](../include/flashvm/swf_sound_mixer.h#L45) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:59](../include/flashvm/swf_sound_mixer.h#L59) |
| `FVM_SWF_SOUND_MIXER_MAX_VOICES` | macro / define | [include/flashvm/swf_sound_mixer.h:9](../include/flashvm/swf_sound_mixer.h#L9) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:55](../include/flashvm/swf_sound_mixer.h#L55) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:61](../include/flashvm/swf_sound_mixer.h#L61) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../include/flashvm/swf_sound_mixer.h#L50) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:78](../include/flashvm/swf_sound.h#L78) |
| `FVM_SWFSoundMixerInfo` | struct | [include/flashvm/swf_sound_mixer.h:13](../include/flashvm/swf_sound_mixer.h#L13) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
<!-- source-links:end -->
