# Явный бюджет памяти подготовленных потоков

[English](STREAM_STORAGE.md) · [подготовка ADPCM](STREAM_ADPCM.ru.md) · [учебный C89-пример](../examples/multimedia/swf_stream/README.md)

`fvm_swf_stream_sound_snapshot_open_pcm_bounded(context, stream, budget_bytes, out)`
добавляет задаваемый вызывающим предел **пиковой владеемой памяти**. Поддерживаются
те же PCM и целые ADPCM-пакеты, что у [`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L73);
новых кодеков здесь нет. Владелец контекста подготавливает снимок до публикации
неизменяемых данных worker. Указатель результата сначала обнуляется. Ошибки
контекста/индекса и null output сохраняют прежние статусы; корректный нулевой
бюджет даёт OUT_OF_MEMORY: даже пустому снимку нужен объект владения.
Статусы неподдержанных/некорректных кодеков сохраняются. При отказе выделения —
OUT_OF_MEMORY с освобождением временных данных. Устройство, callback и VM-код
не запускаются, кадр фильма не продвигается.

Предел включает объект снимка, точный индекс блоков, массив сжатых/исходных байтов
и одновременно выделенный массив native S16. Начальный объём проверяется **до**
выделения объекта/индекса/байтов. Раскрытие проверяется до выделения PCM-массива
и запуска декодера. Прежний runtime-предел payload/index применяется независимо;
новая функция его не повышает и не изменяет. Прежние raw/open_pcm API сохраняют
исходную политику бюджета payload. Они также записывают размеры, но не получают
нового явно задаваемого вызывающим предела.

`fvm_swf_stream_sound_snapshot_storage_info(snapshot, out)` копирует
[`FVM_SWFStreamSoundStorageInfo`](../include/flashvm/swf_stream_sound.h#L77):

| Поле | Смысл |
| --- | --- |
| `struct_size` | Точный нативный `sizeof`, задаваемый вызывающим |
| `retained_bytes` | Объект снимка, живой индекс и удерживаемые исходные либо PCM-массивы |
| `peak_bytes` | Максимальная одновременная учтённая память объекта/индекса/encoded/decoded при подготовке |

Для raw и неизменённого PCM значения равны. После ADPCM сжатое хранилище удаляется
до возврата: разность peak и retained равна размеру encoded payload. Учитывается
и владение пустым снимком. Значения зависят от нативной архитектуры и не являются
переносимой wire-схемой. Запрос не выделяет память, не вызывает callback/VM/device,
работает после уничтожения исходного контекста/runtime. Null snapshot/output либо
неверный `struct_size` дают INVALID_ARGUMENT, сохраняя output. Параллельные const
читатели допустимы после безопасной публикации; закрытие синхронизируется отдельно.
Снимок должен жить до возврата запроса и закрывается один раз после всех читателей.

Это детерминированный учёт запрошенных размеров владения, **не RSS**. Не учитываются
метаданные/выравнивание allocator, стек, исходный SWF в VM, управляемая куча,
таблица сегментов host, другие снимки, retry-буферы и память устройства. Массивы
при подготовке имеют точный начальный размер: скрытого растущего worker-кеша нет.

## Общий бюджет нескольких снимков

Задайте бюджет группы и заранее вычтите таблицу сегментов/другие host-буферы,
которые хотите учитывать. В порядке подготовки передавайте оставшийся бюджет
новому снимку. После успеха запросите размеры и вычтите **retained**, а не peak,
перед следующим источником. При реальном получении удерживайте предыдущие снимки.
Тогда временный пик следующего снимка вместе с удерживаемыми предшественниками
не превышает потолок группы. При ошибке позднего источника сначала закройте все
предыдущие снимки, затем освободите их host-таблицу. Это не откат посторонних
эффектов host/VM.

Предварительная проверка может повторять ту же арифметику, сразу закрывая временный
снимок, но виртуально сохраняя его charge. При старте копирование/декодирование
повторится, зато скрытый удерживаемый кеш не нужен, а бюджет получения будет тем
же. Это не общий предел памяти event voices, окон либо всего процесса.

## Исполняемый пример и проверки

[Учебный C89-пример](../examples/multimedia/swf_stream/README.md) задаёт предел
один МиБ в подготовленных режимах. `OUTPUT.pcm --storage` уничтожает входную VM,
запрашивает размеры, печатает нативный отчёт и экспортирует прежний точный PCM
независимым курсором. Аудиоустройство и внешние декодеры не нужны.

```sh
cmake --build build --target flashvm_examples_multimedia_swf_stream_main flashvm_tests_swf_stream_storage
./build/flashvm_examples_multimedia_swf_stream_main output.pcm --storage
./build/flashvm_tests_swf_stream_storage
python3 scripts/check_swf_stream_example.py build/flashvm_examples_multimedia_swf_stream_main
```

`swf_stream_storage.cpp` проверяет точный/на байт меньший/нулевой бюджет,
независимый runtime-предел, raw/normalized-размеры, неизменённый PCM, сохранение
output при ошибке и запрос/чтение после уничтожения VM. Fault-fixture использует
bounded API и запрашивает размеры с нулевым разрешением аллокаций после teardown.
C89 header/link-проверка охватывает обе функции. Это не измерение RSS allocator,
приёмка динамиков либо исполнение на непроверенных платформах.
Проверки ревизии: прошли все 21 выбранный нативный core stream/ADPCM-тест и проверки
C89-примера, включая точный FFmpeg-оракул. Изменённые модули и storage/ADPCM-fixture
прошли ASan/UBSan; бюджеты 0–4 проверялись в отдельных процессах. Перечисление
утечек здесь недоступно; вся VM/зависимости под sanitizer не пересобирались.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:73](../include/flashvm/swf_stream_sound.h#L73) |
| `fvm_swf_stream_sound_snapshot_open_pcm_bounded` | function | [include/flashvm/swf_stream_sound.h:88](../include/flashvm/swf_stream_sound.h#L88) |
| `fvm_swf_stream_sound_snapshot_storage_info` | function | [include/flashvm/swf_stream_sound.h:93](../include/flashvm/swf_stream_sound.h#L93) |
| `FVM_SWFStreamSoundStorageInfo` | struct | [include/flashvm/swf_stream_sound.h:77](../include/flashvm/swf_stream_sound.h#L77) |
<!-- source-links:end -->
