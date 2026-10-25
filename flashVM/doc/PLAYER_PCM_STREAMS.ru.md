# Последовательность root PCM-заголовков в SDL-плеере

Текущее gate/authored-Stop планирование: [контракт](MIXED_GATES.ru.md). Ограничения final-EOF-only/root-pause ниже относятся к исходному checkpoint v0.986 и заменены новым mixed scheduling.

Текущая root stream/event композиция и границы gate/pause/device: [контракт](DESKTOP_MIX.ru.md). Прежние versioned stream-only описания ниже исторические.
v0.819 проверяет CRC заголовка/side information MP3 и все статические корневые
MP3-ссылки до unmuted-аудиоустройства, один раз на ID. Stop-only ссылки и mute
не декодируют звук. [Допуск и ограничения](MP3_CRC.ru.md).


v0.780 добавляет поддержанный root MP3 stream/event decode с bounded native preparation и независимой проверкой PCM. Mixed root stream/event, negative latency и reference MP3 seek/A/V остаются открытыми. Free-format добавлен в v0.806–0.811; короткие события без достаточных заголовков остаются unsupported. [Контракт free-format](FREE_FORMAT_MP3.ru.md). [Текущий контракт](MP3_AUDIO.ru.md).

С v0.758 реальный desktop player принимает один проверенный сегмент SoundStreamHead/Head2
формата PCM_NATIVE или PCM_LE без корневых StartSound. Источник преобразуется в 48000 Гц
stereo S16 и передаётся настоящей SDL-очереди. Другие кодеки, несколько заголовков и смесь
event/stream явно отклоняются при preflight до окна/устройства. --mute по-прежнему
проверяет и пропускает root stream без аудиоустройства. Строгий импорт core не изменён:
плеер отдельно готовит snapshot того же источника и явно отключает stream-контролы в
модели сцены.

## Общий бюджет подготовки (v0.767)

Таблица, удерживаемые снимки и раскрытие следующего источника теперь вместе ограничены
64 МиБ до native startup. Preflight включает временные копии PCM и декодирование
ADPCM. [Контракт и проверки](PLAYER_STREAM_BUDGETS.ru.md). Прежние описания только
per-snapshot бюджета ниже относятся к историческим контрольным точкам.

## Ограниченное завершение конечного запуска (v0.765)

