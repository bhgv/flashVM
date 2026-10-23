# Вывод подготовленного потока с caller-буфером

[English](STREAM_PCM_OUTPUT.md), [conversion](STREAM_PCM_CONVERSION.ru.md).
`swf_stream_output.h` соединяет native converter с существующим
[FVM_AudioQueueBackend](../include/flashvm/swf_sound_output.h#L16). Producer, retry buffer и converter принадлежат caller;
init/pump/pause/info/seek не выделяют память и не обращаются к VM. Это
ограниченная FIFO подача, а не готовое desktop SWF stream playback.

## Владение, обратное давление, повторы

Привяжите converter в текущей позиции, эксклюзивную пустую backend queue и
неперекрывающийся S16 buffer. Block — 1..1024 output frames, target не меньше
block и не больше SOUND_OUTPUT_MAX_QUEUE. Capacity считает значения каналов,
не меньше block*output_channels. Геометрия converter/backend должна совпасть.
Init копирует backend table, заимствует userdata, проверяет пустую очередь и
ставит pause при успехе. Ошибка сохраняет state/converter/PCM; init reentry
запрещён. Backend статусы передаются, C++ исключения превращаются в ERROR.

Хост сохраняет snapshot/converter/buffer/backend до завершения операций и
callbacks. Нельзя отдельно seek/read заимствованного converter, менять retry PCM
или писать/очищать backend. Все вызовы сериализуются; busy guard отклоняет
callback reentry, но не заменяет mutex. После quiescence хост явно чистит/закрывает
queue/device; скрытого close или деструктора producer нет.

Pump готовит не больше одного converted chunk по свободному target budget.
Неудачная write сохраняет точный pending PCM/chunk: converter продвинут один
раз, retry не декодирует его повторно. Backend write копирует все frames до OK,
не принимает ни одного при ошибке/исключении. Частично записанный файл этому
контракту не соответствует: перед необратимым I/O нужна собственная FIFO.
Пустые/skipped markers дают OK/accepted=0 и обновляют chunk metadata.
NOT_FOUND — backpressure или замеченный EOF, различаются по info.eof.
Необязательный accepted обнуляется первым, включая ошибки аргументов.

## Счётчик очереди и явный seek

Info проверяет exclusive backlog: queued <= target/submitted, submitted-queued
не регрессирует. Нарушение даёт ERROR с сохранением caller output.
submitted_frames/played_frames точны в double до MAX_CLOCK, seconds равен
played_frames/output_rate. Это dequeue accounting, не DAC и не movie/A/V clock.
EOF может быть true, пока последние принятые frames ещё в очереди.
Pause/resume меняет только backend, без priming/clear/advance.

Seek проверяет абсолютный source block/offset на копии converter до callbacks.
Затем pause и clear. Только успешный clear публикует новую position/phase zero,
отбрасывает pending и сбрасывает epoch счётчиков. Если pause успешен, а clear
нет, pause остаётся подтверждённым; pending/source и старая epoch сохраняются
для retry. Успешный seek оставляет pause: хост явно готовит backlog и resume.
Уже услышанный звук вернуть невозможно.

| API | Контракт |
| --- | --- |
| [`fvm_swf_stream_output_init`](../include/flashvm/swf_stream_output.h#L45) | Копирует backend и заимствует native объекты; нужны пустая queue/совпадающая геометрия. INVALID_ARGUMENT при неверных descriptor/bounds, UNSUPPORTED при недостаточной точности double, backend статусы передаются. Без allocation/advance. |
| [`fvm_swf_stream_output_pump`](../include/flashvm/swf_stream_output.h#L54) | Проверяет budget, готовит/повторяет один блок, отдаёт backend. Ошибка write сохраняет pending; пустые markers OK/0; EOF/backpressure NOT_FOUND. Accepted обнуляется первым; новая pending подготовка намеренно сохраняется при write error. |
| [`fvm_swf_stream_output_pause`](../include/flashvm/swf_stream_output.h#L107) | Подтверждает pause 0/1, без clear/priming. Ошибка сохраняет подтверждённый pause. |
| [`fvm_swf_stream_output_info`](../include/flashvm/swf_stream_output.h#L119) | Копирует clock/chunk лишь после корректного queued observation. Точный struct_size, ошибка сохраняет output. Без source read/allocation. |
| [`fvm_swf_stream_output_seek`](../include/flashvm/swf_stream_output.h#L124) | Проверяет source position, pause/clear, затем reset source grid/pending/epoch. Поэтапный контракт ошибки выше; неверная позиция не вызывает backend side effects. |

## Сборка и проверка

```sh
cmake -S flashVM -B build-stream-output -DFVM_BUILD_TESTS=ON
cmake --build build-stream-output --target flashvm_tests_swf_stream_output \
  flashvm_tests_swf_stream_output_allocation flashvm_tests_swf_stream_pcm_reader_api
ctest --test-dir build-stream-output -R 'swf_stream_(output.*|pcm_reader_api)$' --output-on-failure
```

Native тест проверяет ошибки/исключения write, точный retry, callback reentry,
backpressure, consumed clocks, невозможный backlog/регрессию, empty/EOF markers,
seek/clear failure и отмену pending. Allocation injection запрещает новые
выделения после уничтожения source VM/runtime. Model backend проверяет контракт,
не слышимое устройство или полную player/platform приёмку.

## Исполняемый native FIFO/file адаптер (v0.754)

[C89 swf_stream](../examples/multimedia/swf_stream/README.md) добавляет
`OUTPUT.pcm --queue`. Прокомментированный [queue.inc](../examples/multimedia/swf_stream/queue.inc)
входит в executable, это не эскиз. Подготовленный converter 48000 Hz stereo
привязан к собственной FIFO на два frames. Backend write атомарно копирует лишь
в native storage, не пишет файл. При заполнении или source EOF последовательный
host consumer выводит FIFO через настоящий fwrite. Ошибка файла прерывает экспорт:
producer не получает ложной возможности повторить частично записанный файл.

Source VM/runtime уже уничтожены. Хост владеет snapshot/retry storage/FIFO/userdata
до quiescence producer/consumer. Пустые блоки видны в отчёте. EOF с непустой FIFO
не означает завершение: последние байты выводятся до pause/close. Файл и отчёт
совпадают с `--convert`: четыре stereo frames, шестнадцать raw native-endian байт.
Независимый integer oracle сравнивает все байты; ошибка открытия/cleanup проверяется.

Пример не запускает audio device, thread или socket. Это настоящая ограниченная
native FIFO и file consumption, не слышимый звук, real-time queue scheduling,
DAC clock или SWF timeline/A/V synchronization.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --queue
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

Проверка v0.754: прошли 75 выбранных native регрессионных проверок и 14 shared-host. Новые output/converter/reader/snapshot пути, worker контракты и все пять export режимов прошли ASan/UBSan; allocation injection — native. Leak detection отключён ограничениями хоста, вся VM не инструментирована. Новые C++98 implementations и C89 consumer/tutorial objects собираются для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW; foreign execution или полная player/device приёмка не заявляются. Все девять стадий открыты.

## Явное разрешение movie frames (v0.755)

Прежний unrestricted pump сохранён. Новый gated pump принимает лишь блоки с
номером movie frame не больше заданной хостом границы; ноль блокирует все кадры.
Проверяются доверенные prepared metadata до conversion либо pending metadata до
повторной write. Будущий кадр даёт NOT_FOUND без продвижения converter и записи
PCM; queue-clock observations могут обновиться. Будущие данные не теряются.
Пустые/skipped markers подчиняются той же границе; пропуск не разрешает более
поздний звук автоматически.

Отдельный next-frame query возвращает сначала pending frame, иначе frame из
source index. Нет backend callbacks, decode, allocation или VM. NOT_FOUND на EOF
сохраняет выход. Вместе с info это различает gate, FIFO backpressure и EOF.
Gate не отзывает уже принятый FIFO звук. Переход назад/новый root incarnation
требует явного seek/clear либо новых native объектов: уменьшившийся tick counter
не является безопасной синхронизацией. Хост задаёт настоящий movie frame,
а не число host ticks/presentations.

| API | Владение и ошибки |
| --- | --- |
| [`fvm_swf_stream_output_next_frame`](../include/flashvm/swf_stream_output.h#L58) | Копирует следующий pending/indexed movie frame; владение source/backend неизменно. Без callback/VM/allocation. INVALID_ARGUMENT при null/reentry, NOT_FOUND на EOF; ошибка сохраняет выход. |
| [`fvm_swf_stream_output_pump_until`](../include/flashvm/swf_stream_output.h#L67) | Контракт retry/backpressure/accepted=0 как у pump; max_movie_frame — явное разрешение, ноль блокирует всё. Будущий/pending-too-new блок даёт NOT_FOUND без conversion/write. Seek/clear при переходе назад выполняет хост. |

Native тест проверяет gate zero, пропуски frames, неудачный retry будущего блока,
пустые markers, приоритет pending, неизменность позиции/байтов и EOF с данными
в FIFO. Allocation injection проверяет query/gated pump после уничтожения VM/runtime.
Полная sound timeline/device/reference приёмка остаётся открытой.

## Настоящий MovieClip на owner (v0.756)

[C swf_stream](../examples/multimedia/swf_stream/README.md) добавляет
`OUTPUT.pcm --timeline`. В отличие от native-only режимов он сохраняет source
context и импортирует тот же movie как pinned root с явным пропуском stream
исполнения. PCM хранится в независимом snapshot/FIFO. Хост продвигает MovieClip,
читает его настоящий _currentframe на owner и разрешает этот номер producer.
Host ticks не подменяют source frame. При контролируемом stop два настоящих ticks
оставляют frame 1 и не добавляют повторный/будущий PCM. Play/resume явные.
Пропуск source frame ждёт фактического позднего кадра; empty markers сохраняют
свою границу. Root/library pinned через owner GC steps; unpin выполняется до
context destruction, после quiescence producer.

Файл/отчёт точно совпадают с `--queue`/`--convert`: четыре converted stereo frames
и явный пустой блок. Последовательный file tutorial проверяет authorization/
ownership, не воспроизведение по часам. Audio device, worker, окно и автоматический
desktop stream playback не устанавливаются.

Отдельный raw SWF integration test задаёт каждому кадру AVM1 assignment. Он
запускает interpreter и доступный native JIT, проверяет visited values и требует
положительный native_entries в JIT режиме. Проверены реальные stop/play ticks,
source gaps, GC safe points и gotoAndStop назад. Возврат сопровождается native
seek/clear перед priming. Прямой seek к frame 3 начинает новую rational grid:
его отдельная output duration намеренно отличается от непрерывного forward grid;
тест проверяет reset, а не предполагает непрерывность.

Это ограниченная owner integration, не reference-player stream semantics,
child/multi-segment scheduling, stream/event mixing, codec/DAC/A/V sync.
Немутое desktop root stream playback пока не поддерживается.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --timeline
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

Source context, clip/prototypes и все fvm_object_*/MovieClip/GC вызовы принадлежат
только VM owner. Backend callbacks/native audio workers получают скопированное
scalar authorization, не входят в VM и не заимствуют clip pointers. Хост разрешает
PCM лишь после нужной safe point собственных frame actions/transitions: ошибка
кадра/скрипта может оставить уже совершённые эффекты.

## Явный source seek по movie frame

[`fvm_swf_stream_output_seek_frame`](../include/flashvm/swf_stream_output.h#L130) принимает положительный movie frame, бинарным
поиском выбирает первый блок сегмента с frame не меньше запрошенного и вызывает
прежний staged pause/clear/seek. Пропуск выбирает поздний блок, который gate всё
равно удерживает до разрешения. После последнего блока выбирается source EOF.
Rational grid начинается с phase zero. Decode/allocation/VM lookup не нужны;
неверный ноль/reentry дают INVALID_ARGUMENT до backend effects.

Gate — верхняя граница, не exact-frame selector: увеличение после перехода вперёд
разрешит и старые ещё не переданные блоки. Если пропущенный звук надо отбросить,
нелинейные forward/backward переходы MovieClip сопровождаются явным source seek/
clear. Helper задаёт именно prefix-discard policy, не все reference Flash stream
seek semantics и не выбор другого header segment. Ошибки pause/clear сохраняют
staged контракт block seek. Настоящий interpreter/native-JIT тест использует
helper для gotoAndStop.

| API | Владение и ошибки |
| --- | --- |
| [`fvm_swf_stream_output_seek_frame`](../include/flashvm/swf_stream_output.h#L130) | Без VM/allocation находит первый indexed block >= positive frame, затем pause/clear/reset epoch. Пропуск ждёт, позиция после конца даёт EOF. Неверные metadata/args не вызывают backend effects; поздние ошибки callbacks следуют staged seek контракту. |

Проверка v0.756: прошли 77 выбранных native проверок (один ранее собранный MP3 тест повторён после восстановления права исполнения), а также 14 shared-host. Обновлённые frame-seek/gating/C89 контракты прошли финальный focused набор. ASan/UBSan проверяет native output/reader/snapshot и настоящую MovieClip authorization с interpreter/native-JIT actions; вся VM не инструментирована, leak detection отключён хостом. Все шесть export режимов совпадают по PCM; minimal build работает без desktop frontend/device. Новые C++98/C89 implementation/consumer/tutorial objects собираются для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW; foreign execution/full cross-player не заявляются. Все девять стадий открыты.


## Нативное наблюдение таймлайна (v0.757)

`fvm_movieclip_get_playback_info(context, clip, &info)` читает приватное состояние локального таймлайна, обходя заменяемое свойство `_currentframe`. Сначала установите `info.struct_size = sizeof(info)`. Снимок [`FVM_MovieClipPlaybackInfo`](../include/flashvm/movieclip.h#L69) содержит положительные `frame`, `total_frames` и флаги `playing`, `entered`. После создания и goto кадр выбран, но `entered=0`; реальный последующий вход устанавливает единицу. Один выбор кадра не разрешает его потоковый звук. Остановленный клип продолжает получать onEnterFrame при последующих ticks; пауза хоста — отдельная политика.

Вызывайте только из потока-владельца VM вне GC/teardown, удерживая клип того же контекста. Нет выделений памяти, callbacks, поиска script-свойств или продвижения кадров; наружу копируются скаляры, которым не нужен root. Null, неверный размер, чужой контекст, прототип и повреждённое состояние дают INVALID_ARGUMENT, сохраняя весь выходной дескриптор. Это не журнал навигации: несколько изменений между наблюдениями могут быть не видны. Идентичность источника, очистка очереди при переходах и синхронизация остаются обязанностью хоста. Тест movieclip_playback_info.cpp проверяет атомарность ошибок, отсутствие выделений, stopped ticks, различие выбора/входа, цикл, заменённое script-свойство и удержание через GC.

## Несколько подготовленных источников (v0.760)

`fvm_swf_stream_output_continue(output, next)` требует исчерпанного текущего
источника без pending PCM и другого initialized converter с теми же output
rate/channels, что у backend. Следующий converter заимствуется с текущей позиции;
source rate/channels могут отличаться. Меняются только binding и EOF. Принятые
FIFO samples, submitted/played clocks, pause и последний chunk сохраняются.
Пустые источники пропускаются отдельными ограниченными шагами хоста. Нет
выделений, VM, backend callbacks и неявного decode. Порядок источников, отдельная
фаза преобразования, lifetime и сериализация принадлежат хосту. INVALID_ARGUMENT:
unfinished/pending/self/null/mismatched/reentrant; прочие validation errors
распространяются, сохраняя оба cursors, FIFO и дескриптор.

`fvm_swf_stream_output_bind_frame(output, target, positive_frame)` — явный разрыв:
проверка geometry и выбор первого блока не раньше кадра, затем pause/clear.
Только подтверждённый clear публикует новый binding, phase-zero position и
сброшенные часы. Допустим тот же converter. При ошибке clear прежний converter,
pending bytes и cursor цели сохраняются, подтверждённая пауза остаётся. Статусы
ошибок/исключений как у обычного seek. Beyond-last выбирает EOF, gap — поздний
блок. Хост эксклюзивно удерживает converter/snapshot до quiescence. Нет выделений
и VM calls. Это prefix-discard политика, не эталонный Flash seek между заголовками.

`swf_stream_output_segments.cpp` проверяет точный PCM, пустой промежуточный
источник, разные source rate/channels, continuity непустой FIFO, переход без
callbacks, отсутствие выделений после VM teardown, geometry errors, reentry и
сохранение pending retry bytes/обоих cursors при failed clear.


Аддитивный [fvm_swf_stream_output_pump_mix_until](../include/flashvm/swf_stream_output.h#L104) согласует разрешённый base, события на hold/EOF и порядок retry. Прежние stream-only API сохраняют контракты. [Текущий контракт](MIXED_GATES.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_movieclip_get_playback_info` | function | [include/flashvm/movieclip.h:83](../include/flashvm/movieclip.h#L83) |
| `FVM_MovieClipPlaybackInfo` | struct | [include/flashvm/movieclip.h:69](../include/flashvm/movieclip.h#L69) |
| `fvm_swf_stream_output_bind_frame` | function | [include/flashvm/swf_stream_output.h:143](../include/flashvm/swf_stream_output.h#L143) |
| `fvm_swf_stream_output_continue` | function | [include/flashvm/swf_stream_output.h:136](../include/flashvm/swf_stream_output.h#L136) |
| `fvm_swf_stream_output_info` | function | [include/flashvm/swf_stream_output.h:119](../include/flashvm/swf_stream_output.h#L119) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:45](../include/flashvm/swf_stream_output.h#L45) |
| `fvm_swf_stream_output_next_frame` | function | [include/flashvm/swf_stream_output.h:58](../include/flashvm/swf_stream_output.h#L58) |
| `fvm_swf_stream_output_pause` | function | [include/flashvm/swf_stream_output.h:107](../include/flashvm/swf_stream_output.h#L107) |
| `fvm_swf_stream_output_pump` | function | [include/flashvm/swf_stream_output.h:54](../include/flashvm/swf_stream_output.h#L54) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:104](../include/flashvm/swf_stream_output.h#L104) |
| `fvm_swf_stream_output_pump_until` | function | [include/flashvm/swf_stream_output.h:67](../include/flashvm/swf_stream_output.h#L67) |
| `fvm_swf_stream_output_seek` | function | [include/flashvm/swf_stream_output.h:124](../include/flashvm/swf_stream_output.h#L124) |
| `fvm_swf_stream_output_seek_frame` | function | [include/flashvm/swf_stream_output.h:130](../include/flashvm/swf_stream_output.h#L130) |
<!-- source-links:end -->
