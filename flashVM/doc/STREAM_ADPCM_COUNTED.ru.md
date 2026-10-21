# Неполные ADPCM-блоки с явными длинами (v0.773)

[English](STREAM_ADPCM_COUNTED.md) · [Автоматические целые пакеты](STREAM_ADPCM.ru.md) · [Бюджеты](STREAM_STORAGE.ru.md)

`fvm_swf_stream_sound_snapshot_open_adpcm_counted(context, stream, counts, count, budget_bytes, out)`
готовит независимый native PCM S16 из корневого ADPCM-потока, включая неполный
последний пакет на 4096 кадров. Хост с достоверными длинами затем использует обычные
reader/converter/output без исходной VM. Автоматическое воспроизведение таких
потоков настольным плеером этот API пока не добавляет.

## Почему длины задаёт хост

average_samples в SoundStreamHead — только рекомендация. Короткие коды и нулевой
padding могут допускать несколько длин: четыре байта mono с двухбитными кодами
могут представлять и два, и три кадра. Среднее число отсчётов и FPS не разрешают
выбирать длину произвольно. Хост получает точные counts из своего asset pipeline
или другого независимого контракта. API проверяет соответствие байтам, но не
доказывает исходное намерение автора. Старые open_pcm/open_pcm_bounded сохраняют
прежнее автоматическое распознавание только целых пакетов.

## Аргументы, ошибки и владение

| Аргумент | Контракт |
| --- | --- |
| context | Живой исходный контекст; синхронная работа на его owner-потоке без исполнения скриптов |
| stream | Индекс корневого SoundStreamHead/Head2 с нуля |
| counts | Инициализированный size_t-массив кадров каждого закодированного блока; заимствуется только до возврата |
| count | Точное число элементов, включая пустые блоки; null counts допустим только при нуле элементов |
| budget_bytes | Предел пикового native владения: объект, индекс, encoded и decoded payload |
| out | Обязательный отдельный записываемый указатель, обнуляется до проверки |

