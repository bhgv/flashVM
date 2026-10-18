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

[`fvm_desktop_trace_open`](../frontend/common/audio_trace.h#L9) copies geometry/path and returns an optional owned trace.
[`fvm_desktop_trace_event`](../frontend/common/audio_trace.h#L13) appends a bounded static-name operation record;
[`fvm_desktop_trace_pcm`](../frontend/common/audio_trace.h#L16) copies accepted frames, marking truncation when full.
[`fvm_desktop_trace_close`](../frontend/common/audio_trace.h#L19) writes portable WAV/report and releases ownership after
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
