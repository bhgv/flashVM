# Последовательность root PCM-заголовков в SDL-плеере

С v0.758 реальный desktop player принимает один проверенный сегмент SoundStreamHead/Head2
формата PCM_NATIVE или PCM_LE без корневых StartSound. Источник преобразуется в 48000 Гц
stereo S16 и передаётся настоящей SDL-очереди. Другие кодеки, несколько заголовков и смесь
event/stream явно отклоняются при preflight до окна/устройства. --mute по-прежнему
проверяет и пропускает root stream без аудиоустройства. Строгий импорт core не изменён:
плеер отдельно готовит snapshot того же источника и явно отключает stream-контролы в
модели сцены.

## Последовательность заголовков (v0.761)

Плеер принимает до [FVM_DESKTOP_AUDIO_MAX_SEGMENTS](../frontend/common/audio.h#L19) (64) проверенных PCM-заголовков,
по-прежнему без root StartSound. Каждый должен иметь PCM_NATIVE или PCM_LE;
сжатые кодеки, event/stream mixing и превышение лимита отклоняются до окна/device.
Mute проверяет произвольное число индексированных root segments без snapshots и
аудиоустройства. Этот предел ограничивает startup/seek, не размер пакета 9P и не
геометрию сетевого media channel.

Владелец выделяет обнулённый массив [FVM_DesktopStreamSegment](../frontend/common/audio.h#L22) и готовит snapshots/
converters до старта worker. Каждая запись владеет snapshot и отдельной phase grid.
При частичном отказе close сначала join worker, закрывает каждую ненулевую запись
один раз и освобождает массив; stream_source теперь только borrowed alias первой
записи. Snapshot не становится VM root и не передаётся в SDL device. Startup
проверяет/индексирует отдельно каждый сегмент и может выделять память; worker
переключается без выделений и не читает context.

При EOF источника [fvm_swf_stream_output_continue](../include/flashvm/swf_stream_output.h#L89) сохраняет принятые queue bytes,
pause и суммарные submitted/played clocks. Device открывается один раз. Выход
всегда 48000 Hz stereo S16, source width/mono/stereo/rate меняются по заголовкам.
Фаза переносится между блоками одного header и начинается с нуля у нового. Перед
активацией следующего его cursor сбрасывается к нулевому блоку: старые loops/seeks
не оставляют EOF. Переходы headers, empty/skipped markers и PCM preparation тратят
общий бюджет [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L17) на итерацию; цепочка 64 пустых headers не
становится неограниченным tight loop. stream_info показывает общий EOF только
для исчерпанного последнего header; FIFO drain и StopSounds cancellation отдельны.

Explicit/backward/skipped навигация выбирает последний header не позже target
frame (при равных frames — последний в source order), а до первого header —
первый будущий. [fvm_swf_stream_output_bind_frame](../include/flashvm/swf_stream_output.h#L96) выполняет staged pause/clear и
публикует binding только после подтверждённого clear, выбирая первый блок не
раньше target внутри сегмента. Gaps удерживают будущие данные, тишина не
синтезируется. Следующие cursors заново подготавливаются при достижении. Это
детерминированная prefix-discard политика, не полная Flash header/seek semantics,
drift correction и не декодирование сжатого stream.

[stream_segments.py](../frontend/tests/stream_segments.py) независимо строит FWS/CWS: U8 mono 11025, пустой header,
S16 stereo 22050 и S16 mono 44100, реальные frame actions, gaps, empty/short block.
Каждый принятый sample сравнивается с целочисленным рациональным oracle по headers,
включая второй настоящий loop MovieClip, ровно три transitions за проход и
отсутствие промежуточного clear/reopen. Готовый pcm-segments.swf меняет формат в
середине; проверены все 96000 output frames. [stream_worker.cpp](../frontend/tests/stream_worker.cpp) повторяет реальные
stop/play/GC/StopSounds, backward/same-frame navigation и forward cross-header seek;
preflight принимает 64 headers и отклоняет 65 до native setup. Dummy не доказывает
слышимое устройство, другую ОС или CPU. Старые counts ниже относятся только к
v0.759, а не к новому прогону v0.761.

Fault sweep [stream_prepare_failure.cpp](../frontend/tests/stream_prepare_failure.cpp) проверяет cleanup частичной подготовки, в том числе с несколькими готовыми
snapshots; ASan exception sweep запускает отдельный процесс для каждого allocation budget
из-за ограничений этой среды.

## Сборка и запуск

Нужна самостоятельная сборка FlashVM со статическим SDL2 SDK и графическим профилем из
[руководства сборки](PLAYER_TEST_BUILDS.ru.md), с FVM_ENABLE_SDL_AUDIO и FVM_BUILD_DESKTOP_FRONTEND. Core без плеера по-прежнему не
требует SDL. После сборки flashvm_player:

```sh
python3 flashVM/examples/player/pcm_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --once flashVM/examples/player/pcm_stream/pcm-stream.swf
build/player/frontend/flashvm_player --jit --once flashVM/examples/player/pcm_stream/pcm-stream.swf
```

JIT используйте только при наличии backend. [Воспроизводимый пример](../examples/player/pcm_stream/README.ru.md) меняет цвет и частоту тона через секунду.
Это исполняемый SWF с автономным Python-генератором, не компилятор. Структурированные [fvm_desktop_run](../frontend/common/player.h#L39)
и [fvm_desktop_run_memory](../frontend/common/player.h#L47) используют тот же путь; CLI-only состояния и новых ABI-полей настроек нет. Source
base и политика загрузки прежние. Replace/unload используют существующую строгую политику;
child/sprite streams не поддержаны.

## Потоки и владение

Владелец импортирует/укореняет root и читает [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83) в точках обслуживания. Выбранный, но ещё
не выполненный кадр не разрешает звук. Только скаляры entered frame и play передаются под
audio mutex; worker не читает context, clip и managed values. Он владеет независимым
snapshot, converter cursor и retry buffer и делает не больше [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L17) вызовов [fvm_swf_stream_output_pump_until](../include/flashvm/swf_stream_output.h#L64) за итерацию.
Backend SDL копирует принятый PCM и не входит в VM/GC. Reserve учитывает реальный quantum
драйвера. Короткий разрешённый блок/EOF запускает непустую очередь даже ниже обычного
порога, чтобы ожидание reserve не блокировало старт.

Пауза хоста/сессии/background и stop клипа останавливают очередь, сохраняя данные; play
возобновляет разрешённые данные. StopSounds очищает очередь и отменяет stream до
обнаруженной навигации. Release ramp для stream пока нет. Pending/backward/skipped переход
вызывает [fvm_swf_stream_output_seek_frame](../include/flashvm/swf_stream_output.h#L83) под тем же mutex: старые queued/pending bytes удаляются, phase=0 и выбирается
первый блок не раньше выбранного кадра. Пропуск кадра удерживает более поздний блок,
тишина не синтезируется. EOF источника отличается от опустевшей очереди.

С v0.759 владелец также читает [fvm_movieclip_get_navigation](../include/flashvm/movieclip.h#L93). Изменившийся token вызывает clear/seek, даже если goto
выбрал и выполнил тот же кадр целиком между наблюдениями. nextFrame/prevFrame также меняют
token; обычные ticks/play/stop — нет. Pending/backward/skipped проверки сохранены, в том
числе естественный loop rewind. Это всё ещё не полный журнал: промежуточные цели и эффекты
не переигрываются. Drift/drop/stretch, stream+event mixing,
MP3/ADPCM stream decoder, синхронизация видео и эталонная Flash-семантика seek остаются
открытыми. Root unload отсоединяет доставку, дожидается worker и освобождает
snapshot/queue/device до уничтожения context. GC выполняется только владельцем в штатных
safe points.

## Нативный идентификатор навигации (v0.759)

`fvm_movieclip_get_navigation(context, clip, &token)` копирует unsigned long token. Null output, неверное/чужое/prototype состояние и
GC/teardown дают INVALID_ARGUMENT, сохраняя выход. Ограничения
owner-thread/same-context/retained clip как у [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L83). Нет выделений, getters, callbacks,
продвижения или managed pointers наружу. Сравнивайте только равенство для того же
удерживаемого клипа/content. Новый или старый граф без optional private record возвращает
ноль. Проверенный explicit navigation увеличивает token, включая goto на тот же кадр; [FVM_MOVIECLIP_NAVIGATION_MAX](../include/flashvm/movieclip.h#L85)
переходит в единицу. Числовая запись передаётся как обычное поле графа, но transfer/reload
не делают её глобальной идентичностью. Content lifetime/transport epoch остаются
отдельными. MAX переходов между наблюдениями допускают wrap; это не монотонные часы и не
журнал навигации.

Первая навигация лениво выделяет private numeric slot: старые графы принимаются,
создание/обычные ticks не получают новых выделений. Ошибка до публикации сохраняет token.
Поздняя ошибка/выделение в seek может сохранить уже опубликованный token, как прочие
частичные эффекты seek; безопасная реакция хоста — очистить старый PCM. Неизвестные
labels/неверные targets token не публикуют. [movieclip_playback_info.cpp](../tests/movieclip_playback_info.cpp) проверяет initial/legacy zero, чтение без
выделений, lookup error, next/previous, wrap, атомарность ошибок и OOM. [stream_worker.cpp](../frontend/tests/stream_worker.cpp) доказывает
очистку при goto+реальном входе между наблюдениями даже с неизменным номером кадра.

## Внутренний C API хоста

[Объявления audio.h](../frontend/common/audio.h), [реализация audio.c](../frontend/common/audio.c). Это приватные frontend helpers, не новый стабильный ABI плагинов; сигнатуры
публичных C-функций запуска плеера сохранены.

| Операция | Контракт |
| --- | --- |
| [fvm_desktop_audio_validate](../frontend/common/audio.h#L71) | Owner-thread проверка количества, формата и event-политики. OK без stream, UNSUPPORTED для неподдержанных сочетаний, ошибки структуры распространяются. Нет native device или полученного владения. |
| [fvm_desktop_audio_open_paused](../frontend/common/audio.h#L80) | Получает snapshot/converter/FIFO/device/mutex/worker и stop binding при наличии звука; pause только 0/1. После любой частичной ошибки вызывайте close. Snapshot независим от VM, но stop binding отсоединяется до context destroy. |
| [fvm_desktop_audio_timeline](../frontend/common/audio.h#L76) | Владелец читает playback/navigation token удерживаемого клипа своего контекста и публикует скаляры под mutex. Ошибки query/seek/backend возвращаются; pause/clear имеют staged effects без rollback. Worker в VM не входит. |
| [fvm_desktop_audio_stream_info](../frontend/common/audio.h#L92) | Копирует stream-only [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L11) под mutex. Ошибки сохраняют дескриптор; возможны ожидание lock/backend. Это FIFO dequeue position, не DAC/movie sync. |
| [fvm_desktop_audio_pause](../frontend/common/audio.h#L87) / [fvm_desktop_audio_tick_paused](../frontend/common/audio.h#L85) | Независимая пауза хоста; нет очистки данных. Tick проверяет worker error latch. Хост вызывает последовательно; worker сериализован внутри. |
| [fvm_desktop_audio_clear](../frontend/common/audio.h#L95) / [fvm_desktop_audio_close](../frontend/common/audio.h#L97) | Владелец отсоединяет binding/callback, join worker, закрывает snapshot и device, уничтожает mutex. Clear сохраняет счётчики. Context/userdata должны жить до detachment/quiescence. |

Регрессия [memory_movie.c](../frontend/tests/memory_movie.c) также запускает скопированный изменяемый `char buf[]` с PCM-фильмом через [fvm_desktop_run_memory](../frontend/common/player.h#L47),
намеренно задаёт не-файловую source label и проверяет неизменность заимствованных байтов.
При включённом SDL audio используется dummy device.

## Проверки и ограничения

[stream_worker.cpp](../frontend/tests/stream_worker.cpp) использует настоящий SDL dummy queue и укоренённый MovieClip: initial hold, gaps,
медленный владелец, stop/play, GC, EOF, StopSounds, backward seek, host pause и joined
cleanup, с выбором interpreter/JIT. [stream_audio.py](../frontend/tests/stream_audio.py) независимо строит FWS/CWS с AVM1 действиями и
сравнивает каждый принятый PCM sample реального CLI trace с 9600 точными stereo frames.
[stream_preflight.py](../frontend/tests/stream_preflight.py) проверяет поддержанные/неподдержанные/повреждённые/muted сочетания с отсутствующими
драйверами для отклонённых путей. Dummy не доказывает слышимый звук, другую ОС или CPU.
FVM_AUDIO_TRACE явно включает bounded RAM capture и запись файлов при close: это принятые
блоки, в том числе позже удалённые clear, не speaker playback. Все стадии roadmap остаются
открытыми.
Прежний audio_underruns считает наблюдения starvation event-producer; stream mode пока не
измеряет число/длительность underrun и не выводит поведение DAC из пустой очереди.

Проверка v0.759: пройдены 176 выбранных player/frontend/stream/MovieClip проверок
и 16 shared-host. Самостоятельный minimal profile проверяет playback observation
и реальные timeline gates без desktop frontend. ASan/UBSan инструментируют
изменённые native stream, MovieClip и frontend модули с обычными core/SDL
зависимостями; это не полная VM-инструментация и не leak acceptance. Строгие
C89/C++98 объекты собраны для ARM64 Linux, RISC-V64 Linux и Windows x64; foreign
executables/devices не запускались. Двухсекундный пример также независимо разобран:
все 96000 принятых stereo output frames совпадают с целочисленным oracle.

Проверка v0.761: пройдены 101 выбранная native player/frontend/stream проверка,
18 shared-host и три standalone minimal-profile output/API. Изменённые модули
проходят ASan/UBSan с обычными linked dependencies и выключенным leak detection
в этой ограниченной среде. Многократные exception injections в одном ASan process
здесь ненадёжно снимают stack poison; бюджеты 0–10 проверены каждый в новом процессе,
а обычный native CTest выполняет полный in-process fault sweep. C89/C++98 объекты
скомпилированы для ARM64 Linux, RISC-V64 Linux и Windows x64; это не полная foreign
сборка плеера, target SDK/runtime и не приёмка физического устройства.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_navigation` | function | [include/flashvm/movieclip.h:93](../include/flashvm/movieclip.h#L93) |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `FVM_MOVIECLIP_NAVIGATION_MAX` | macro / define | [include/flashvm/movieclip.h:85](../include/flashvm/movieclip.h#L85) |
| `fvm_swf_stream_output_bind_frame` | function | [include/flashvm/swf_stream_output.h:96](../include/flashvm/swf_stream_output.h#L96) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:89](../include/flashvm/swf_stream_output.h#L89) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:64](../include/flashvm/swf_stream_output.h#L64) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:83](../include/flashvm/swf_stream_output.h#L83) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:11](../include/flashvm/swf_stream_output.h#L11) |
<!-- source-links:end -->
