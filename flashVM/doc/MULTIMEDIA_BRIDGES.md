# Мосты мультимедиа и устройств / Multimedia and device bridges

## Назначение / Scope

C89 API в [multimedia.h](../include/flashvm/multimedia.h) отделяет хост от VM:
декодирование аудио/видео, явный захват камеры, вывод PCM/RGBA и опрос устройств.
FFmpeg backend вызывает библиотеки напрямую. Воспроизведение и окно принадлежат
приложению: таблица вывода подключается к реальной библиотеке ОС. Явные
DefineSound/StartSound и root streaming PCM теперь можно направлять в
зарегистрированный output: [SWF_SOUND_RESOURCES.md](SWF_SOUND_RESOURCES.md),
[SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md). Автоматическая MovieClip audio/video
интеграция и полный AS Sound/Camera/Microphone остаются открытыми. StopSounds теперь управляет явно зарегистрированными
контекстными выходами: [AUDIO_CHANNELS.md](AUDIO_CHANNELS.md). Это не является заявлением о завершении мультимедиа Flash.

The C ABI separates decoding/capture, host playback/presentation and native
input. Default NO_MEDIA retains custom bridges; MEDIA enables the real static
FFmpeg decoder. No process invocation, hidden device open, background thread,
implicit network connection or automatic input ActionScript dispatch is performed.
Context output registration and AVM1/global stopping are documented in
[AUDIO_CHANNELS.md](AUDIO_CHANNELS.md).

Explicit root SWF streaming PCM inspection and caller-buffer conversion feed
the same output ABI: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md). This does not
add progressive network decoding or automatic MovieClip sound scheduling.

Explicit native Linux PCM output is available with a supplied static ALSA SDK:
[ALSA_AUDIO_OUTPUT.md](ALSA_AUDIO_OUTPUT.md). Disabled builds remain dependency-free.

Explicit loaded SWF DefineSound inspection/16-bit PCM submission is documented
in [SWF_SOUND_RESOURCES.md](SWF_SOUND_RESOURCES.md); automatic sound timeline
controls remain unsupported.

## Дерево исходников / Source layout

| Directory | Responsibility |
| --- | --- |
| `src/multimedia/audio` | Audio open/write/stop and FFmpeg PCM conversion |
| `src/multimedia/video` | Video open/present and FFmpeg RGBA conversion |
| `src/multimedia/camera` | Explicit camera opening and registered device-format lookup |
| `src/multimedia/input` | Common polling plus Linux, Windows, macOS and Android functions |
| `src/multimedia/common` | Session ownership, validation, FFmpeg open/read/seek/close/backend |
| `src/multimedia/speech/tts` | Native text-to-speech providers and explicit voice sessions |
| `src/multimedia/speech/stt` | Native microphone recognition and retained UTF-8 transcripts |
| `external/ffmpeg` | Pinned explicit static SDK build recipe and CMake import |

Функции разнесены по операциям; каждый файл помечает ОС, каждая строка нового
переходника и примеров пояснена. Обёртки системного ввода работают независимо от
MEDIA и OS_PORTS. Для speech/TTS/STT используется такой же принцип;
[реальные системные speech bridges и подготовка host](SPEECH_BRIDGES.md).
Сам FFmpeg не синтезирует и не распознаёт речь.

## Общий контракт / Common contract

Все вызовы синхронные. Один session обслуживается одним потоком за раз, включая
закрытие. Инициализировать FFmpeg backend следует на потоке настройки до
параллельных открытий. Независимые sessions допускают отдельные потоки при
потокобезопасном backend/userdata. Callback не должен выбрасывать C++ исключения,
повторно входить в тот же session или закрывать его во время собственного вызова.
Хостовые UI/device службы могут дополнительно требовать главный поток ОС.

