# Явное преобразование подготовленного PCM потока

[English](STREAM_PCM_CONVERSION.md), [native reader](STREAM_PCM_READER.ru.md).
`swf_stream_convert.h` — C89 converter без allocation поверх неизменяемого PCM
снимка. Он готовит native S16 для явно выбранных частоты/каналов, без живой VM,
устройства или системных часов.

## Позиция и политика границ

Output rate задаётся явно: 5512..192000 Hz, mono/stereo. Source rate/channels —
настоящая геометрия потока, а не playback advice. Сетка продвигается на
source_rate/output_rate через целочисленные quotient/remainder. Дробная фаза
и перескок source frames сохраняются между encoded blocks, включая пустые:
округление не накапливает ошибку длительности на каждой границе. Для N source
frames выдаётся ceil(N*output_rate/source_rate) без умножения неограниченных длин.
Пропуски movie frames сохраняются как metadata: converter не создаёт тишину и
не реализует таймлайн воспроизведения.

Линейная интерполяция использует соседние значения лишь внутри блока. Последнее
значение удерживается до конца его source interval; чужой блок и пропуск кадров
не используются для интерполяции. Это явная детерминированная политика границ,
не band-limited resampler. Anti-alias filter, codec, A/V sync, queue/device
scheduling не добавляются. Немутое root stream playback плеера ещё не поддержано.

Mono дублируется в stereo. Stereo-to-mono усредняется к нулю перед временной
интерполяцией; итог тоже округляется к нулю. Произведения double точны ниже
2^53: нет переполнения 32-bit long на ILP32/LLP64. Поддерживаются только
PCM_NATIVE/PCM_LE, принятые существующим reader.

next посещает один encoded block, выдаёт не более min(max_frames,1024), сохраняет
stream/block/movie-frame. Capacity/written — отдельные значения каналов,
chunk.frames — выходные sample frames. first_source/phase описывают исходную
точку сетки. BEGIN — первый chunk/marker блока, END — больше output positions
в нём нет. Пустой блок даёт ноль значений и EMPTY. При downsampling непустой
блок может не содержать выходных точек: он всё равно даёт нулевой BEGIN|END
marker без EMPTY, перенося перескок дальше. OK всегда продвигает output или
block position; EOF — NOT_FOUND с сохранением выходов.

## API и владение

