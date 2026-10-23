# Player audio evidence — v0.584

[Русский](PLAYER_AUDIO_DIAGNOSTICS.ru.md).
This is a diagnostic build for the reported Android knocks/crackling. It does
not claim a fix. The microphone recording contains short broadband impulses;
the expected test sounds are approximately 250 ms tones. Linux SDL disk output
for v0.583 has matching interpreter/JIT peaks and expected frequencies/durations;
that does not establish Android PCM or native output correctness.

## Android workflow

1. Install the v0.584 APK over the preceding test build (same disposable signing key).
2. Clear Mute and select **Диагностика звука: 15 секунд, сохранить WAV и отчёт**.
3. Run 01 Interpreter and wait for the automatic return to the menu. Run 01 JIT.
4. Run 03 Interpreter and 03 JIT in the same way. 02 is optional: its authored
   audio is identical to 01, while its input logic differs.
5. Tap **Отправить диагностику звука (ZIP)**. Save/share audio-diagnostics.zip
   and attach it to the issue/conversation. The ZIP contains the completed runs.

Let each run finish, keeping the player in the foreground. The bound is 360
movie frames (15 seconds at authored 24 fps); slow rendering or background
suspension can make wall time longer. Back normally finalizes a shorter trace.
Force-stopping/crashing the process can lose the RAM capture. Repeating a selected
movie/mode replaces only that run. Other movie/mode traces remain. When comparing
loudness, retain the same phone volume, speaker/headphones and device route.

## Contents and interpretation

Each movie/mode has a .wav and .txt with distinct interpreter/jit filenames.
WAV is 48000 Hz stereo signed 16-bit little-endian PCM, with no lossy compression.
It contains the exact blocks accepted by the SDL queue adapter, including silence.
It is **not microphone audio or a hardware loopback**. The adapter copies input;
capture reads those same unchanged samples immediately after successful acceptance.
Failed writes are logged but not appended, so a retry cannot duplicate rejected PCM.

The report identifies actual SDL driver, obtained sample rate/channels/device
quantum, target/resume reserves, captured frames, truncation and omitted events.
Records contain monotonic elapsed milliseconds, operation, value, status and the
capture frame cursor. `event` values are root control ordinals, `write` values
are interleaved frames, `pause` values are flags, and `clear` values are zero.
Native result zero means success. `device.txt` includes app/Android version,
manufacturer/model, supported ABIs and AudioManager's output properties; it does
not contain account identifiers, microphone audio or private user documents.

Clear discards real queued playback but **does not erase previously captured
blocks**. A StopSounds WAV may therefore include an accepted tone that was later
cleared before reaching the device. Pause/background intervals are not inserted
into WAV. Read the operation report for clears, retries and pauses. WAV duration
is submitted/captured PCM length, not wall-clock listening duration. There is
no DAC timing or drift correction in this evidence. A clean WAV with distorted
physical output narrows the fault to SDL/device conversion or later output,
whereas distorted WAV directs investigation toward prepared PCM/host submission.

## Ownership, bounds and failures

[Native recorder](../frontend/common/audio_trace.c) is internal frontend code,
not a new VM API. Explicit `FVM_AUDIO_TRACE` environment basename enables it;
normal playback has no capture allocation or copying. The Android checkbox sets
that environment through SDL before native main. A bounded 20-second PCM buffer
(3.84 MB at current geometry) and 4096 operation slots are allocated before the
worker starts. The shared native mutex serializes all capture operations with
submit/pump/clear/pause. No VM pointers are retained and no GC roots are added.
Allocation failure disables capture with a log rather than altering sound.