При `--frames`/`--once` плеер сохраняет длительность последнего кадра, затем ждёт
завершения уже разрешённых потоковых блоков и принятого PCM в очереди.
[fvm_desktop_audio_stream_pending](../frontend/common/audio.h#L102) копирует условие под mutex producer, не производит
PCM, не выделяет память, не декодирует, не продвигает фильм и не разрешает будущие
кадры. Пустые неактивные заголовки пропускаются ограниченным обходом; последующие
источники проверяются с блока ноль, поскольку worker перезапускает их при переходе.
При ошибке аргумент результата сохраняется; ошибка worker/backend передаётся наружу.

Без потока, при host pause, авторском stop или StopSounds дополнительное ожидание
не требуется. Для event sound сохраняется прежняя политика последнего кадра.
Будущий блок за пределами frame gate не удерживает конечный запуск. Завершение
включает опустошение FIFO, а не только EOF/передачу данных; это всё ещё не часы DAC.

Дополнительный срок — [FVM_DESKTOP_AUDIO_TAIL_MS](../frontend/common/audio.h#L20) (2000 мс) после более позднего
из планового конца последнего кадра и входа в tail. Этот кооперативный срок не прерывает заблокированный SDK/host callback. Если работа осталась,
воспроизведение завершается ошибкой `stream completion deadline exceeded` с обычным
join/cleanup. Ошибка очереди также даёт отказ. Поэтому слишком длинный либо
застрявший поток может потребовать другого предела кадров/частоты, но не обрезается
с успешным статусом по окончании срока. Audio-driven pacing, общая коррекция
seek/drift и time stretching этим не реализованы.

В ожидании продолжаются owner TURN и события окна: STOP, закрытие, смена корня,
пауза и уход в фон могут прекратить/пропустить дополнительное ожидание. PCM остаётся
на worker. Публичные дескрипторы host и структуры ABI VM не меняются. Запись ниже
о потере хвоста при голодании относится к прежней v0.764: безусловный выход после
планового кадра заменён ограниченным завершением.

[stream_pending.cpp](../frontend/tests/stream_pending.cpp) с настоящей очередью и остановленным worker проверяет gate,
пустой/активный/следующий источник, неизменность позиции, принятую очередь, pause,
cancel и сохранение результата при ошибке. [tail_stream.py](../frontend/tests/tail_stream.py) запускает реальный
owner loop с протокольным presenter: успех в 180 мс, закрытие до 130 мс, ошибку
наблюдения и точный timeout в 2100 мс после кадра длиной 100 мс. Это не device-тест. CLI-оракул дополнительно задаёт `--fps 1000` для PCM/ADPCM
и предел в один кадр: весь разрешённый PCM должен завершиться без будущих блоков.
Прежняя проверка фоновой паузы сохранена; её fake endpoint теперь реализует текущие
наблюдения аудио и использует только булевый, не разыменовываемый маркер устройства.

## Целые ADPCM-пакеты в плеере (v0.764)

Корневой ADPCM теперь принимается вместе с PCM в прежнем лимите 64 заголовков,
без смешивания с корневыми StartSound. Поддержаны коды 2–5 бит, моно/стерео и
исходные частоты из [контракта целых пакетов](STREAM_ADPCM.ru.md). Короткие пакеты
и другие кодеки остаются неподдержанными. Muted/raw-инспекция сохраняет непрозрачные
байты: при mute декодирования и снимков нет.

До открытия окна/устройства [fvm_desktop_audio_validate](../frontend/common/audio.h#L76) временно подготавливает
и закрывает ADPCM-снимки. Поэтому предварительная проверка теперь может выделять
память/декодировать и возвращать OUT_OF_MEMORY; ошибки заполнения и неподдержанной
геометрии передаются вызывающему. При последующем открытии аудио повторно проверяется
политика заголовков и подготавливаются удерживаемые снимки до открытия SDL-очереди.
Декодирование между preflight и acquisition намеренно повторяется: предварительная
проверка не сохраняет скрытого владения. Бюджет одной подготовки не является
общим бюджетом всех 64 удерживаемых снимков.

Подготовленный ADPCM представлен PCM_NATIVE/S16. Используются прежние converter,
frame gate, продолжение очереди, stop/pause/navigation и освобождение после join.
В worker не добавляются декодирование, VM или аллокации. Публичные дескрипторы
player-host ABI не меняются. [Готовый ADPCM-фильм и генератор](../examples/player/pcm_stream/README.ru.md)
включены в каталог примеров; FFmpeg нужен только для генерации/независимой сверки.

[stream_adpcm.py](../frontend/tests/stream_adpcm.py) независимо упаковывает постоянные differential codes для восьми
сочетаний ширины/каналов и сравнивает каждый принятый сэмпл после целочисленной
рациональной конверсии. Настоящий CLI проверяется FWS/CWS, interpreter/native JIT
и переходом к PCM-заголовку (32 запуска). Необязательный `--ffmpeg` сверяет все
71332 принятых стереокадра готового примера. Worker-fixture проверяет ADPCM с
stop/play, GC, StopSounds и переходами назад/на тот же кадр/вперёд. Fault sweep
начинается с ADPCM и включает уже подготовленные последующие PCM-заголовки.
Это dummy SDL, не приёмка физического окна/динамиков.

При сильном голодании host конечный запуск может завершиться до производства
всего разрешённого PCM. Прежний конечный PCM-пример проявил это при параллельной
кросс-линковке; изолированный повтор фиксируется отдельно. Audio-driven pacing
и завершающий drain-handshake здесь не реализованы, эти временные ограничения
не объявляются исправленными.

## Последовательность заголовков (v0.761)

Плеер принимает до [FVM_DESKTOP_AUDIO_MAX_SEGMENTS](../frontend/common/audio.h#L22) (64) проверенных PCM-заголовков,
по-прежнему без root StartSound. Каждый должен иметь PCM_NATIVE или PCM_LE;
сжатые кодеки, event/stream mixing и превышение лимита отклоняются до окна/device.
Mute проверяет произвольное число индексированных root segments без snapshots и
аудиоустройства. Этот предел ограничивает startup/seek, не размер пакета 9P и не
геометрию сетевого media channel.

Владелец выделяет обнулённый массив [FVM_DesktopStreamSegment](../frontend/common/audio.h#L26) и готовит snapshots/
converters до старта worker. Каждая запись владеет snapshot и отдельной phase grid.
При частичном отказе close сначала join worker, закрывает каждую ненулевую запись
один раз и освобождает массив; stream_source теперь только borrowed alias первой
записи. Snapshot не становится VM root и не передаётся в SDL device. Startup
проверяет/индексирует отдельно каждый сегмент и может выделять память; worker
переключается без выделений и не читает context.

При EOF источника [fvm_swf_stream_output_continue](../include/flashvm/swf_stream_output.h#L186) сохраняет принятые queue bytes,
pause и суммарные submitted/played clocks. Device открывается один раз. Выход
всегда 48000 Hz stereo S16, source width/mono/stereo/rate меняются по заголовкам.
Фаза переносится между блоками одного header и начинается с нуля у нового. Перед
активацией следующего его cursor сбрасывается к нулевому блоку: старые loops/seeks
не оставляют EOF. Переходы headers, empty/skipped markers и PCM preparation тратят
общий бюджет [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L18) на итерацию; цепочка 64 пустых headers не
становится неограниченным tight loop. stream_info показывает общий EOF только
для исчерпанного последнего header; FIFO drain и StopSounds cancellation отдельны.

Explicit/backward/skipped навигация выбирает последний header не позже target
frame (при равных frames — последний в source order), а до первого header —
первый будущий. [fvm_swf_stream_output_bind_frame](../include/flashvm/swf_stream_output.h#L195) выполняет staged pause/clear и
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

Владелец импортирует/укореняет root и читает [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L104) в точках обслуживания. Выбранный, но ещё
не выполненный кадр не разрешает звук. Только скаляры entered frame и play передаются под
audio mutex; worker не читает context, clip и managed values. Он владеет независимым
snapshot, converter cursor и retry buffer и делает не больше [FVM_DESKTOP_AUDIO_PUMPS](../frontend/common/audio.h#L18) вызовов [fvm_swf_stream_output_pump_until](../include/flashvm/swf_stream_output.h#L86) за итерацию.
Backend SDL копирует принятый PCM и не входит в VM/GC. Reserve учитывает реальный quantum
драйвера. Короткий разрешённый блок/EOF запускает непустую очередь даже ниже обычного
порога, чтобы ожидание reserve не блокировало старт.

Пауза хоста/сессии/background и stop клипа останавливают очередь, сохраняя данные; play
возобновляет разрешённые данные. StopSounds очищает очередь и отменяет stream до
обнаруженной навигации. Release ramp для stream пока нет. Pending/backward/skipped переход
вызывает [fvm_swf_stream_output_seek_frame](../include/flashvm/swf_stream_output.h#L178) под тем же mutex: старые queued/pending bytes удаляются, phase=0 и выбирается
первый блок не раньше выбранного кадра. Пропуск кадра удерживает более поздний блок,
тишина не синтезируется. EOF источника отличается от опустевшей очереди.

С v0.759 владелец также читает [fvm_movieclip_get_navigation](../include/flashvm/movieclip.h#L116). Изменившийся token вызывает clear/seek, даже если goto
выбрал и выполнил тот же кадр целиком между наблюдениями. nextFrame/prevFrame также меняют
token; обычные ticks/play/stop — нет. Pending/backward/skipped проверки сохранены, в том
числе естественный loop rewind. Это всё ещё не полный журнал: промежуточные цели и эффекты
не переигрываются. Drift/drop/stretch, stream+event mixing,
MP3/короткие ADPCM-пакеты, синхронизация видео и эталонная Flash-семантика seek остаются
открытыми. Root unload отсоединяет доставку, дожидается worker и освобождает
snapshot/queue/device до уничтожения context. GC выполняется только владельцем в штатных
safe points.

## Нативный идентификатор навигации (v0.759)

`fvm_movieclip_get_navigation(context, clip, &token)` копирует unsigned long token. Null output, неверное/чужое/prototype состояние и
GC/teardown дают INVALID_ARGUMENT, сохраняя выход. Ограничения
owner-thread/same-context/retained clip как у [fvm_movieclip_get_playback_info](../include/flashvm/movieclip.h#L104). Нет выделений, getters, callbacks,
продвижения или managed pointers наружу. Сравнивайте только равенство для того же
удерживаемого клипа/content. Новый или старый граф без optional private record возвращает
ноль. Проверенный explicit navigation увеличивает token, включая goto на тот же кадр; [FVM_MOVIECLIP_NAVIGATION_MAX](../include/flashvm/movieclip.h#L106)
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
| [fvm_desktop_audio_validate](../frontend/common/audio.h#L76) | Owner-thread проверка количества, формата и event-политики. OK без stream, UNSUPPORTED для неподдержанных сочетаний, ошибки структуры распространяются. Нет native device или полученного владения. |
| [fvm_desktop_audio_open_paused](../frontend/common/audio.h#L85) | Получает snapshot/converter/FIFO/device/mutex/worker и stop binding при наличии звука; pause только 0/1. После любой частичной ошибки вызывайте close. Snapshot независим от VM, но stop binding отсоединяется до context destroy. |
| [fvm_desktop_audio_timeline](../frontend/common/audio.h#L81) | Владелец читает playback/navigation token удерживаемого клипа своего контекста и публикует скаляры под mutex. Ошибки query/seek/backend возвращаются; pause/clear имеют staged effects без rollback. Worker в VM не входит. |
| [fvm_desktop_audio_stream_info](../frontend/common/audio.h#L97) | Копирует stream-only [FVM_SWFStreamOutputInfo](../include/flashvm/swf_stream_output.h#L12) под mutex. Ошибки сохраняют дескриптор; возможны ожидание lock/backend. Это FIFO dequeue position, не DAC/movie sync. |
| [fvm_desktop_audio_pause](../frontend/common/audio.h#L92) / [fvm_desktop_audio_tick_paused](../frontend/common/audio.h#L90) | Независимая пауза хоста; нет очистки данных. Tick проверяет worker error latch. Хост вызывает последовательно; worker сериализован внутри. |
| [fvm_desktop_audio_clear](../frontend/common/audio.h#L105) / [fvm_desktop_audio_close](../frontend/common/audio.h#L107) | Владелец отсоединяет binding/callback, join worker, закрывает snapshot и device, уничтожает mutex. Clear сохраняет счётчики. Context/userdata должны жить до detachment/quiescence. |

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
В исходной контрольной точке audio_underruns считал только event-producer.
С v0.769 [восстановление stream](PLAYER_STREAM_RECOVERY.ru.md) также учитывает
наблюдаемую пустую работающую очередь при разрешённой работе. Длительность
аппаратного underrun не измеряется, поведение ЦАП из пустого FIFO не выводится.

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
## Область проверок v0.763–v0.765

Прошли 108 выбранных нативных и 23 shared-host проверки: настоящий CLI PCM/ADPCM,
offscreen-встраивание, ресурсные/frame-каналы, host ABI и завершение с обработкой
событий. Core-подготовка также прошла 20 stream/ADPCM-тестов и точную сверку FFmpeg.
ASan/UBSan проверили изменённые модули/fixture; отказы выделения изолировались до
бюджета 4 для core и 11 для frontend. Вся VM и зависимости SDL не инструментировались;
перечисление утечек процессов здесь недоступно. Один sanitizer-запуск примера при
параллельной сборке превысил явный срок завершения; изолированный повтор ADPCM/FFmpeg/
быстрых frame-deadline сценариев прошёл. Истечение срока остаётся ошибкой, успех
при произвольном голодании CPU не гарантируется.

Полные минимальные ядра и три ADPCM/C89-программы слинкованы для ARM64, RISC-V64,
x86-64 Linux/musl. x86-64/musl-тесты действительно запущены здесь. Обновлённый
статический ARM64 SDL/AGG-плеер слинкован и проверен по ELF. Исполнение на другом CPU,
физические устройства и полная Flash/platform-приёмка остаются открытыми.
Прежние пропуски комментариев Windows speech описаны в [core-отчёте](STREAM_ADPCM.ru.md);
этот общий проверщик успешным не объявляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_navigation` | function | [include/flashvm/movieclip.h:116](../include/flashvm/movieclip.h#L116) |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:104](../include/flashvm/movieclip.h#L104) |
| `FVM_MOVIECLIP_NAVIGATION_MAX` | macro / define | [include/flashvm/movieclip.h:106](../include/flashvm/movieclip.h#L106) |
| `fvm_swf_stream_output_bind_frame` | function | [include/flashvm/swf_stream_output.h:195](../include/flashvm/swf_stream_output.h#L195) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:186](../include/flashvm/swf_stream_output.h#L186) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:86](../include/flashvm/swf_stream_output.h#L86) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:178](../include/flashvm/swf_stream_output.h#L178) |
| `FVM_SWFStreamOutputInfo` | struct | [include/flashvm/swf_stream_output.h:12](../include/flashvm/swf_stream_output.h#L12) |
<!-- source-links:end -->
