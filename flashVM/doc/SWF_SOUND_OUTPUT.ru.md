# Очередь PCM и часы потребления

v0.581: явный host-мост между [микшером StartSound](SWF_SOUND_MIXER.ru.md),
[преобразованием частоты/каналов](SWF_SOUND_CONVERSION.ru.md) и сменной FIFO
аудиовывода. [EN](SWF_SOUND_OUTPUT.md). [Учебник C89](../examples/multimedia/swf_audio_queue/README.md).

## Владение и выполнение

[`FVM_SWFSoundOutput`](../src/swf/sound_output.cpp#L11) владеет одним буфером подготовленного PCM. Микшер заимствован;
[`FVM_AudioQueueBackend`](../include/flashvm/swf_sound_output.h#L16) копируется, но userdata/устройство принадлежат host.
Закрывать producer раньше микшера и устройства; userdata живёт до последнего
вызова. Управляемых указателей/context нет: подготовленные голоса переживают VM.
Для этих native handles GC pin не нужен; MovieClip требует обычных
[корней владения](MOVIECLIP_SOUND.ru.md).

Все операции producer и микшера сериализуются одним host-потоком либо общим
внешним mutex для всех участвующих потоков. Native worker может pump подготовленный
PCM под этим mutex; submit из context остаётся в его владеющем потоке под тем же
mutex. Producer эксклюзивно читает микшер и управляет write/pause/clear очереди.
До close нужно остановить и join worker. Backend callback выполняется в вызывающем
потоке и должен поддерживать выбранную host-политику.
Посторонние записи/очистка, чтение микшера или seek нарушают учёт.
Callback не должен самостоятельно менять заимствованный микшер. Повторный вход в
pump/info/pause/stop возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); close из callback
игнорируется, предотвращая освобождение выполняющегося вызова. Это не mutex.
Закрывать снаружи callback.

## Контракт backend

`struct_size` = sizeof полной таблицы, `abi_version` =
[`FVM_AUDIO_QUEUE_ABI_VERSION`](../include/flashvm/swf_sound_output.h#L10) (1). `sample_rate` и `channels` точно совпадают с
микшером: 5512–192000 frames/s, 1–2 канала. `userdata` — заимствованный opaque
указатель. PCM — native-endian interleaved signed short. Каждый frame содержит
channels значений; счётчики и часы выражены во **frames**, не байтах.

| Callback | Обязанности |
| --- | --- |
| `write(userdata, pcm, frames)` | [`FVM_OK`](../include/flashvm/flashvm.h#L42) означает копирование/приём ВСЕХ frames до возврата, без сохранения pcm. При любой ошибке/исключении не принять НИ ОДНОГО. Частичные записи разрешает сам мост: ошибка после частичного приёма приведёт к дублированию при повторе. |
| `queued(userdata, out_frames)` | При успехе вернуть полные непотреблённые frames, включая паузу. Потерю устройства сообщать ошибкой, а не ложным нулём. |
| `pause(userdata, paused)` | Атомарно установить 1 (пауза) или 0 (воспроизведение), сохранив PCM. Ошибка/исключение сохраняет прежнее состояние. |
| `clear(userdata)` | Успех удаляет всю очередь; ошибка/исключение сохраняет её. Устройство остаётся открытым. |

Ошибки [`FVM_Result`](../include/flashvm/flashvm.h#L40) передаются без изменения. C++ bad_alloc преобразуется в
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), остальные исключения — в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Атомарность обязательна
и при исключениях. Callback должен завершаться быстро: отмены заблокированного
системного вызова здесь нет. Если системный API не обеспечивает этот контракт,
мосту необходим собственный промежуточный transactional буфер.

## Создание

`fvm_swf_sound_output_create(mixer, backend, block_frames, target_frames, out)`
выделяет producer и block_frames * channels значений PCM. При ошибке *out = null;
out обязателен. Требуются микшер, таблица, все четыре callback, точные ABI и
геометрия. block: 1..[`FVM_SWF_SOUND_OUTPUT_MAX_BLOCK`](../include/flashvm/swf_sound_output.h#L11) (65536),
block <= target <= [`FVM_SWF_SOUND_OUTPUT_MAX_QUEUE`](../include/flashvm/swf_sound_output.h#L12) (262144).
Неверные параметры: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); нехватка памяти:
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); DBL_MANT_DIG < [`FVM_SWF_SOUND_OUTPUT_CLOCK_BITS`](../include/flashvm/swf_sound_output.h#L13) (53): [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).

После выделения памяти вызывает pause(1), затем queued: очередь обязана быть
пустой. Непустая очередь даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), её PCM не удаляется.
Успешная пауза сохраняется, даже если последующий query завершился ошибкой или
нашёл данные. Ошибки backend передаются вызывающему. Успех публикует producer на
паузе с нулевой эпохой часов, без чтения голосов. Владение зависимостями не меняется.

## Pump и повтор

`fvm_swf_sound_output_pump(output, accepted_frames)` проверяет FIFO и обрабатывает
не более одного блока. Необязательный счётчик результата сначала обнуляется.
Null/reentry: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Невозможный рост FIFO, регресс часов или
превышение [`FVM_SWF_SOUND_OUTPUT_MAX_CLOCK`](../include/flashvm/swf_sound_output.h#L14) (2^53−1 принятых frames на эпоху):
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Ошибки query/write передаются без изменения.

Если target заполнен либо недостаточно места для целого уже подготовленного
блока, возвращает [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47) без чтения/записи. Иначе готовит до
min(block, target−queued) frames, включая тишину после EOF голосов, затем пишет.
Успех возвращает принятое число frames. Ошибка write оставляет **тот же PCM**
для следующего явного pump: микшер уже продвинулся, повторно читать его нельзя.
Автоматических повторов/отката нет. Stop/close удаляют pending PCM. Stop по ID
влияет на будущий mix, но не на уже подготовленные/принятые samples; для их
удаления целиком нужен output stop.

Буфер producer выделяется при create. Mixer read/учёт не выделяют память, однако
законченные голоса могут освобождаться, а системный write может выделять память.
Работа ограничена block и числом голосов; realtime-safety не обещается.

## Пауза и часы

`fvm_swf_sound_output_pause(output, paused)` требует живой незанятый output и
0/1; иначе [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Состояние кэша меняется только после успешного
callback. FIFO/pending сохраняются. Пауза не подготавливает очередь: сначала
заполнить её pump на паузе, затем явно разрешить потребление. Ошибки передаются.

[`FVM_SWFSoundOutputInfo`](../include/flashvm/swf_sound_output.h#L34) содержит копию полей часов.
`fvm_swf_sound_output_info(output, info)` требует ненулевые указатели, точный
info.struct_size = sizeof(*info), отсутствие reentry. Нарушение:
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). После успешного query копирует всю структуру; при ошибке
выходная структура вызывающего не меняется. Ошибки backend передаются;
невозможный рост FIFO/регресс часов дают [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

| Поле | Смысл |
| --- | --- |
| `struct_size` | Точный размер ABI структуры. |
| `submitted_frames` | Все принятые frames с успешного stop/reset, включая тишину; точность до 2^53−1. |
| `queued_frames` | Последний проверенный остаток FIFO. После pump — верхняя оценка до нового query, поскольку consumer работает параллельно. |
| `played_frames` | submitted минус queued: наблюдаемые монотонные часы извлечения из FIFO. |
| `seconds` | played_frames / sample_rate выхода. |
| `pending_frames` | Подготовленный блок, не принятый после ошибки write; не входит в submitted/played. |
| `paused` | Последнее успешно установленное producer состояние паузы. |
| `starved` | Нет паузы, ранее данные принимались, теперь FIFO пуста; длительность underrun не восстанавливается. |

Это часы **программной FIFO, не ЦАП и не точного момента звучания**. Драйвер и
железо добавляют задержку. При голодании часы замирают до нового потребления;
API не подставляет тишину по wall clock, не пропускает исходные samples и не
догоняет timeline автоматически. Политику задержки/восстановления выбирает host.
Desktop frontend пока автоматически к этим часам не подключён.

## Stop и close

`fvm_swf_sound_output_stop(output)`: null/reentry дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).
Сначала pause(1); ошибка сохраняет прежнее состояние. Успешная пауза фиксируется,
далее clear. При ошибке clear producer остаётся на паузе, pending/голоса/счётчики
сохраняются для восстановления. Успех останавливает все голоса, удаляет pending,
обнуляет часы и оставляет паузу. Исключения преобразуются по общему правилу.
Host должен перебазировать video/timeline после новой эпохи. Seek MovieClip нет.

`fvm_swf_sound_output_close(output)` принимает null, возвращает void. Освобождает
только producer/pending: не вызывает callback, pause, clear, mixer stop/close или
device close. Поэтому допускает освобождение после потери устройства. Для общей
остановки сначала stop, затем close producer, mixer, device. Reentrant close
игнорируется и не создаёт отложенного освобождения.

## SDL2

`fvm_audio_sdl_queue_open(rate, channels, device_buffer_frames, out, backend)`
открывает настоящее default playback устройство SDL2 на Linux/Windows/macOS.
Оба выходных указателя обязательны; handle/таблица очищаются до проверки.
Частота 5512–192000 помещается в int; каналы 1..2; device buffer — степень двух
в [`FVM_SDL_AUDIO_MIN_BUFFER`](../include/flashvm/swf_sound_output.h#L102)..[`FVM_SDL_AUDIO_MAX_BUFFER`](../include/flashvm/swf_sound_output.h#L103) (64..32768 frames).
Неверные параметры: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); не 8-bit byte/16-bit short либо
неожиданный полученный формат: [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46); ошибка init/open:
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43); нехватка памяти handle: [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44). Частичные ресурсы
освобождаются. Успех публикует точную native S16 геометрию, устройство на паузе и
заимствованную таблицу callback.

Используются SDL_InitSubSystem, SDL_OpenAudioDevice, SDL_QueueAudio,
SDL_GetQueuedAudioSize, SDL_PauseAudioDevice и SDL_ClearQueuedAudio;
[операции разделены по файлам](../src/multimedia/audio/sdl_queue_open.cpp).
STOPPED сообщает ошибку, а не завершённое воспроизведение. Успешный QueueAudio
копирует весь блок. Часы SDL остаются программными, не аппаратными.
Захвата/микрофона нет.

[`FVM_SDLAudioQueue`](../src/multimedia/audio/sdl_queue.hpp#L11) владеет native SDL устройством.
`fvm_audio_sdl_queue_close(device)`: null даёт [`FVM_OK`](../include/flashvm/flashvm.h#L42); живой handle закрывает
устройство и балансирует ссылку SDL audio subsystem. Сначала закрыть producer;
не вызывать SDL_Quit при живом устройстве и не использовать старую таблицу после
close. Nonnull в сборке без SDK даёт [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) (получить валидный handle
в такой сборке невозможно).


Ошибка SDL QueueAudio фиксирует неисправность адаптера: query/write/resume дают
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43) до успешного stop/clear. Пауза остаётся доступной. Это исключает
использование недостоверных FIFO-счётчиков SDK после ошибки выделения памяти.
Producer сохраняет pending, но этот адаптер требует reset вместо обычного повтора.
Close остаётся доступным.

По умолчанию зависимости от SDK нет: корректный open даёт [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) с
очищенными выходами. Явное включение:

```sh
cmake -S flashVM -B /tmp/flashvm-audio -DFVM_ENABLE_SDL_AUDIO=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build /tmp/flashvm-audio --target flashvm_examples_multimedia_swf_audio_queue_main
/tmp/flashvm-audio/flashvm_examples_multimedia_swf_audio_queue_main --sdl
```

Нужен установленный SDL2 >=2.0.9 с CMake CONFIG/SDL2::SDL2; для отдельного SDK
задать SDL2_DIR. [Подготовка ОС](../examples/multimedia/swf_audio_queue/README.md#native-preparation).
`FVM_SDL_AUDIO_DEVICE_TESTS=ON` явно добавляет SDK protocol test с
audio driver dummy. Это не проверка слышимого вывода. Нативная приёмка остальных
ОС/Android, автоматическая синхронизация фильма и длительная работа с underrun
остаются открытыми. Существующие FFmpeg decoder-мосты независимы от этого выхода.

## Фактический блок устройства (v0.583)

`fvm_audio_sdl_queue_info(device, info)` копирует неизменяемую геометрию SDL в
[`FVM_SDLAudioQueueInfo`](../include/flashvm/swf_sound_output.h#L111): `sample_rate` в Гц, `channels`, `device_buffer_frames` —
число interleaved frames в native-блоке. Установить `struct_size` равным
`sizeof(FVM_SDLAudioQueueInfo)`. На Android фактический блок может превышать
запрошенный: запас очереди и порог resume рассчитываются по этому значению.
Это не текущая очередь и не измеренная задержка аппаратуры.

Требуются живой device, ненулевой выход и точный размер структуры. Неверные
указатели/размер дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), не меняя выход. Успех — [`FVM_OK`](../include/flashvm/flashvm.h#L42),
без выделения памяти, VM/GC, callback и передачи владения. Неизменяемый снимок
можно читать в worker при гарантированном времени жизни устройства; close не
должен выполняться одновременно. Без SDL корректно оформленный вызов даёт
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46); живые устройства в такой сборке не создаются.

## Согласование блока в v0.586

[`fvm_audio_sdl_queue_open`](../include/flashvm/swf_sound_output.h#L106) разрешает только SDL_AUDIO_ALLOW_SAMPLES_CHANGE:
`FVM_SDLAudioQueueInfo.device_buffer_frames` показывает согласованный callback-блок,
а не скрытый conversion stream с запрошенным размером. Частота/каналы/native S16
на входе очереди сохраняются. Запрос ограничен степенями двойки, но полученный
ненулевой блок может быть больше и не обязан быть степенью двойки. Он нужен для
запаса/resume очереди, не для определения позиции ЦАП или физической задержки.
Требуется SDL SDK >=2.0.9; ABI структуры/очереди и владение не менялись. Явный
real-SDK тест оборачивает acquisition устройством на 8192 frames и проверяет,
что разрешено только согласование числа frames.

## Необязательное затухание v0.587

[Политика плеера, новые API и ограничения](PLAYER_AUDIO_RELEASE.ru.md). Прежняя немедленная остановка сохраняется по умолчанию.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AUDIO_QUEUE_ABI_VERSION` | macro / define | [include/flashvm/swf_sound_output.h:10](../include/flashvm/swf_sound_output.h#L10) |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:122](../include/flashvm/swf_sound_output.h#L122) |
| `fvm_audio_sdl_queue_info` | function | [include/flashvm/swf_sound_output.h:120](../include/flashvm/swf_sound_output.h#L120) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:106](../include/flashvm/swf_sound_output.h#L106) |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_SDL_AUDIO_MAX_BUFFER` | macro / define | [include/flashvm/swf_sound_output.h:103](../include/flashvm/swf_sound_output.h#L103) |
| `FVM_SDL_AUDIO_MIN_BUFFER` | macro / define | [include/flashvm/swf_sound_output.h:102](../include/flashvm/swf_sound_output.h#L102) |
| `FVM_SDLAudioQueue` | struct | [src/multimedia/audio/sdl_queue.hpp:11](../src/multimedia/audio/sdl_queue.hpp#L11) |
| `FVM_SDLAudioQueueInfo` | struct | [include/flashvm/swf_sound_output.h:111](../include/flashvm/swf_sound_output.h#L111) |
| `FVM_SWF_SOUND_OUTPUT_CLOCK_BITS` | macro / define | [include/flashvm/swf_sound_output.h:13](../include/flashvm/swf_sound_output.h#L13) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:99](../include/flashvm/swf_sound_output.h#L99) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:50](../include/flashvm/swf_sound_output.h#L50) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:88](../include/flashvm/swf_sound_output.h#L88) |
| `FVM_SWF_SOUND_OUTPUT_MAX_BLOCK` | macro / define | [include/flashvm/swf_sound_output.h:11](../include/flashvm/swf_sound_output.h#L11) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:86](../include/flashvm/swf_sound_output.h#L86) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:84](../include/flashvm/swf_sound_output.h#L84) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:90](../include/flashvm/swf_sound_output.h#L90) |
| `FVM_SWFSoundOutput` | struct | [src/swf/sound_output.cpp:11](../src/swf/sound_output.cpp#L11) |
| `FVM_SWFSoundOutputInfo` | struct | [include/flashvm/swf_sound_output.h:34](../include/flashvm/swf_sound_output.h#L34) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