[`fvm_desktop_trace_open`](../frontend/common/audio_trace.h#L12) copies geometry/path and returns an optional owned trace.
[`fvm_desktop_trace_event`](../frontend/common/audio_trace.h#L16) appends a bounded static-name operation record;
[`fvm_desktop_trace_pcm`](../frontend/common/audio_trace.h#L19) copies accepted frames, marking truncation when full.
[`fvm_desktop_trace_close`](../frontend/common/audio_trace.h#L22) writes portable WAV/report and releases ownership after
the producer worker has been joined. All functions are internally serialized;
none is independently thread-safe. PCM capture and event recording perform no
file I/O or allocation. This extra copying still has a diagnostic overhead and
is opt-in. Exceeding capacity leaves playback running and marks the report.
Native writes use temporary filenames, excluded from sharing until completed.
Write failures are logged, partial temporary files removed, and playback status
is not replaced by a diagnostic error. Missing output directories fail at save;
Android creates its private directory before launch.

The private read-only [provider](../frontend/android/java/org/bhgv/flashvm/DiagnosticProvider.java)
exposes only the generated ZIP URI; it is not exported and requires a temporary
read grant from the explicit share action. ZIP compression runs off the UI thread.
No Internet, microphone or external storage permission is added. Completed traces
are retained in app-private storage until replaced or app data is cleared.
The share target is chosen by the user; the player sends nothing automatically.

## Desktop reproduction and checks

With an SDL player already built:

```sh
FVM_AUDIO_TRACE="$PWD/01-interpreter" ./flashvm_player --no-jit --frames 360 --stats 01-animation-audio.swf
FVM_AUDIO_TRACE="$PWD/01-jit" ./flashvm_player --jit --frames 360 --stats 01-animation-audio.swf
```

The output basename's parent must exist. SDL's usual device selection applies;
a dummy driver verifies the native pipeline without audible hardware acceptance.
The SDL/X11 test `flashvm_player_audio_trace` validates real WAV headers,
non-clipped tone amplitudes/durations, interpreter/JIT peak equality, six source
controls and StopSounds markers. It uses an explicit temporary output directory
and removes it. Android JNI/provider/share execution still requires a real phone;
Java/Dex compilation and APK signing checks do not validate that lifecycle.

## v0.585: Android AudioTrack selection

The submitted Samsung SM-S931B / Android 16 / API36 traces identify SDL's AAudio
driver. All six runs fill the 960000-frame (20-second) capture in 825–1035 ms,
with 12072–12248 omitted operation records. Source events remain at approximately
0/1000/1084/1500/3000 ms. Nonzero captures have peak 6400; both 03 captures are
silent because they fill before the next audible event. This is evidence of
excessive queue consumption, not a JIT gain change. The queue clock measures
SDL dequeue, not hardware playback, so it cannot detect samples dropped later.

SDL 2.30.0's [AAudio write](https://github.com/libsdl-org/SDL/blob/release-2.30.0/src/audio/aaudio/SDL_aaudio.c#L381)
uses a 1000000 ns (1 ms) timeout for a whole block and does not retain the unwritten
suffix after a nonnegative partial write. This provides a concrete mechanism for
the observed drop/compression. The exact partial-write count was not captured.
The [Android manifest](../frontend/android/AndroidManifest.xml#L8) now sets SDL_ENV.SDL_AUDIODRIVER=android before audio init,
using SDL's blocking Java AudioTrack bridge instead. Upstream SDL is unchanged;
there is no dummy fallback. Device acceptance still requires another phone run.

The v0.585 APK reuses the v0.584 native libraries/assets byte-for-byte: no VM/audio
C implementation changed. Its manifest was rebuilt with Android SDK35 aapt2, and
the two equal-length Java version labels were updated in DEX with SHA1/Adler32
recalculated. Package/signature/16 KiB alignment were checked. Full native rebuild
and device execution were not performed for this manifest-only checkpoint.
A new disposable test signing key is used because the old transient build key
was lost. Uninstall the preceding test app before installing v0.585; export any
remaining diagnostic files first. Retain the v0.585 test keystore for future
builds (alias flashvm-test, test-only store/key password android).

After installation, repeat 01 and 03 in both modes with diagnostics. The report
should say driver=android and a 15-second foreground run should no longer fill
a 20-second capture in one second. WAV pitch/duration and physical output must
both be checked; this is a targeted workaround, not claimed device acceptance.

## v0.586: publish the negotiated device quantum

The second SM-S931B / Android16 archive confirms driver=android, expected
440/660/990/880 Hz tones, about 250 ms single voices, identical 6400 peaks in both
execution modes, 18 controls per 15-second run and no capture truncation or
omitted records. However, 01/02 submit only 604800 frames (12.6 seconds) during
15 seconds of wall time; queue drains cluster around 119–120 ms. This is not
proof of complete output acceptance. Physical listening confirmation was not
provided with that archive.

SDL can enlarge AudioTrack's block from the requested 1024 frames. With zero
allowed-change flags SDL keeps the requested `obtained.samples` and builds an
internal audio stream, hiding the larger consumer quantum. A target of 4800 frames
can then be smaller than one native consumption block, resulting in silent gaps.
The bridge now sets only SDL_AUDIO_ALLOW_SAMPLES_CHANGE. Sample rate, channels
and S16 remain fixed at the application-facing interface; SDL may perform its
normal hardware conversion. The negotiated callback quantum is published through
[`fvm_audio_sdl_queue_info`](../include/flashvm/swf_sound_output.h#L184) and used by the existing four-block reserve/two-block
resume policy. It is not a hardware latency measurement. SDL SDK >=2.0.9 is now
required so the flag is available; disabled builds keep their usual unsupported
behavior. The obtained quantum need not be a power of two even though the request
is bounded to powers of two.

The report also records `starved` when the worker observes a transition to an
empty unpaused software FIFO. Its value/status are zero; this is an observation,
not a backend error or a measurement of hardware silence. No extra PCM is inserted
into the WAV for a gap. StopSounds still keeps previously accepted blocks in the
capture, so compare its operation report when reading 03.

v0.586 rebuilds native VM/player and SDL2 2.30.0 for Android ARM64/API23 with
NDK r27c and Java/Dex with JDK17/SDK35. It reuses the persisted v0.585 test signer;
installing over v0.585 should retain application data. The AAudio workaround stays.
The new report should expose the enlarged quantum and corresponding reserve;
for uninterrupted 01/02 playback the submitted-frame clock should approach 48000
frames per second after startup, within queue/backlog and final-shutdown bounds.
15 seconds of wall time is not required to produce exactly 15 seconds of WAV.
Recheck 01/03 in interpreter/JIT and physical output. Device confirmation of this
new behavior remains pending.

## v0.587 optional stop release

[Player policy, new APIs and limitations](PLAYER_AUDIO_RELEASE.md). Original immediate stop behavior remains the default.

## v0.588 ADPCM fixture block restarts

The six Samsung/Android 16 v0.587 captures have no reported software starvation
or backend errors. They contain the same 880 Hz ADPCM transient about 186 ms into
each 250 ms tone, near its end. This is separate from explicit stop releases.
The 660/990 Hz stereo PCM tone has no corresponding block restart.

The [fixture encoder](../examples/player/animation_audio/make_movies.py) reset
its adaptive step index to zero at every 4096-frame block header. At source
frame 8192 the following codes cannot track a full-level sinusoid until the step
size catches up. Independent decoding of the old asset gives a maximum error of
2530 PCM units near that boundary. The encoder now retains the adapted index
while writing the new absolute predictor. The first block still starts at zero;
valid external ADPCM is decoded unchanged. All three shipped SWFs are regenerated.
The same boundary error is now 83, and the largest source-frame step in that
window drops from 2134 to 748. These are fixture measurements, not DAC recordings.

[Independent asset regression](../frontend/tests/audio_assets.py) checks both
internal boundaries against the intended analytic sinusoid without importing
the encoder or VM decoder. [Player capture regression](../frontend/tests/audio_trace.py)
also checks the resampled mono tone for large steps in interpreter and JIT.
Run `python3 frontend/tests/audio_assets.py` from the component directory.
The user has now confirmed correct listening on the affected phone for all six
test/mode combinations. The new archive contains the corrected waveform (880 Hz
maximum output step 688), though its old hard-coded device label says 0.587;
v0.589 reads the actual packaged version. v0.588 changes test
assets, not playback filtering, output gain or general decoder semantics.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_sdl_queue_info` | function | [include/flashvm/swf_sound_output.h:184](../include/flashvm/swf_sound_output.h#L184) |
<!-- source-links:end -->