| API | Контракт |
| --- | --- |
| [`fvm_swf_stream_converter_init`](../include/flashvm/swf_stream_convert.h#L48) | Проверяет output rate/channels, заимствует native PCM snapshot, копирует source metadata/count. INVALID_ARGUMENT при неверных args, UNSUPPORTED при другом codec/host representation. Без allocation, ошибка сохраняет state. |
| [`fvm_swf_stream_converter_next`](../include/flashvm/swf_stream_convert.h#L67) | Положительный max_frames, точный chunk.struct_size, ёмкость в значениях каналов и PCM pointer для непустого результата. Ограниченный planning проверяет capacity до записи. EOF/ошибка сохраняют PCM/chunk/written/state. Нет VM, I/O, allocation, callbacks и неявных повторов. |
| [`fvm_swf_stream_converter_seek`](../include/flashvm/swf_stream_convert.h#L73) | Задаёт block/source offset <= sample_count; block_count/0 — EOF. Новая сетка начинается с phase zero. Ошибка сохраняет state. Уже сыгранный/отправленный output не перематывается. |

Хост владеет snapshot до завершения всех converter/reader. Исходные runtime/context
после подготовки можно уничтожить. Один converter используется последовательно;
state/input/output не перекрываются. Независимые converter могут делить native
bytes. Самоссылок нет: копирование позиции допустимо с тем же lifetime source.

## Проверка

Native тест проверяет все четыре source rates, блоки из одного/двух/трёх frames, пустые блоки, семь
output rates (включая 48000/192000), mono/stereo и разные размеры chunks.
Независимый целочисленный эталон long division не использует native float или
64-bit extensions. Проверяются duration, skipped blocks, EOF/seek/capacity
atomicity, ограничение работы и C89 linkage. Allocation injection запрещает
новые выделения при преобразовании после уничтожения исходной VM.

```sh
cmake -S flashVM -B build-convert -DFVM_BUILD_TESTS=ON
cmake --build build-convert --target flashvm_tests_swf_stream_converter \
  flashvm_tests_swf_stream_pcm_reader_api flashvm_tests_swf_stream_sound_snapshot_allocation
ctest --test-dir build-convert -R 'swf_stream_(converter|pcm_reader_api|sound_snapshot_allocation)$' --output-on-failure
```

## Native C экспорт и worker (v0.752)

[Прокомментированный swf_stream](../examples/multimedia/swf_stream/README.md)
принимает `OUTPUT.pcm --convert`: копирует segment, уничтожает исходные
runtime/context и экспортирует 48000 Hz stereo S16 через прямые C file callbacks.
Новая output VM не создаётся. Три source frames в двух непустых блоках дают
четыре output frames; пропущенный movie frame не заполняется, явный пустой блок
показывается отдельно. Первые строки отчёта описывают source lengths, итоговая —
output count. Default/`--prepared`/`--cursor` сохраняют прежние native-rate байты.

Независимый Python тест вычисляет converted samples через целочисленные weighted
sums/truncation и сравнивает каждый байт. FFmpeg остаётся отдельным эталоном
исходного U8-to-S16 decoding, а не явной block-edge политики этого converter.
Ошибки открытия и владение перезаписываемым файлом проверяются во всех режимах.
Четыре настоящих native worker дополнительно выполняют по 500 циклов conversion
rewind/read/EOF на 5512/48000/96000/192000 Hz, деля только immutable source bytes.
Main присоединяет всех до source close. Это ограниченная Linux native thread
проверка, не foreign OS/device или real-time audio callback acceptance.

```sh
cmake -S flashVM -B build-stream -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-stream --target flashvm_examples_multimedia_swf_stream_main
./build-stream/flashvm_examples_multimedia_swf_stream_main OUTPUT.pcm --convert
python3 flashVM/scripts/check_swf_stream_example.py ./build-stream/flashvm_examples_multimedia_swf_stream_main
```

Проверка v0.752: прошли 73 выбранные native проверки (два resource-player запуска повторены после восстановления права исполнения собранного файла), а также 14 shared-host. Новые converter/reader/snapshot пути, native worker и все file-export режимы прошли ASan/UBSan; вся VM не инструментирована, leak detection отключён хостом. Новый implementation и C89 consumer/tutorial objects собираются для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW; foreign binaries и полный cross-player не запускались. Все девять стадий открыты.
[Bounded queue producer / Ограниченный producer очереди](STREAM_PCM_OUTPUT.ru.md).


## Непрерывная интерполяция сегмента (v0.803)

[`fvm_swf_stream_converter_init_continuous`](../include/flashvm/swf_stream_convert.h#L56) принимает те же аргументы и статусы,
заимствует снимок на тех же условиях времени жизни и единственного владельца, что init.
Правый отсчёт берётся из следующего непустого блока; последнее значение удерживается
только в конце сегмента. Пустые блоки и пропуски кадров не добавляют тишину.
Нет выделений, callbacks, обращения к VM или дополнительного PCM-хранилища.
Поиск соседа один раз за next проходит промежуточные пустые дескрипторы: кроме
ограниченных 1024 выходных кадров возможен обход оставшегося индекса блоков.
Выход сохраняет идентичность блока левого отсчёта; чтение соседа не разрешает
воспроизведение будущего блока. Seek сбрасывает фазу, сохраняя политику.
Обычный init сохраняет прежнее ограничение интерполяции одним блоком.
В структуру состояния добавлено поле: нативные потребители нужно пересобрать.

Целочисленный оракул проверяет обе политики, четыре исходные и семь выходных частот,
моно/стерео, порции 1/11 кадров, пустые блоки, конец сегмента и seek. Это проверка
нативного преобразования, а не приёмка слышимого звука на устройствах или полного SWF.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:48](../include/flashvm/swf_stream_convert.h#L48) |
| `fvm_swf_stream_converter_init_continuous` | function | [include/flashvm/swf_stream_convert.h:56](../include/flashvm/swf_stream_convert.h#L56) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:67](../include/flashvm/swf_stream_convert.h#L67) |
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:73](../include/flashvm/swf_stream_convert.h#L73) |
<!-- source-links:end -->
