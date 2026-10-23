# Explicit output-rate and channel conversion

## Scope

The configured mixer can combine SWF PCM/ADPCM voices with different source
rates/channels into one native signed-16 output clock, such as 48000 Hz stereo.
It extends [incremental StartSound mixing](SWF_SOUND_MIXER.md) and accepts events
from the [actual root timeline callback](MOVIECLIP_SOUND.md).

Conversion is opt-in. The existing [`fvm_swf_sound_mixer_create`](../include/flashvm/swf_sound_mixer.h#L71) stays strict:
non-SWF output rates and mismatched voice geometry retain their previous errors.
Its [`FVM_SWFSoundMixerInfo`](../include/flashvm/swf_sound_mixer.h#L13) ABI is unchanged. Existing same-geometry mixing uses
the previous direct path, including when conversion is enabled but unnecessary.
No device, thread, hidden queue, compressed decoder or audio/video clock is added.

## Configuration ABI

[`FVM_SWFSoundMixerConfig`](../include/flashvm/swf_sound_mixer.h#L28) is a separate C89 struct. Initialize every field:

| Field | Contract |
| --- | --- |
| struct_size | Exactly sizeof([FVM_SWFSoundMixerConfig](../include/flashvm/swf_sound_mixer.h#L28)). |
| sample_rate | Output sample frames per second. Strict mode accepts 5512/11025/22050/44100. Convert mode accepts every integer from [FVM_SWF_SOUND_MIXER_MIN_RATE](../include/flashvm/swf_sound_mixer.h#L25) (5512) through [FVM_SWF_SOUND_MIXER_MAX_RATE](../include/flashvm/swf_sound_mixer.h#L26) (192000), inclusive. |
| channels | Output mono (1) or interleaved stereo (2). |
| voice_capacity | Concurrent voices, 1 through [FVM_SWF_SOUND_MIXER_MAX_VOICES](../include/flashvm/swf_sound_mixer.h#L9) (64). |
| flags | Zero for strict geometry; [FVM_SWF_SOUND_MIXER_CONVERT](../include/flashvm/swf_sound_mixer.h#L21) explicitly enables linear rate/channel conversion. [`FVM_SWF_SOUND_MIXER_DECLICK`](../include/flashvm/swf_sound_mixer.h#L23) additionally enables stop release; other bits are invalid. |

The SWF nominal 5.5 kHz convention remains the existing integer 5512 Hz, not a new
fractional 5512.5 Hz representation. The host must choose a supported device
format separately and pass the resulting rate/channels here.

### fvm_swf_sound_mixer_create_configured

`fvm_swf_sound_mixer_create_configured(config, out)` copies the complete policy
and allocates the mixer plus fixed-capacity slots/scratch. `out` must be nonnull;
`*out` is cleared before all other validation. Null config, wrong struct size,
unknown flags, invalid rate/channels/capacity return INVALID_ARGUMENT. Incompatible
host byte/short widths give UNSUPPORTED. Allocation failure gives OUT_OF_MEMORY;
no partial handle is published. On OK the host owns the mixer until close.
The input config is borrowed only during this call; later changes have no effect.
Creation needs no source VM, callback, thread or device. Fixed per-slot lookahead
adds bounded storage; samples are not expanded to the output rate in memory.

### fvm_swf_sound_mixer_get_config

`fvm_swf_sound_mixer_get_config(mixer, out)` copies the immutable creation policy.
Set `out->struct_size` to exactly sizeof the config struct. Null mixer/out or wrong
size returns INVALID_ARGUMENT and preserves output. On OK, sample_rate/channels
are output geometry and flags reports the selected conversion policy. No VM
access, allocation, callback or voice advancement occurs. This query does not
report an actual hardware format or device availability. Legacy creation reports
flags=0 through this query.

## Sampling policy

A converted voice is sampled at source position `n * source_rate / output_rate`,
where n is its zero-based output frame index since submission. An integer phase
numerator/remainder tracks that position across arbitrary caller/internal blocks;
there is no cumulative floating-point clock. The phase sum is bounded by output
rate plus a SWF source rate, safely inside 32-bit unsigned long.

The pipeline is deliberately defined:

1. Existing voice preparation/reads decode on the original source clock and apply
   InPoint/OutPoint, repetitions and source-time envelopes.
2. Channel mapping: mono→stereo duplicates the sample; stereo→mono averages left
   and right, truncating toward zero. For example, (-32768 + 32767)/2 gives 0.
3. Linear interpolation between adjacent **mapped, styled** source frames uses
   the exact integer phase fraction. The result of each voice is truncated toward
   zero before adding to the wide mixer sum. Products are evaluated as double to
   avoid non-C++98 long-long extensions; their small integer numerators fit exactly
   in double. The final complete voice sum saturates once to -32768..32767.

At the end, hold the final source sample until its source interval expires; then
contribute silence. N total source frames, including repetitions, produce
`ceil(N * output_rate / source_rate)` output frames. The phase does not restart at
loop boundaries: interpolation can cross the loop seam, and repeated rounding of
each individual play does not stretch the sound. This is an explicit host policy,
not a claim of original Flash Player resampler equivalence. Linear interpolation
has **no anti-alias low-pass filter**; downsampling can alias, and high-fidelity
band-limited conversion remains separate work.

For source mono 44100 Hz samples -32768, 32512, the 48000 Hz stereo result is:

| Output frame | Left | Right |
| --- | ---: | ---: |
| 0 | -32768 | -32768 |
| 1 | 27208 | 27208 |
| 2 | 32512 | 32512 |
| 3 and later | 0 | 0 |

## State, errors and ownership

Submission still prepares a native snapshot before publication; failed decoding,
unsupported codecs, OOM or full capacity preserve existing voices. Strict mode
still rejects mismatched geometry. Converted sources retain their original PCM
rate/channel count and memory budget; resampling is incremental, not a second
full decoded copy. Reading allocates nothing and retains no VM pointers.
Retiring finished voices can free native memory; this is not a hard real-time
allocator/deadline guarantee. All mixer operations require external serialization.

One current and one next mapped frame are buffered. Lookahead may have consumed
the last decoder sample while converted output remains audible. Such a voice
still counts in active_voices and suppresses SyncNoMultiple. STOP and stop_all
immediately discard both the snapshot and buffered output; they do not flush a
hardware queue. Reused slots restart with zero phase and no stale buffered frames.
Zero reads or invalid capacity never initialize/advance conversion. Nonzero reads
still fill the complete requested output size, padding exhausted tails with
silence; counts are output **channel values**, not source frames. Existing voices
continue after source reload/GC/context/runtime destruction.

## Example and verification

The annotated [C89 tutorial](../examples/multimedia/swf_event_mixer/README.md)
adds `--convert-demo [output-native-s16.pcm]`. It registers the real root callback,
uses configured 48000 Hz stereo and optionally writes exactly the samples above
through fopen/fwrite/fclose. No output path means no file/device I/O. NO_MEDIA
works without an SDK. Existing --demo/--timeline-demo remain strict mono modes.

Tests compare absolute-time independent oracles with six read block sizes,
all nominal source rates, eleven output clocks, mono/stereo combinations,
long fractional-ratio runs, 64 voices, all voice permutations, loop seams,
negative odd channel averages, held EOF and slot reuse. ADPCM widths 2..5 across
4096-frame packet boundaries, styled source envelopes/repetitions, strict C89,
OOM and VM teardown are covered. Real interpreter/JIT root actions prepare a
converted mixer and check native/interpreter counters; the executable tutorial's
output file is compared byte-for-byte. These are software tests; native device,
foreign-platform and original-player conformance acceptance remain open.

## v0.587 optional stop release

[Player policy, new APIs and limitations](PLAYER_AUDIO_RELEASE.md). Original immediate stop behavior remains the default.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../include/flashvm/swf_sound_mixer.h#L21) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:71](../include/flashvm/swf_sound_mixer.h#L71) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:58](../include/flashvm/swf_sound_mixer.h#L58) |
| `FVM_SWF_SOUND_MIXER_DECLICK` | macro / define | [include/flashvm/swf_sound_mixer.h:23](../include/flashvm/swf_sound_mixer.h#L23) |
| `fvm_swf_sound_mixer_get_config` | function | [include/flashvm/swf_sound_mixer.h:68](../include/flashvm/swf_sound_mixer.h#L68) |
| `FVM_SWF_SOUND_MIXER_MAX_RATE` | macro / define | [include/flashvm/swf_sound_mixer.h:26](../include/flashvm/swf_sound_mixer.h#L26) |
| `FVM_SWF_SOUND_MIXER_MAX_VOICES` | macro / define | [include/flashvm/swf_sound_mixer.h:9](../include/flashvm/swf_sound_mixer.h#L9) |
| `FVM_SWF_SOUND_MIXER_MIN_RATE` | macro / define | [include/flashvm/swf_sound_mixer.h:25](../include/flashvm/swf_sound_mixer.h#L25) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:28](../include/flashvm/swf_sound_mixer.h#L28) |
| `FVM_SWFSoundMixerInfo` | struct | [include/flashvm/swf_sound_mixer.h:13](../include/flashvm/swf_sound_mixer.h#L13) |
<!-- source-links:end -->