Sessions use unmanaged native allocations, not VM objects or the VM memory
budget. No [FVM_Context](../src/core/runtime.hpp#L238) is retained. Host userdata, referenced labels and output
handles must outlive calls/sessions. [GC:] Managed VM values kept by a host
callback need explicit existing VM roots; an opaque pointer is not a GC edge.
Do not call VM APIs from an arbitrary device callback thread; enqueue a host-owned
copy and dispatch on the VM thread with the usual context restrictions.

Таблицы bridge проверяются по точному struct_size; таблица декодера также по
[FVM_MEDIA_ABI_VERSION](../include/flashvm/multimedia.h#L9). Это версия ABI моста, не [FVM_ABI_VERSION](../include/flashvm/flashvm.h#L25). Поля сначала
обнуляются; неизвестные callbacks остаются null. Будущее расширение структуры
требует согласованного изменения ABI, не дописывания полей за старым размером.

Statuses: [FVM_OK](../include/flashvm/flashvm.h#L42) means completed work; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) means invalid ABI,
arguments or caller-supplied output geometry; [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) means allocation
failure where identified; [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) means absent backend/operation/codec;
[FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) means EOF **or temporary lack of an input frame/event**; [FVM_ERROR](../include/flashvm/flashvm.h#L43)
means I/O, decoding or malformed successful callback output. No new result enum
values are introduced. FFmpeg may write its normal diagnostic log to stderr.
Some upstream failures deliberately share [FVM_ERROR](../include/flashvm/flashvm.h#L43) rather than exposing native
error numbers. There is no asynchronous error callback in this revision.

## Форматы и владение / Formats and ownership

[FVM_MediaRequest](../include/flashvm/multimedia.h#L30) borrows a NUL-terminated filename/device URL during open only.
Builtin file opens probe a format; camera opens require an explicit registered
video-input device format. Format is reserved for named-format custom bridges;
the file convenience functions pass null. Each open selects one stream. Open the
file separately for audio and video; packets from other streams are skipped.
There is no shared A/V clock, resynchronization, mixing or subtitle extraction.

[FVM_MediaFrame](../include/flashvm/multimedia.h#L37) is a borrowed view, invalidated by **any next read, seek or close**
on its session, including failed operations. Copy bytes before retaining them.
PCM is native-endian signed 16-bit, interleaved, source rate/channel count and
native channel order. `samples` counts sample frames, `size` counts bytes; no
portable channel-position metadata is exported yet. `sizeof(short)==2` and the
usual signed-16 range are required. Conversion changes packing/sample format,
not sample rate or channel layout. RGBA8 uses width/height/stride in bytes, with
four bytes per pixel; builtin FFmpeg output is tightly packed. Swscale uses its
default color policy; full HDR/range/color-management parity is not claimed.

`seconds` is meaningful only with `has_timestamp`; it uses the source stream's
time base and can have a nonzero start. No monotonic VM time conversion occurs.
The output callbacks must consume/copy bytes before returning; queued playback
must own its copied buffer and report its own completion/drain policy.

## Написание декодера / Writing a decoder or capture bridge

1. Create native state holding the real library/device handles and reusable
   output buffers. Choose its allocation/cleanup rules explicitly.
2. Fill [FVM_MediaBackend](../include/flashvm/multimedia.h#L56) with exact size/version, a stable os_name, userdata and
   open/read/close. Seek is optional. The session copies this table at open;
   callback code and userdata must remain alive, even if the table goes away.
3. Open validates request.kind and URL, creates native resources and writes one
   non-null private stream only on [FVM_OK](../include/flashvm/flashvm.h#L42). **A failing open must free its own
   partial allocations**; common code releases only its unpublished wrapper.
4. Read writes a valid [FVM_MediaFrame](../include/flashvm/multimedia.h#L37). Do not return success with an empty buffer.
   Common read clears outputs, checks kind and overflow-safe geometry, and clears
   partial output on failure. Native state still owns the bytes.
5. Seek flushes every compressed/decoded/resampler reference and resets EOF.
   Null seek returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Close frees each native resource once.
6. Add a real input fixture, malformed-input/EOF/seek/cleanup tests, a complete
   annotated example and source-linked API documentation. Do not confuse a fake
   source used in a test with a supported real device.

См. исполняемый [custom_audio_bridge](../examples/multimedia/custom_audio_bridge/README.md):
он действительно читает raw PCM через stdio, не имитирует воспроизведение.

## Вывод в систему / Writing a playback or presentation bridge

Fill [FVM_MediaOutput](../include/flashvm/multimedia.h#L67) with exact size, borrowed userdata, synchronous write and
optional stop. Audio/video entrypoints validate geometry before calling write.
Use the real OS/library queue: negotiate sample rate/channels or pixel format,
copy borrowed bytes if the queue is asynchronous, bound queue growth, and release
queued buffers according to that library's completion API. Surface failures as
[FVM_Result](../include/flashvm/flashvm.h#L40). Stop must stop the chosen output and discard pending buffers according
to the host contract; it does not implicitly close a source or stop every VM sound.

Пример [custom_video_output](../examples/multimedia/custom_video_output/README.md)
показывает реальный файловый вывод PPM. Для окна заменить callback вызовом
системной библиотеки; для audio — callback записи в настоящий аудиобуфер. Готового
универсального воспроизведения на звуковую карту в этом checkpoint нет.

## API: фабрика и источники / Factory and source API

### fvm_media_ffmpeg_backend

`backend` must point to writable [FVM_MediaBackend](../include/flashvm/multimedia.h#L56) storage; no input initialization
is required. The function clears it, registers the target SDK's device formats
and fills callbacks. Null gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); NO_MEDIA gives
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) with an empty table; MEDIA gives [FVM_OK](../include/flashvm/flashvm.h#L42). No resources need closing
for the table itself. Setup-thread initialization restriction is described above.
Build instructions and exact codec/device scope: [static SDK](../external/ffmpeg/README.md).

### fvm_audio_open

Arguments: compatible backend, nonempty NUL-terminated filename/URL, writable
out_session. Clears output before validation. Selects [FVM_MEDIA_AUDIO](../include/flashvm/multimedia.h#L19). Success
transfers session ownership; close it once. Invalid arguments/table give
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); wrapper allocation gives [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44); other results
come from open (FFmpeg uses [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for no matching stream/decoder,
[FVM_ERROR](../include/flashvm/flashvm.h#L43) for failed file inspection). No output playback is started.

### fvm_video_open

Same arguments, ownership, thread and error rules as [fvm_audio_open](../include/flashvm/multimedia.h#L111), selecting
[FVM_MEDIA_VIDEO](../include/flashvm/multimedia.h#L20). Other stream packets are skipped. Output is RGBA, not a native
window or graphics resource. A file with only audio cannot produce video success.

### fvm_camera_open

Arguments: backend, explicit device string, explicit nonempty format string,
out_session. Selects [FVM_MEDIA_CAMERA](../include/flashvm/multimedia.h#L21); read produces [FVM_MEDIA_VIDEO](../include/flashvm/multimedia.h#L20) frames.
Invalid format strings give [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); unavailable/unregistered
video-input formats give [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46); real device-open failures give
[FVM_ERROR](../include/flashvm/flashvm.h#L43). Close releases the actual capture device. The host acquires required
permissions before calling; there is no device auto-discovery/default fallback.
FFmpeg may block while opening/reading a camera; host scheduling owns cancellation
and worker policy. This API has no timeout or interrupt callback yet.

### fvm_media_read

Arguments: live session and writable frame. The frame is cleared before work.
Null arguments give [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); successful malformed backend output
becomes [FVM_ERROR](../include/flashvm/flashvm.h#L43) with a cleared frame. Other callback results are preserved.
Borrowed buffer lifetime is specified above. Repeated read at terminal FFmpeg
EOF returns [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47). Temporary device EAGAIN also returns [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47);
a live host must schedule another poll, while finite-file loops can treat it as
EOF. The library demuxer/decoder drains delayed frames before terminal EOF.

### fvm_media_seek

Arguments: live session, finite nonnegative seconds. Invalid arguments give
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); absent/failed native seek gives [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). FFmpeg
rejects timestamps outside its signed clock range, seeks to a preceding keyframe,
flushes packet/frame/codec state and resets drain flags. [FVM_OK](../include/flashvm/flashvm.h#L42) does not guarantee
the next frame is exactly at the requested time; inspect timestamps and discard
preceding frames as needed. Old buffer views are invalid by contract even if a
seek fails. Camera sources typically cannot seek.

### fvm_media_close

Argument: session or null. Returns void. Calls native close exactly once, then
frees the common wrapper. Null is harmless; duplicate non-null close and use of
copies afterwards are invalid. Close must not race with read/seek/callbacks.
No context or GC lifetime is implied.

## API: вывод / Output API

### fvm_audio_write

Arguments: exact-size [FVM_MediaOutput](../include/flashvm/multimedia.h#L67) with non-null write, valid AUDIO frame.
Checks non-null data, size, rate/channels/sample count and overflow-safe S16
capacity. Invalid input gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) without invoking callbacks;
otherwise returns write(userdata, frame) unchanged. Borrows both arguments only
during the call; the output provider owns native handles and any copied queue.

### fvm_video_present

Same table/callback/lifetime rules as [fvm_audio_write](../include/flashvm/multimedia.h#L128), requiring VIDEO, positive
width/height, stride large enough for RGBA pixels and accessible complete rows.
Bad geometry gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Presentation/window ownership and pacing
belong to write; success is the host callback's accepted-work result.

### fvm_audio_stop

Arguments: exact-size output table. Invalid table gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45);
null stop gives [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46); otherwise returns stop(userdata) unchanged.
Does not close streams or free host userdata. The host defines whether the real
system stop is synchronous and how previously copied buffers are reclaimed.

## Устройства и будущие датчики / Devices and future sensors

[FVM_DeviceBridge](../include/flashvm/multimedia.h#L101) contains exact struct_size, userdata and poll. The table is
borrowed for a call, not copied or registered globally. [FVM_DeviceEvent](../include/flashvm/multimedia.h#L92) has kind,
native_code, x/y/z and native seconds. No strings or GC objects cross the ABI.
Existing script input API remains separate; normalize codes/coordinates and call
it from the VM thread deliberately. See [input API](../include/flashvm/input.h).

For accelerometers, supply **complete XYZ samples in m/s²** including or excluding
gravity according to the provider's documented contract. Compass uses XYZ µT,
gyroscope XYZ rad/s. Orientation requires an explicit host convention (axes,
handedness, radians/quaternion/Euler semantics); this revision does not pretend a
universal orientation representation. Set native_code to a documented sensor ID.
Declare the timestamp clock, screen rotation, calibration, permission requirements,
sample rate and dropped-sample policy. Transport owned copies between threads.
No built-in compass/accelerometer/gyro driver is claimed in this checkpoint;
these categories permit future real sensor bridges without smuggling pointers
or changing the audio/video buffer layout.

### fvm_device_poll

Arguments: exact-size bridge with poll, writable event. Clears output first;
invalid arguments give [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Calls poll(userdata,event); preserves
NOT_FOUND/error. Success with an unknown kind becomes [FVM_ERROR](../include/flashvm/flashvm.h#L43) and a cleared
view. Known kinds are not automatically calibrated or validated for device units;
that is the provider's contract. No native queue ownership is transferred.

### fvm_input_linux_evdev_read

Arguments: host-owned **nonblocking** fd and writable event. Negative fd/null
output give [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Linux calls real read for one input_event.
EAGAIN/EWOULDBLOCK/EINTR, filtered SYN/other records give [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47); EOF,
partial record or hard errors give [FVM_ERROR](../include/flashvm/flashvm.h#L43). Other OSes give [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
Descriptor remains open with unchanged flags. EV_KEY preserves keys and mouse
buttons (x=0/1/2); EV_REL gives mouse/wheel deltas; EV_ABS gives native pointer or
multitouch fragments (x=value, native_code=axis/slot/contact identifier). y/z are
zero. This is not a reconstructed multitouch gesture/frame: host must track slots,
ABS ranges, SYN boundaries and screen mapping separately. Timestamp is the native
evdev clock converted from microseconds, not necessarily the VM clock.

### fvm_input_windows_pointer

Argument: writable event. Null gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). On Windows uses
GetCursorPos; failure gives [FVM_ERROR](../include/flashvm/flashvm.h#L43); success returns MOUSE with global desktop
pixel x/y, native_code=0, seconds=0. Other targets return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) with an
empty event. No mouse buttons or script notifications are synthesized.

### fvm_input_windows_key

Arguments: native virtual-key code and writable event. Null gives
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); Windows rejects code above [FVM_DEVICE_WINDOWS_VK_MAX](../include/flashvm/multimedia.h#L11).
GetAsyncKeyState sign bit supplies KEY x=1/0, native_code=key. It deliberately
ignores the unreliable historical low bit. OS zero cannot distinguish key-up
from inaccessible desktop/UIPI restrictions. [FVM_OK](../include/flashvm/flashvm.h#L42) therefore reports the API's
observed state, not guaranteed keyboard permission. Other OSes return
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). This is a snapshot, not a queue of transitions.

### fvm_input_macos_pointer

Argument: writable event. On macOS creates a temporary CGEvent, reads global
Core Graphics display x/y and releases it. Null gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45);
failed CGEvent creation gives [FVM_ERROR](../include/flashvm/flashvm.h#L43); other targets give [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
Success gives MOUSE with native_code=0, seconds=0. Host owns coordinate conversion
and additional platform permissions if its integration requires them.

### fvm_input_android_motion

Arguments: borrowed live AInputEvent pointer as const void*, pointer index,
writable event. Null arguments give [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Android validates the
event is MOTION and index is within pointer count before native accessors.
Produces TOUCH for pointer samples (including mouse/stylus motion sources), local
pixel x/y, z=stable pointer ID, native_code=full packed action. The host decodes
source/action/index bits and calls for every desired pointer. Native event time
is converted from nanoseconds. No queue acquire/finish/release is performed by
FlashVM; caller owns that lifecycle. Other OSes give [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). It is not a
keyboard decoder or gesture recognizer.

## Платформы, тесты и ограничения / Platform evidence

Linux: compiled strict C89/C++98 public/implementation ABI; real static FFmpeg
6.1.2 WAV/PPM byte/pixel tests, malformed input, EOF, seek and cleanup;
custom callbacks and evdev pipe fixtures. Pipe fixtures validate native decoding,
**not a physical keyboard/touchscreen**. Camera needs a real permitted device;
there is no camera in this runner, so live capture is not claimed as tested.
Windows/macOS/Android branches call real SDK functions but await native compile,
permission and hardware runs. No overall portability acceptance is inferred.

Examples: [custom audio](../examples/multimedia/custom_audio_bridge/README.md),
[custom video output](../examples/multimedia/custom_video_output/README.md),
[FFmpeg decoding](../examples/multimedia/ffmpeg_decode/README.md),
[system devices](../examples/multimedia/system_devices/README.md).

Primary SDK references: [FFmpeg device API](https://ffmpeg.org/doxygen/6.1/group__lavd.html),
[decode send/receive](https://ffmpeg.org/doxygen/6.1/group__lavc__encdec.html),
[sample conversion](https://ffmpeg.org/doxygen/6.1/group__lswr.html),
[Windows key state](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate),
[Windows cursor](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getcursorpos),
[Core Graphics position](https://developer.apple.com/documentation/coregraphics/cgevent/location),
[Android NDK input](https://developer.android.com/ndk/reference/group/input).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ABI_VERSION` | macro / define | [include/flashvm/flashvm.h:25](../include/flashvm/flashvm.h#L25) |
| `fvm_audio_open` | function | [include/flashvm/multimedia.h:111](../include/flashvm/multimedia.h#L111) |
| `fvm_audio_stop` | function | [include/flashvm/multimedia.h:134](../include/flashvm/multimedia.h#L134) |
| `fvm_audio_write` | function | [include/flashvm/multimedia.h:128](../include/flashvm/multimedia.h#L128) |
| `fvm_camera_open` | function | [include/flashvm/multimedia.h:117](../include/flashvm/multimedia.h#L117) |
| `FVM_Context` | struct | [src/core/runtime.hpp:238](../src/core/runtime.hpp#L238) |
| `fvm_device_poll` | function | [include/flashvm/multimedia.h:139](../include/flashvm/multimedia.h#L139) |
| `FVM_DEVICE_WINDOWS_VK_MAX` | macro / define | [include/flashvm/multimedia.h:11](../include/flashvm/multimedia.h#L11) |
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:101](../include/flashvm/multimedia.h#L101) |
| `FVM_DeviceEvent` | struct | [include/flashvm/multimedia.h:92](../include/flashvm/multimedia.h#L92) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_android_motion` | function | [include/flashvm/multimedia.h:154](../include/flashvm/multimedia.h#L154) |
| `fvm_input_linux_evdev_read` | function | [include/flashvm/multimedia.h:142](../include/flashvm/multimedia.h#L142) |
| `fvm_input_macos_pointer` | function | [include/flashvm/multimedia.h:152](../include/flashvm/multimedia.h#L152) |
| `fvm_input_windows_key` | function | [include/flashvm/multimedia.h:149](../include/flashvm/multimedia.h#L149) |
| `fvm_input_windows_pointer` | function | [include/flashvm/multimedia.h:146](../include/flashvm/multimedia.h#L146) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MEDIA_ABI_VERSION` | macro / define | [include/flashvm/multimedia.h:9](../include/flashvm/multimedia.h#L9) |
| `FVM_MEDIA_AUDIO` | macro / define | [include/flashvm/multimedia.h:19](../include/flashvm/multimedia.h#L19) |
| `FVM_MEDIA_CAMERA` | macro / define | [include/flashvm/multimedia.h:21](../include/flashvm/multimedia.h#L21) |
| `fvm_media_close` | function | [include/flashvm/multimedia.h:125](../include/flashvm/multimedia.h#L125) |
| `fvm_media_ffmpeg_backend` | function | [include/flashvm/multimedia.h:108](../include/flashvm/multimedia.h#L108) |
| `fvm_media_read` | function | [include/flashvm/multimedia.h:120](../include/flashvm/multimedia.h#L120) |
| `fvm_media_seek` | function | [include/flashvm/multimedia.h:123](../include/flashvm/multimedia.h#L123) |
| `FVM_MEDIA_VIDEO` | macro / define | [include/flashvm/multimedia.h:20](../include/flashvm/multimedia.h#L20) |
| `FVM_MediaBackend` | struct | [include/flashvm/multimedia.h:56](../include/flashvm/multimedia.h#L56) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:37](../include/flashvm/multimedia.h#L37) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:67](../include/flashvm/multimedia.h#L67) |
| `FVM_MediaRequest` | struct | [include/flashvm/multimedia.h:30](../include/flashvm/multimedia.h#L30) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_video_open` | function | [include/flashvm/multimedia.h:114](../include/flashvm/multimedia.h#L114) |
| `fvm_video_present` | function | [include/flashvm/multimedia.h:136](../include/flashvm/multimedia.h#L136) |
<!-- source-links:end -->
