# Linux ALSA output / Вывод PCM через ALSA

Native ALSA symbols are third-party declarations in the supplied SDK's
`include/alsa/pcm.h` (upstream source `include/pcm.h`), rather than first-party
FlashVM functions. Their versioned source archive is linked in the
[SDK guide](../external/audio/README.md); no guessed local SDK/source links are used.

## Build and endpoint preparation

The optional provider uses actual Linux `snd_pcm_open`, `snd_pcm_set_params`,
`snd_pcm_writei`, `snd_pcm_start`, `snd_pcm_prepare`, `snd_pcm_drop` and
`snd_pcm_close`. It supplies a normal [`FVM_MediaOutput`](../include/flashvm/multimedia.h#L67) table for
[`fvm_audio_channel_create`](../include/flashvm/multimedia.h#L81), so AVM1 StopSounds and global stopAllSounds reach the
real endpoint. No FFmpeg subprocess, default device selection or automatic
permission prompt is introduced. Channels do not own this provider.

Enable `FVM_ENABLE_LINUX_AUDIO=ON` and provide `FVM_ALSA_ROOT` with matching static
ALSA headers/archive; see [SDK preparation](../external/audio/README.md).
This is independent of MEDIA and Linux STT/Vosk. The default build has no ALSA
link dependency and returns UNSUPPORTED from a valid open request. Enabling the
option on a non-Linux/Android target or with missing SDK components fails CMake
configuration; no shared-library fallback or network acquisition occurs.

Choose an ALSA PCM name explicitly, such as a host-configured `default`,
`plughw:0,0` or another installed named endpoint. Ensure device node ACLs, desktop
session/sound-server access and ALSA configuration/plugins are prepared outside
FlashVM. `default` may resolve through the host's PulseAudio/PipeWire plugin;
that is host configuration, not a guaranteed dependency or fallback. Driver and
plugin availability are not inferred from successful SDK compilation. Static
ALSA plugins/configuration may have additional system dependencies; use a
matching SDK and retain its license/source notices for distribution.

## fvm_audio_linux_alsa_open

**Purpose:** explicitly acquire/configure one host-owned Linux PCM playback
provider and publish its copied-table-compatible callbacks.

**Parameters, in order:** device is a required nonempty borrowed PCM name; rate
is a nonzero samples-per-second request; channels is a nonzero interleaved channel
count; latency_us is a nonzero ALSA buffer-latency request in microseconds;
out is the required [FVM_AlsaOutput](../src/multimedia/audio/linux_alsa.hpp#L7) handle destination; output is the required
[FVM_MediaOutput](../include/flashvm/multimedia.h#L67) table destination. Device text is needed only during the call.
Both valid destinations are cleared before any argument/provider validation.

**Format:** native-endian signed 16-bit, RW_INTERLEAVED. The provider requests
exact input rate/channel geometry and passes zero for ALSA soft-resampling;
host-selected ALSA plugins can have their own conversions. It uses
SND_PCM_NONBLOCK; open/configuration themselves are synchronous library calls,
not a hard real-time guarantee. Configuration prepares the stream but sends no
samples and starts no playback. Unsupported geometry/device/configuration returns
ERROR after closing any successfully opened partial handle.

**Results:** OK transfers one live provider handle to the host and publishes a
complete output table. INVALID_ARGUMENT rejects null destinations, empty device
or zero numeric settings before native calls. OUT_OF_MEMORY reports wrapper
allocation failure. UNSUPPORTED means the provider was not compiled in. ERROR
reports actual native open/configuration failure. Every failure leaves valid
out destinations cleared. No VM context is required and no managed GC occurs.
The wrapper is native host-owned storage, outside VM heap accounting.

**Lifetime:** retain this handle and callback userdata until every channel/table
which references it is no longer in use. Register a copy of the table with a
context if StopSounds control is wanted. A temporary table can disappear after
channel creation; the provider itself cannot. Use the owning host/VM thread;
there are no provider locks or automatic worker threads. Do not share the same
provider across concurrently executing contexts or close it from its callbacks.

## Output callback semantics

The write callback validates a complete AUDIO frame and requires the same sample
rate/channels configured at open. Invalid metadata returns INVALID_ARGUMENT
without touching the endpoint. Native-endian S16 representation and accessible
buffer bounds use the common media validator. The callback consumes/copies no
more than the declared sample frames, including any extra accessible padding.

After a stop/drop it prepares the stream before writing. Positive short writes
advance within the borrowed frame. Zero progress, EAGAIN, underrun or another
native write error returns ERROR after a best-effort drop of queued samples;
there is no wait, hidden retry timer, drain, recovery loop or duplicate replay.
The next write prepares again. A fully accepted short frame is explicitly
started if ALSA is still PREPARED, rather than waiting indefinitely for a full
buffer threshold. Failed start also drops pending output and returns ERROR.
Audio already physically played before a partial failure cannot be undone;
hardware errors can also make the best-effort drop fail. The host must still
perform terminal provider cleanup and choose its own retry/clock policy.

The stop callback drops pending samples immediately and retains the device.
If ALSA already reports SETUP after a previous drop, it acknowledges the stopped
state without redundant drop. Successful stop returns OK; failure returns ERROR.
Subsequent writes reprepare. No buffering/mixing/scheduling is added by FlashVM.
The channel active flag remains an acknowledged API state, not an ALSA completion
probe; natural playback completion does not automatically clear it.

[GC:] There are no managed references or collector roots in the provider. When
called through a registered channel, the ordinary channel native barrier applies;
standalone output calls remain the host's explicitly synchronized responsibility.

## fvm_audio_linux_alsa_close

**Parameters:** a live provider handle or null. Close all referencing channels
first, or destroy their contexts while keeping provider userdata alive. Then
call provider close last. Stale handles and concurrent use are invalid.

**Results:** null returns OK in every build. For a live native handle, it performs
a best-effort drop, calls native close, releases the wrapper and returns OK or
ERROR according to native close status. The wrapper is invalidated even when
ERROR is returned: discard the handle, do not retry close on that pointer.
The final drop status is not returned separately. A disabled build returns
UNSUPPORTED for non-null input (it cannot have created a valid provider).
There is no managed GC, VM callback or userdata transfer during provider cleanup.

## Русский контракт

`fvm_audio_linux_alsa_open(device, rate, channels, latency_us, out, output)`
явно открывает названный ALSA PCM в nonblocking playback-режиме и настраивает
native-endian S16 interleaved. Имена/частота/каналы/латентность не выбираются
скрыто; числовые значения должны быть ненулевыми, указатели назначения обязательны.
Успех публикует host-owned provider и таблицу callbacks. Ошибки очищают оба
доступных назначения: INVALID_ARGUMENT для параметров, OUT_OF_MEMORY для wrapper,
UNSUPPORTED для выключенного переходника, ERROR для настоящей ошибки ALSA.
Латентность задаётся в микросекундах. Soft-resampling ALSA выключен; плагины,
выбранные пользователем, могут отдельно преобразовывать формат.

Write принимает только совпадающие rate/channels и полный безопасный S16-буфер.
После drop выполняет prepare, продвигается по положительным short writes,
при EAGAIN/ошибке/нулевом прогрессе сбрасывает очередь и возвращает ERROR без
скрытого ожидания. Короткий успешно принятый кадр явно запускается через start.
Уже воспроизведённые сэмплы при частичной ошибке вернуть невозможно; hardware
ошибка может помешать даже drop. Stop сохраняет устройство, отбрасывает очередь
и допускает последующий prepare/write. Естественное завершение звука само по
себе не выключает логический active регистрации.

Provider не принадлежит VM/GC. Его userdata должно жить до окончания всех
channel callbacks: сначала close каналов или уничтожение контекста, только потом
[`fvm_audio_linux_alsa_close`](../include/flashvm/multimedia.h#L78). Последний принимает null; ненулевой handle становится
недействительным даже при ERROR native close, повторять close старого указателя
нельзя. Все операции на одном host/VM-потоке, без одновременного доступа из разных
VM. Параметры/статусы/lifetime выше одинаковы для английского и русского контрактов.
Требования SDK, устройств, разрешений и плагинов настраиваются пользователем явно.

## Validation checkpoint v0.547

Linux native tests: five passed with actual static ALSA 1.2.12; the same five
passed with ASAN/UBSAN on FlashVM code. The external ALSA archive was not built
with sanitizers; LeakSanitizer is disabled for this runner. Default/disabled
full suite: 1136 passed plus one existing named Unix endpoint skip (1137 total).
The real null-PCM tutorial completed with EOF input; ELF dependencies contain
no dynamic libasound. Missing static SDK configuration was explicitly rejected.
All 89 catalog entries and 115 bridge/tutorial source files passed documentation
checks. No physical sound-card or foreign native OS playback is claimed.

## Tutorial and evidence

[alsa_output](../examples/multimedia/alsa_output/README.md) is an executable,
every-line-commented C89 tutorial. It opens only an explicitly supplied endpoint,
submits a short generated tone and waits for Enter before AVM1 StopSounds.
No-argument usage opens nothing; physical audio is an explicit side effect.

The native automated fixture uses real static ALSA 1.2.12 with its built-in null
PCM and explicit CTest configuration. It validates open/configure/write/start,
drop/reprepare/rewrite, VM StopSounds and close, plus invalid device/geometry.
A separate linker-wrapped fixture injects partial write/EAGAIN/zero progress and
start failure while retaining the real null device. These tests are separate
from physical audible playback, desktop sound-server setup and foreign OS runs.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AlsaOutput` | struct | [src/multimedia/audio/linux_alsa.hpp:7](../src/multimedia/audio/linux_alsa.hpp#L7) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:81](../include/flashvm/multimedia.h#L81) |
| `fvm_audio_linux_alsa_close` | function | [include/flashvm/multimedia.h:78](../include/flashvm/multimedia.h#L78) |
| `fvm_audio_linux_alsa_open` | function | [include/flashvm/multimedia.h:76](../include/flashvm/multimedia.h#L76) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:67](../include/flashvm/multimedia.h#L67) |
<!-- source-links:end -->
