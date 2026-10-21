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
| [`fvm_swf_stream_converter_init`](../include/flashvm/swf_stream_convert.h#L47) | Проверяет output rate/channels, заимствует native PCM snapshot, копирует source metadata/count. INVALID_ARGUMENT при неверных args, UNSUPPORTED при другом codec/host representation. Без allocation, ошибка сохраняет state. |
| [`fvm_swf_stream_converter_next`](../include/flashvm/swf_stream_convert.h#L58) | Положительный max_frames, точный chunk.struct_size, ёмкость в значениях каналов и PCM pointer для непустого результата. Ограниченный planning проверяет capacity до записи. EOF/ошибка сохраняют PCM/chunk/written/state. Нет VM, I/O, allocation, callbacks и неявных повторов. |
| [`fvm_swf_stream_converter_seek`](../include/flashvm/swf_stream_convert.h#L64) | Задаёт block/source offset <= sample_count; block_count/0 — EOF. Новая сетка начинается с phase zero. Ошибка сохраняет state. Уже сыгранный/отправленный output не перематывается. |

Хост владеет snapshot до завершения всех converter/reader. Исходные runtime/context
после подготовки можно уничтожить. Один converter используется последовательно;
state/input/output не перекрываются. Независимые converter могут делить native
bytes. Самоссылок нет: копирование позиции допустимо с тем же lifetime source.

## Проверка

Native тест использует блоки из одного/двух/трёх frames, пустые блоки, семь
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_converter_init` | function | [include/flashvm/swf_stream_convert.h:47](../include/flashvm/swf_stream_convert.h#L47) |
| `fvm_swf_stream_converter_next` | function | [include/flashvm/swf_stream_convert.h:58](../include/flashvm/swf_stream_convert.h#L58) |
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:64](../include/flashvm/swf_stream_convert.h#L64) |
<!-- source-links:end -->
