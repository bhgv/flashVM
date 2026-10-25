# Event voices added to native stream PCM (v0.771)

[Русский](STREAM_EVENT_MIX.ru.md) · [functional roadmap](ROADMAP.md) · [C89 example](../examples/multimedia/swf_stream_mix/README.md)

Since v0.772, the [native composition adapter](MIX_QUEUE.md) provides retained
original/mixed queue retry and acknowledged clear. The tutorial now exercises
both direct export and a real bounded FIFO; full desktop timeline integration
remains open.

`fvm_swf_sound_mixer_add(mixer, frames, pcm, capacity_values, written_values)`
adds prepared event voices **in place** to an initialized native-S16 base block.
The base can come from a prepared SWF stream converter, an external native source,
or explicit host-generated silence. Base and voices use the mixer's configured
sample rate/channel geometry. There is no automatic rate inference or timeline
scheduling. This is the first native composition checkpoint in the functional
completion section, not completion of desktop mixed-movie playback.

The base contributes to the same bounded wide accumulator as every prepared voice;
saturation happens once at the end of the mixing pass. For example base -30000
plus two +30000 voices yields +30000. Rendering the voices into clipped S16 first
and then adding the base would incorrectly yield +2767. With no active voices the
base is preserved. Existing [`fvm_swf_sound_mixer_read`](../include/flashvm/swf_sound_mixer.h#L157) keeps its zero-base behavior,
including silence after voice exhaustion; both entry points advance the same voice
clock and can be interleaved under the same serial owner. Conversion, loops,
envelopes and stop policies retain their existing voice/mixer contracts.

## C API contract

| Argument | Requirement |
| --- | --- |
| `mixer` | Live prepared native mixer; external serialization for every operation |
| `frames` | Requested interleaved frames, not scalar values |
| `pcm` | Caller-owned initialized, aligned S16 buffer at configured channel count; overwritten on success |
| `capacity_values` | Accessible scalar values, at least frames times channels |
| `written_values` | Nonnull, separate writable size_t; cleared before validation |

Success returns OK and writes exactly frames times channels values. Null mixer,
missing positive-length storage, inadequate capacity or arithmetic range returns
INVALID_ARGUMENT before touching PCM or advancing voices. Null written_values
returns INVALID_ARGUMENT. Zero frames permits null PCM and advances nothing.
Keep PCM disjoint from written_values and opaque mixer state. Zero initialization
is required only when the host deliberately wants a zero base; existing initialized
stream samples must not be discarded.

No allocation, callbacks, VM execution, managed pointers or device calls occur.
Retirement can free exhausted native voices. All source preparation/submission is
owner-side work before native publication. A safely retained mixer may be used
after the original context/runtime are destroyed. The caller owns PCM throughout
the call, and retains mixer/voice ownership until all serial use ends; ordinary
mixer_close releases it. There is no internal lock, worker or hidden queue.
The maximum 64 voices plus one S16 base fit the existing signed-long accumulation
bound on supported hosts.

## Queue retries, tails and cancellation

A successful add consumes voice time **once** even if a later host write fails.
Retain the already mixed bytes and retry those bytes; do not call add again on the
same PCM or rebuild the base from an advanced converter. Repeating add is a new
mixing operation, not an idempotent enqueue. This API cannot roll back external
writes or restore consumed voice time. A backend adapter must own its pending
mixed block until acceptance/cancellation and serialize event submissions around
that lifetime. File export treats partial/native errors as terminal rather than
pretending that fwrite is an atomic queue retry.

At stream EOF the host can supply initialized zeros while event voices finish;
it chooses the duration, authorization and output clock explicitly. A movie frame
gap is not permission to invent stream samples or advance all audio time. Stop,
seek, already queued audio and tails require a coordinated host policy. The desktop
player still rejects root stream+event movies pending that complete integration.
No MP3 decoder, partial-ADPCM support, child stream or A/V drift behavior is added.

## Actual SWF/file example

The [C89 tutorial](../examples/multimedia/swf_stream_mix/README.md) embeds one SWF8
with two DefineSound/StartSound resources and one root PCM stream. Both events are
explicitly started at its only frame. Owner preparation finishes, then the source
context/runtime are destroyed. The native stream converter and event mixer export
16 stereo frames at 48000 Hz in four-frame buffers, preserving the longer event
tail over explicit zero base. There is no SDL/device or live VM in the loop.
An independent Python absolute-rational oracle checks all output samples, clipping
order, tail/silence, replacement, CLI usage and native failure cleanup.

```sh
cmake --build build --target flashvm_tests_swf_sound_mixer_add \
  flashvm_tests_swf_sound_mixer_add_allocation flashvm_examples_multimedia_swf_stream_mix_main
ctest --test-dir build --output-on-failure -R 'swf_sound_mixer_add|swf_stream_mix'
python3 scripts/check_swf_stream_mix_example.py build/flashvm_examples_multimedia_swf_stream_mix_main
```

The API fixture covers four SWF rates, mono/stereo, strict/converted clocks,
513-frame calls crossing internal 256-frame passes, both clipping polarities,
invalid/overflow/zero requests and legacy read after exhaustion. The allocation
fixture prepares all 64 voices, destroys the VM and forbids further allocation
through additive mixing/retirement. C89 compile/link checks cover the new symbol.
These are bounded native contracts; full Flash and physical/platform acceptance
remain open.

## Revision validation

40 native sound/stream regression tests passed, followed by the strengthened
interleaved-read/add fixture and exact C89 tutorial oracle. Edited mixer and the
two new C++ fixtures/C89 example passed ASan/UBSan. The linked core/dependencies
were not fully instrumented; leak enumeration is disabled on this restricted host.
A strict C89 consumer linked and ran against the shared player-host library.

Four selected programs (additive fixture, no-allocation fixture, C89 ABI fixture
and annotated tutorial) compile/link for ARM64, RISC-V64 and x86-64 Linux/musl,
and Windows x64. All 12 Linux images have the expected ELF CPU/static-loader
identity; four Windows images pass PE32+ x64/direct-OS-import checks. The x86-64
musl fixtures and independent exact tutorial oracle actually ran on this x86-64
host. ARM64, RISC-V64 and Windows execution and physical audio remain untested.
The existing multimedia comment checker still reports unchanged Windows speech
source failures; the new example's every-line comments and API coverage pass.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mixer_add` | function | [include/flashvm/swf_sound_mixer.h:171](../include/flashvm/swf_sound_mixer.h#L171) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:157](../include/flashvm/swf_sound_mixer.h#L157) |
<!-- source-links:end -->