При OK публикуется неизменяемый PCM_NATIVE/S16 snapshot. Сохраняются rate/channels,
номера root frame/block, advisory playback и average. Исходные байты не меняются;
live и encoded snapshot API по-прежнему выдают непрозрачный ADPCM. Массив counts
можно освободить сразу после возврата. Snapshot живёт независимо от context/runtime;
после завершения всех readers/converters освобождается через
[fvm_swf_stream_sound_snapshot_close](../include/flashvm/swf_stream_sound.h#L154).

INVALID_ARGUMENT: неверные/null аргументы, несовпадение размера таблицы, длина выше
ULONG_MAX, укороченные/лишние байты и ненулевой padding. Ошибки исходного SWF и
отсутствующего stream передаются без подмены. При совпавшем числе блоков иной кодек
возвращает UNSUPPORTED; несовпадение длины таблицы имеет приоритет. OUT_OF_MEMORY:
арифметическое переполнение, allocation failure, превышение runtime payload/index
или явного peak-бюджета. Нулевой бюджет допустим, но недостаточен. Ошибка освобождает
временное владение и не публикует snapshot.

Подготовка выделяет память и декодирует: выполнять её до передачи worker, а не в
real-time audio callback. Нет device/backend callback, исполнения VM или создания
потока. Изменение/уничтожение context сериализует владелец. Неизменяемый snapshot
допускает параллельные чтения с отдельными курсорами; close — только после их
завершения. Managed pointers и GC roots не сохраняются.

Бюджет как у bounded open_pcm: encoded и decoded одновременно живут при подготовке;
encoded освобождается при успехе. [fvm_swf_stream_sound_snapshot_storage_info](../include/flashvm/swf_stream_sound.h#L117) сообщает
retained/peak без allocator bookkeeping, VM, counts, стека и caller buffers.
При подготовке нескольких сегментов хост вычитает retained из общего бюджета;
сам API не ограничивает память всего процесса.

## Геометрия и пустые блоки

Каждый непустой блок имеет собственные два бита разрядности. Пакет содержит
predictor/index каждого канала и до 4095 differential frames; следующий predictor
идёт без байтового выравнивания. Поддержаны разрядности 2–5, mono/stereo и неполный
последний пакет. Точная геометрия должна занимать все байты кроме максимум семи
**нулевых** битов в конце. Каждый блок начинает новый декодер. Ноль кадров допускает
пустой payload либо один width-only byte с нулевым padding. Пустой блок сохраняет
элемент counts; промежутки таймлайна не превращаются в синтезированную тишину.

## Воспроизводимые проверки

```sh
cmake --build build --target flashvm_tests_swf_stream_adpcm_counted \
  flashvm_tests_swf_stream_adpcm_counted_allocation
ctest --test-dir build --output-on-failure -R 'swf_stream_adpcm|swf_stream_storage'
python3 scripts/check_counted_adpcm_ffmpeg.py build/flashvm_tests_swf_stream_adpcm_counted
```

Native-матрица проверяет все разрядности/каналы и длины 0, 1, 2, 3, 7, 17, 4095,
4096, 4097, 8191, 8192, 8193: каждый отсчёт, EOF после VM teardown, точный peak и
нехватку одного байта, неверные длины/padding и неоднозначные нулевые коды. Allocation
fixture перебирает отказы четырёх приобретений и чтение без allocation после VM.
Числовой аргумент 0–4 запускает отдельный бюджет для sanitizer-процесса.

Необязательный Python oracle независимо пакует FWS/CWS с меняющимися кодами,
неполными restart-хвостами, пустыми блоками и намеренно неверным average. Все
отсчёты выбранных диапазонов совпадают с FFmpeg для восьми width/channel сочетаний.
FFmpeg нужен только для проверки. Физический звук, desktop mixed playback, MP3,
child streams, A/V и референсный movie timing, другие платформы остаются открытыми.
Ни одна стадия roadmap не закрывается.


## Граница проверки v0.773

Все 57 native sound/stream регрессий проходят strict C89/C++98 Linux minimal
Release. Матрица counts проходит с полным minimal core под ASan/UBSan. Пять
изолированных allocation-бюджетов проходят UBSan для изменённого stream-модуля и
fixture с обычным Release core. Успешное выделение также проходит полный ASan/UBSan.
ASan fault-unwind не засчитан: ограниченный хост не видит стек через /proc и после
искусственных исключений сообщает no-return/stack-poisoning diagnostics.
LeakSanitizer выключен из-за недоступного перечисления процессов. Физическая и
foreign-platform приёмка не заявляется.


## Диапазоны без выделения памяти (v0.774)

[fvm_swf_stream_sound_read_adpcm](../include/flashvm/swf_stream_sound.h#L146)(context, stream, block, samples, first, frames,
out, capacity_values, written_values) читает один live root ADPCM block на owner
контекста. [fvm_swf_stream_sound_snapshot_read_adpcm](../include/flashvm/swf_stream_sound.h#L150)(snapshot, block, samples,
first, frames, out, capacity_values, written_values) читает **закодированный**
неизменяемый snapshot, в том числе после уничтожения VM. Нормализованный PCM
snapshot возвращает UNSUPPORTED; для него остаётся обычный PCM reader.

samples — достоверное полное число кадров выбранного блока; first/frames —
запрошенный диапазон source frames. capacity_values/written_values считают
interleaved S16 channel values. При положительном frames нужны aligned output
и достаточная capacity; при нуле допускается null output. written_values обязателен,
обнуляется до проверки источника. Вход, выход и счётчик должны быть непересекающимися.
Ошибка не меняет PCM. Даже для раннего либо пустого диапазона проверяются полная
геометрия, padding и все объявленные коды. Правила empty/width-only соответствуют
counted preparation.

INVALID_ARGUMENT: неверные указатели, диапазон/capacity, padding, длина/число кадров,
samples выше ULONG_MAX. UNSUPPORTED: иной кодек либо несовместимое native S16
представление. OUT_OF_MEMORY означает арифметическое переполнение битовой длины,
хотя функции не выделяют память. Ошибки исходного SWF/отсутствующего stream/block
передаются как есть с нулевым written. Нет callback, исполнения VM, изменения
источника, нового native владельца или managed/GC ссылки. Live context требует
owner-сериализации; immutable encoded snapshot допускает параллельные чтения в
разные buffers; close и teardown сериализуются отдельно.

Декодер проверяет блок с начала: время линейно в полном числе кадров, а не размере
выбранного диапазона. Мелкие повторные чтения повторяют работу. Путь подходит для
анализа/извлечения assets; для low-latency playback следует один раз подготовить
нормализованный PCM. Автоматические worker, desktop scheduling, child streams и
вывод временных меток не добавлены.

Range fixture проверяет все разрядности/каналы через packet restart, неверные
capacity/ranges/codecs, ошибочный хвост даже для пустого чтения, reload и чтение
после VM. C89 fixture вызывает все три counted/range API с буквальными SWF bytes.
Allocation fixture запрещает выделение памяти для live/native range reads;
FFmpeg oracle повторно сравнивает весь файл через native ranges после VM teardown.


### Проверка диапазонов

Все 60 native sound/stream проверок проходят Linux minimal Release. Полный minimal
core под ASan/UBSan проходит counted/range матрицы, C89 ABI, успешное выделение и
четыре настоящих native worker, читающих общий encoded snapshot после VM teardown.
Изменённый stream-модуль и allocation fixture проходят все пять отдельных UBSan
бюджетов, включая live/native reads без allocation. Прежние ограничения ASan при
исключениях и LeakSanitizer сохраняются. Сравнение FFmpeg FWS/CWS проходит обоими
exporters (prepared/ranges) для восьми сочетаний разрядности/каналов. Это Linux
functional checks, не физический звук, foreign-platform приёмка или race detector.

```sh
cmake --build build --target flashvm_tests_swf_stream_adpcm_ranges \
  flashvm_tests_swf_stream_adpcm_ranges_c89 flashvm_tests_swf_stream_adpcm_ranges_os_ports
ctest --test-dir build --output-on-failure -R 'swf_stream_adpcm'
```

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_read_adpcm` | function | [include/flashvm/swf_stream_sound.h:146](../include/flashvm/swf_stream_sound.h#L146) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:154](../include/flashvm/swf_stream_sound.h#L154) |
| `fvm_swf_stream_sound_snapshot_open_adpcm_counted` | function | [include/flashvm/swf_stream_sound.h:111](../include/flashvm/swf_stream_sound.h#L111) |
| `fvm_swf_stream_sound_snapshot_read_adpcm` | function | [include/flashvm/swf_stream_sound.h:150](../include/flashvm/swf_stream_sound.h#L150) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:117](../include/flashvm/swf_stream_sound.h#L117) |
<!-- source-links:end -->
