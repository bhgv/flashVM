# Context audio channels / Аудиоканалы контекста

## Scope

A channel connects a VM context to an actual host [`FVM_MediaOutput`](../include/flashvm/multimedia.h#L66) write/stop
pair. [`fvm_audio_channel_write`](../include/flashvm/multimedia.h#L77) submits already decoded native-endian signed-16
interleaved PCM synchronously. AVM1 StopSounds (0x09) and installed global
`stopAllSounds()` call [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L85). They consume no action operands and
produce no action result; the global function ignores arguments and returns
undefined on success. Host errors propagate without replaying the action.

This is output registration and stopping, **not complete SWF sound playback**.
DefineSound, StartSound, SoundStreamHead/Block, mixing, envelopes, looping,
seek synchronization, AS Sound objects, natural-completion notifications and
sound-card drivers are not implemented by this change. Standalone
[`fvm_audio_write`](../include/flashvm/multimedia.h#L122) / [`fvm_audio_stop`](../include/flashvm/multimedia.h#L124) do not register an output automatically.
FFmpeg-decoded frames can be submitted explicitly through a registered channel;
no file/device is opened by channel creation or by an empty StopSounds call.
NO_MEDIA supports custom outputs without requiring FFmpeg.

## Ownership, execution and callback contract

The context owns each [`FVM_AudioChannel`](../src/multimedia/audio/channel.hpp#L6) wrapper and copies the complete output
table. The host owns callback code, userdata and devices until close succeeds,
or until context destruction finishes. Closing a channel never frees userdata.
Handles are not managed objects, do not participate in graph serialization and
must not be used after successful close or context destruction. Channels belong
to one context; transferring a function/object does not transfer devices.

Call only from the context's owning VM thread. Creation, write, stop, close and
stop-all reject destruction, GC collection/release, and nested channel mutation
from an output callback with INVALID_ARGUMENT. Ordinary VM/native callers are
allowed. State queries are allowed during a write/stop callback but report only
previously acknowledged state. A callback must return normally across the C ABI:
do not throw C++ exceptions, change threads, destroy the runtime or retain frame
pointers. Context destruction attempted from a callback is rejected by the
ordinary native barrier. Do not retain that context beyond its owning lifetime.

[GC:] Output callbacks run behind the native collection barrier. Explicit
collection from them is rejected. There are no managed references in the
registry: userdata is a host pointer, not an implicitly traced capture. Hosts
retaining managed objects must establish their own explicit roots and release
them before context destruction. Normal GC neither stops nor finalizes channels.

An active flag records a successful write, not a physical device probe. It stays
active after natural device completion until stopped. Failed writes leave the
previous active flag unchanged; a backend must clean partially opened/queued
resources on failure or keep them in its own separately cleaned host state.
Successful stop must stop/discard that output's pending playback. The same
userdata/device registered twice is two registrations; the host must avoid
unintended shared-device effects. Keep stop callbacks bounded: all work is
synchronous on the VM thread; there is no hidden output worker.

## fvm_audio_channel_create

**Parameters:** required context, output table, and out-channel pointer. Table
struct_size must exactly equal sizeof([FVM_MediaOutput](../include/flashvm/multimedia.h#L66)); write and stop are both
required; userdata may be null if the callbacks accept it. The table is copied,
so its stack storage can disappear immediately after this call.

**Results:** OK publishes a new inactive channel. INVALID_ARGUMENT rejects a
null/malformed table, null destination or invalid context phase. OUT_OF_MEMORY
contains native wrapper/registry allocation failure. If the output pointer is
valid, it is cleared before validation; failure publishes nothing and does not
call host code. Creation allocates only native registration storage, never opens
a device and does not perform VM GC. New wrappers are outside managed heap
accounting, like existing multimedia sessions.

## fvm_audio_channel_write

**Parameters:** required live channel and borrowed [FVM_MediaFrame](../include/flashvm/multimedia.h#L36). It must be
AUDIO with valid non-null data, nonzero sample rate/channel/sample-frame counts,
complete S16 buffer size and overflow-safe bounds, following the existing media
frame validator. Width/height/stride are not audio fields.

**Results:** INVALID_ARGUMENT for invalid lifecycle/frame, before host code;
otherwise exactly the write callback's result. Success marks active. Failure
retains the previous flag. There is no allocation, conversion or implicit copy;
the host must consume/copy bytes before returning. It may not retain the view.
A later successful write after stop starts the same registered output again.

## fvm_audio_channel_stop

**Parameters:** required live channel. **Results:** INVALID_ARGUMENT for null or
invalid owner phase. Inactive returns OK without entering the host. Active calls
stop once: OK clears active, other statuses preserve active for retry. No
allocation occurs. The handle remains registered regardless of result.

## fvm_audio_stop_all

**Parameters:** required owning context. **Results:** INVALID_ARGUMENT for
invalid context/callback reentry; otherwise attempts every active registration
once in registration order. Returns the first non-OK callback status, after
attempting the remaining outputs. Successful outputs become inactive; failed
ones remain active and are the only ones retried by another stop-all. Empty
contexts return OK. There is no allocation, replay, managed capture or change to
other contexts, decoders, video, speech sessions or standalone output tables.

## fvm_audio_channel_close

**Parameters:** a live channel, or null for partial-setup cleanup. **Results:**
null returns OK. Otherwise it applies the same lifecycle validation and stop
policy as the single-channel stop. Failure returns that status and leaves the
handle valid. After successful stop, unregisters/frees the wrapper and returns
OK without freeing host userdata. The caller must discard the handle immediately.
No allocation occurs. A stale handle is invalid, including a second close on a
successfully closed non-null pointer.

## fvm_audio_channel_active

**Parameters:** required live channel and required integer output. **Results:**
OK copies zero/one; INVALID_ARGUMENT for null arguments, destruction or GC
collection/release. Failed validation leaves output unchanged. No allocation,
callback or ownership transfer occurs. Unlike mutation APIs, the query may be
used inside an output callback; it observes state before the current callback's
success is acknowledged.

## Context teardown

Context destruction stops each still-active registration once and releases every
wrapper before finalizing the managed graph. Userdata must remain alive through
this step. Destruction is terminal and has no result channel: a failed stop is
not retried and its error cannot be returned from the void destroy function.
Hosts needing reliable error handling must stop/close explicitly first. The
host remains responsible for releasing its device even if stop failed. Teardown
callbacks must not call back into the VM being destroyed.

## JIT and transport

A body containing StopSounds currently follows interpreter fallback when the
native JIT compiler cannot compile it. It is never replaced with a successful
no-op. `stopAllSounds` has a stable native callback identity in graph transport;
a transported function stops the destination context's outputs, not its source.
Reaching 100/100 dispatch source references is an inventory gate, not complete
AVM1, audio or platform conformance.

## Русский контракт

Канал связывает контекст с настоящими host callbacks записи PCM и остановки.
Регистрация копирует таблицу, но не владеет userdata и не открывает устройство.
Успешная запись включает active, успешный stop выключает. Ошибка write не меняет
старый флаг; backend обязан сам убрать частичные ресурсы или оставить их в своём
состоянии для явного cleanup. Ошибка stop оставляет канал активным для повтора.
[`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L85), Action 0x09 и `stopAllSounds()` пробуют все активные каналы
по порядку, возвращая первую ошибку после остальных попыток. Чужие контексты,
standalone output, декодеры, видео и речь не затрагиваются.

Все вызовы — на VM-потоке. В host callback запрещены повторные изменения
реестра и GC; read-only active разрешён. [GC:] Реестр не содержит управляемых
ссылок; host userdata не является автоматически трассируемым capture. Параметры,
статусы и lifetime каждой функции перечислены выше и одинаковы для RU/EN.
Close принимает null; при ошибке stop сохраняет handle, при успехе уничтожает.
Двойной close старого ненулевого указателя запрещён. При уничтожении контекста
все активные выходы получают последнюю попытку stop, ошибки не возвращаются,
handles становятся недействительными. Host обязан держать userdata до конца
этого процесса и отдельно освободить устройство даже при ошибке stop.

Это основа остановки зарегистрированных выходов; SWF DefineSound/StartSound,
потоковый звук, микшер, огибающие, циклы и полный AS Sound ещё не реализованы.
Нативные звуковые устройства других ОС не проверены этой реализацией.

## Validation and tutorial

Tests: audio_channels (C89, SWF1–8 action dispatch, requested interpreter/JIT,
real host callback counters, isolation, retry, reentry and teardown),
audio_channels_allocation (atomic OOM and allocation-free stop/close),
audio_channels_transport (actual serialized global function on another VM).
The [audio_channels tutorial](../examples/multimedia/audio_channels/README.md)
uses actual fwrite/fclose on an explicit PCM file; this is observable output
control, not evidence of audible sound-card playback.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:83](../include/flashvm/multimedia.h#L83) |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:81](../include/flashvm/multimedia.h#L81) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:75](../include/flashvm/multimedia.h#L75) |
| `fvm_audio_channel_stop` | function | [include/flashvm/multimedia.h:79](../include/flashvm/multimedia.h#L79) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:77](../include/flashvm/multimedia.h#L77) |
| `fvm_audio_stop` | function | [include/flashvm/multimedia.h:124](../include/flashvm/multimedia.h#L124) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:85](../include/flashvm/multimedia.h#L85) |
| `fvm_audio_write` | function | [include/flashvm/multimedia.h:122](../include/flashvm/multimedia.h#L122) |
| `FVM_AudioChannel` | struct | [src/multimedia/audio/channel.hpp:6](../src/multimedia/audio/channel.hpp#L6) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:36](../include/flashvm/multimedia.h#L36) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:66](../include/flashvm/multimedia.h#L66) |
<!-- source-links:end -->
