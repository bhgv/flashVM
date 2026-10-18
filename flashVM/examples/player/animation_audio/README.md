# Animation, input and event audio / Анимация, ввод и событийный звук

Три готовых SWF8 для ручной проверки настоящего плеера, без внешнего AS-компилятора.
**Это исполняемые SWF, не эскизы.** [make_movies.py](make_movies.py) подробно
показывает самостоятельную запись RECT, MATRIX, Shape3/Sprite, PCM/ADPCM и AVM1.
Пересоздание: `python3 make_movies.py` (Python 3, только стандартная библиотека).
Файлы перезаписываются рядом с генератором. Сам генератор устройств не открывает.

Three runnable 640×360, 24 fps, 120-frame SWF8 movies. All loop every five seconds
in the default single-file player. Input capture uses the documented **HostInput
player extension**, not a claim of complete Flash Key/Mouse/button compatibility.
[Full EN guide](../../../doc/PLAYER_TEST_BUILDS.md) /
[RU](../../../doc/PLAYER_TEST_BUILDS.ru.md).

v0.583: PCM подаётся отдельным worker с запасом по реальному блоку устройства;
[причина изменения и ограничения](../../../doc/PLAYER_TEST_BUILDS.ru.md#исправление-звука-android-в-v0583).

## Tests / Тесты

| File | What to check / Что проверять |
| --- | --- |
| [01-animation-audio.swf](01-animation-audio.swf) | Оранжевый квадрат движется вправо и по синусоиде; синий движется навстречу. Раз в пять секунд начинается новая последовательность трёх тонов. Не требует ввода. |
| [02-input-audio.swf](02-input-audio.swf) | Оранжевая анимация продолжается. Стрелки двигают синий квадрат; Space увеличивает его по X, отпускание восстанавливает размер. Розовый квадрат следует мыши/основному касанию и расширяется при удержании кнопки. Зелёный индикатор меняет X по коду последней клавиши. Жёлтый с каждым кликом смещается на 20 px. |
| [03-stop-sounds.swf](03-stop-sounds.swf) | Начальный 440 Hz тон отменяется в том же кадре; loop начинается на кадре 24 и обрывается на 30. Поздние stereo/ADPCM звучат как в 01; анимация продолжается. Проверяет StopSounds в interpreter/JIT. |

Кликните окно для фокуса клавиатуры. Зажмите стрелку примерно на полсекунды.
Короткие нажатия между кадрами сохраняются в счётчиках, но held-флаг может уже
успеть вернуться в ноль. Синий/индикаторы не ограничены краями: они могут уйти
за сцену; перезапустите тест для сброса. На Android касание заменяет основную
кнопку мыши; стрелкам нужна физическая/BT/USB клавиатура. Экранная клавиатура и
мультитач-жесты в этом тестовом APK не предоставляются.

At frame 0: quiet 440 Hz U8 mono PCM/11025 Hz. Frame 24: four loops; frame 26:
no-multiple does not add a duplicate voice; frame 36: stop-by-ID truncates the
loop. Frame 72: S16 little-endian stereo/44100 Hz, 660 Hz left and 990 Hz right.
Frame 96: four-bit SWF ADPCM mono/44100 Hz, approximately 880 Hz. Every source
has a short fade to reduce clicks. Resampling targets 48000 Hz stereo. On
headphones the stereo tone should differ between channels. Loop transitions
and queue latency are not sample-accurate synchronization acceptance.

## Launch / Запуск

Из распакованного Linux-пакета / from the Linux binary package:

```sh
chmod +x flashvm_player run-interpreter.sh run-jit.sh
./run-interpreter.sh
./run-jit.sh
./flashvm_player --no-jit --stats tests/01-animation-audio.swf
./flashvm_player --jit --stats tests/02-input-audio.swf
./flashvm_player --jit --mute --frames 120 --stats tests/02-input-audio.swf
```

Windows: двойной клик `run-interpreter.bat` / `run-jit.bat`; окно консоли сохраняет
ошибки после выхода. Или из папки пакета:

```bat
flashvm_player.exe --no-jit --stats tests\01-animation-audio.swf
flashvm_player.exe --jit --stats tests\02-input-audio.swf
```

Android: установить APK, выбрать один из шести пунктов (три теста × два режима).
Mute отключает открытие устройства. Back закрывает текущий тест. Возврат из фона
возобновляет сцену; удерживаемые кнопки сбрасываются при уходе в фон.
Используется приватная копия встроенного SWF: разрешения storage/mic/network не нужны.

## Implementation and limits / Реализация и ограничения

Каждая управляемая фигура обёрнута в DefineSprite: GetProperty/SetProperty
применяются к MovieClip, а не к сырому shape instance. Имена pointer/box/key/click
разрешаются обычным AVM1 lookup. Снимок HostInput публикуется перед advance;
AVM1 выполняет перемещение внутри interpreter/JIT. Движение оранжевого квадрата
записано timeline-тегами; его наличие само по себе не доказывает исполнение AS.
Исполняемые тела содержат AVM1, а `--stats` показывает фактические native entries.

The host owns window/input snapshots and audio queue separately from the VM.
Input and movie roots are pinned until cleanup. StartSound callbacks prepare
native mixer voices; PCM is copied into SDL, then producer/mixer/device are
closed before SDL shutdown. No tutorial assertions or hidden file/device writes
exist in the generator. The real player explicitly opens playback on launch;
`--mute` avoids it. Errors are visible, not converted to fake successful playback.

Здесь проверяются root StartSound, loops/no-multiple/stop-by-ID, PCM8/PCM16/ADPCM,
а также StopSounds в третьем тесте; MP3/AAC/video, sprite sound и AS Sound
не поддерживаются этим примером. [Контракт StopSounds](../../../doc/AUDIO_STOP_BINDINGS.ru.md). Видео-такты и PCM device clock независимы;
изменение --speed/--fps не меняет высоту звуков. Качество/кодеки/фильтры, отсутствующие
в player-jit профиле, не входят в эти пакеты. JIT может иметь fallback на других
роликах; наличие --jit не означает полную native-компиляцию любого SWF.

## Demonstrated API / Используемые API

The generator directly writes public SWF records/AVM1 bytes. The player exercises
these individual C APIs; ownership and error contracts are in the linked guides.

| API | Behavior / Где читать |
| --- | --- |
| [`fvm_movieclip_advance`](../../../include/flashvm/movieclip.h#L54) | Выполняет очередной реальный кадр и его AVM1; [desktop host](../../../doc/DESKTOP_FRONTEND.md). |
| [`fvm_movieclip_render_view`](../../../include/flashvm/movieclip.h#L59) | Растеризует сцену с отдельной viewport-проекцией; [render contract](../../../doc/DESKTOP_FRONTEND.md#new-public-vm-api). |
| [`fvm_input_set_pointer`](../../../include/flashvm/input.h#L72) | Публикует scene/world координаты без callback; [input](../../../doc/INPUT_POINTER.md). |
| [`fvm_set_global`](../../../include/flashvm/value.h#L87) | Привязывает HostInput в context; [API index](../../../doc/API_INDEX.md). |
| [`fvm_object_set`](../../../include/flashvm/value.h#L74) | Обновляет отдельные scalar-поля host-снимка перед кадром; [API index](../../../doc/API_INDEX.md). |
| [`fvm_movieclip_set_sound_callback`](../../../include/flashvm/movieclip.h#L21) | Синхронно доставляет настоящий root StartSound до DoAction; [contract](../../../doc/MOVIECLIP_SOUND.md). |
| [`fvm_swf_sound_mixer_submit`](../../../include/flashvm/swf_sound_mixer.h#L48) | Готовит независимый native voice или применяет stop/no-multiple; [mixer](../../../doc/SWF_SOUND_MIXER.md). |
| [`fvm_swf_sound_mixer_create_configured`](../../../include/flashvm/swf_sound_mixer.h#L37) | Выделяет bounded mixer и явно выбирает 48 kHz stereo conversion; [conversion](../../../doc/SWF_SOUND_CONVERSION.md). |
| [`fvm_swf_sound_output_pump`](../../../include/flashvm/swf_sound_output.h#L52) | Готовит/повторяет один PCM-блок; [output](../../../doc/SWF_SOUND_OUTPUT.md#pump-and-retry). |
| [`fvm_swf_sound_output_pause`](../../../include/flashvm/swf_sound_output.h#L54) | Управляет потреблением при воспроизведении/Android background; [pause](../../../doc/SWF_SOUND_OUTPUT.md#pause-and-copied-clock). |
| [`fvm_swf_sound_output_stop`](../../../include/flashvm/swf_sound_output.h#L58) | Удаляет очередь, pending и голоса перед закрытием; [stop](../../../doc/SWF_SOUND_OUTPUT.md#stop-and-close). |
| [`fvm_audio_sdl_queue_open`](../../../include/flashvm/swf_sound_output.h#L68) | Открывает настоящий output; host владеет device, SDK failure передаётся наружу; [SDL](../../../doc/SWF_SOUND_OUTPUT.md#sdl2-native-bridge). |
| [`fvm_audio_sdl_queue_close`](../../../include/flashvm/swf_sound_output.h#L83) | Закрывает device до SDL_Quit; [cleanup](../../../doc/SWF_SOUND_OUTPUT.md#sdl2-native-bridge). |
| [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L64) | Выбирает interpreter/automatic JIT с явной проверкой доступности; [execution](../../../doc/API_INDEX.md). |
| [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h#L71) | Возвращает фактические execution counters, не состояние CLI-флага; [API index](../../../doc/API_INDEX.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) / [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Закрепляют/освобождают managed root и HostInput на срок host-владения; [GC](../../../doc/GC_NATIVE_ROOTS.md). |

[Каталог](../../README.md#example-player-animation_audio).


v0.584: [PCM/WAV диагностика и отправка ZIP с Android](../../../doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:83](../../../include/flashvm/swf_sound_output.h#L83) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:68](../../../include/flashvm/swf_sound_output.h#L68) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../../../include/flashvm/execution_mode.h#L71) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../../../include/flashvm/execution_mode.h#L64) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../../../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:54](../../../include/flashvm/movieclip.h#L54) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:59](../../../include/flashvm/movieclip.h#L59) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../../../include/flashvm/movieclip.h#L21) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_set_global` | function | [include/flashvm/value.h:87](../../../include/flashvm/value.h#L87) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:37](../../../include/flashvm/swf_sound_mixer.h#L37) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:48](../../../include/flashvm/swf_sound_mixer.h#L48) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:54](../../../include/flashvm/swf_sound_output.h#L54) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:52](../../../include/flashvm/swf_sound_output.h#L52) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:58](../../../include/flashvm/swf_sound_output.h#L58) |
<!-- source-links:end -->
