# Снимки звуковых событий корня — v0.637

[English](SOUND_SNAPSHOT.md). Дополнение к [доставке звука MovieClip](MOVIECLIP_SOUND.ru.md).
[`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L125) доставляет root StartSound через
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21); изменения не открывают аудиоустройство.

Private `sounds` кадра — плотный список от нуля, ограниченный
[`FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS`](../src/standard/movieclip_sound_constants.h#L8) (65536). Снимок скалярных индексов читается
по ordinal, независимо от порядка вставки в hash table. Каждое значение должно быть
целым неотрицательным Number ниже прежней границы, вычисленной из размера source.
Пропуски, неверные типы, NaN/infinity, дробные/выходящие за пределы числа и слишком
большой список дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до первого host callback. Усечённый
плотный префикс был структурно допустим в v0.637–v0.639. Начиная с v0.640 ниже
дополнительно проверяются принадлежность кадру, полнота и уникальность.

Весь native vector готовится до внешних эффектов. Поздняя ошибочная запись больше
не обнаруживается после уже доставленного раннего события. OOM снимка даёт
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) без доставки этого пакета. После начала callbacks прежние
эффекты сохраняются при последующей ошибке. Приоритет сравнения source и политики
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) при отсутствии callback сохраняется. Пустой допустимый список
при зарегистрированном callback ничего не доставляет. Сравнение полного hex-source
по-прежнему линейно по размеру файла.

[GC:] Действующий native barrier tick удерживает managed metadata/source во время
синхронной доставки. Снимок хранит только скопированные числовые индексы и
освобождается при любом выходе. Busy guard блокирует замену callback/source и
повторный advance, восстанавливаясь при ошибках и исключениях. Host callback/userdata
заимствованы и должны жить до отмены регистрации или уничтожения контекста.
Контракт callback по-прежнему запрещает правку clip/frame data и исполнение script.
Тесты намеренно повреждают private metadata для проверки устойчивости; это не
разрешение таких операций в host callback.

Вход root кадра помечается выполненным до подготовки звука. Ошибка capture/source/
callback/OOM останавливает его поздние actions, не откатывая сцену и не повторяя
звук автоматически. Остановленный кадр остаётся выполненным; играющий timeline
может перейти к другому кадру. Для повтора нужен явный host seek/recovery.
Интерпретатор и native JIT используют один путь доставки.

Новые проверки: `sound_snapshot_order` (SWF6–8, порядок вставки, настоящий native
JIT, защитный снимок при изменении полей/списка, эффекты до ошибки),
`sound_snapshot_invalid` (полная локальная структурная/числовая предвалидация),
`sound_snapshot_allocation` (все границы allocations, GC/guards, счётчик выделений).
Прежние sound delivery/validation/transport/allocation tests и
[C89 mixer tutorial](../examples/multimedia/swf_event_mixer/README.md) сохраняют
значение; пример использует публичные API и не изменяет metadata.

Все стадии роудмапа открыты. Это не подтверждает scheduling оригинального Player,
аудиоустройство, другие ОС или полную совместимость SWF8.

## Бюджет доставки (v0.638)

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) охватывает сравнение source, захват ordinal и доставку,
наследуя внешний бюджет AVM1/tick и вложенную кооперативную работу host без сброса.
Расходуется единица перед сравнением каждого канонического байта, перед копированием
каждого ordinal и перед callback. Для N байтов source и K событий стоимость
v0.638–v0.639 равна N+2K без внешнего timeline и host queries. Дополнение v0.640
описано ниже. Чисто визуальный root без sound-source marker/list не добавляет работы.

Исчерпание даёт [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) до следующей native единицы; насыщенный счётчик не
обнуляется и не переполняется. Ошибка подготовки не вызывает sound callbacks;
ошибка доставки сохраняет выполненные callbacks. Несовпадение source может
обнаружиться после оплаты сравнённого префикса. Конечный бюджет может остановить
сравнение раньше проверки отсутствующего callback. Нулевой числовой лимит сохраняет
периодическую остановку worker. Время произвольного host callback не прерывается;
это единицы логической работы. Потребление входа кадра и явный seek/recovery прежние.

`sound_snapshot_budget` проверяет все точные границы, вложенную host работу и
насыщение унаследованного счётчика. `sound_snapshot_worker` использует два настоящих
OS workers: остановку на первой единице сравнения source и остановку после первого
callback канонического пакета из 200 событий. Это software notifications, не
прослушивание звука или доказательство scheduling оригинального Player.

## Бюджет метаданных (v0.639)

Allocation-free read-only запросы [`fvm_swf_sound_count`](../include/flashvm/swf_sound.h#L90), [`fvm_swf_sound_info`](../include/flashvm/swf_sound.h#L94),
[`fvm_swf_sound_at`](../include/flashvm/swf_sound.h#L92), [`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L60), [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L62) и
[`fvm_swf_sound_event_envelope`](../include/flashvm/swf_sound.h#L64) теперь разделяют native work и остановку worker
с вызывающим кодом. `sound_at` удерживает один scope для обоих обходов; event lookup
удерживает его при обходе событий и поиске выбранного ID. Внешний host query начинает
новый бюджет, запрос из sound callback наследует бюджет доставки. Другие составные
decoder/output services не получают полного aggregate-work контракта автоматически.

Обход definitions расходует единицу за root tag и за DefineSound header. Обход
событий — за root tag, StartSound header и каждую точку огибающей. При T tags,
D definitions, E events, P envelope points: count/info definitions стоят T+D,
успешный definition-at — 2(T+D), event count — T+E+P, успешный event-at/envelope
добавляет T+D для проверки уникального выбранного ID. Ранний malformed/missing
выход может стоить меньше. Чтение полей постоянного размера и пропуск непрозрачных
payload не оплачиваются побайтно и не декодируют codec. Бюджет может исчерпаться
до диагностики позднего повреждённого record; частичный результат не публикуется.
Нулевой числовой лимит сохраняет кооперативную проверку остановки worker.

Все шесть функций требуют живого контекста в потоке владельца вне terminal GC.
Count требует ненулевой size_t output; record/envelope — точный struct_size.
ID definitions — ненулевой UI16, ordinal и point начинаются с нуля. Нет source —
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46); нет выбранного значения — [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47); повреждённый stream,
повтор выбранного ID или плохие аргументы — [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); бюджет/stop —
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Все ошибки сохраняют outputs, включая count и все байты structs.
Event count проверяет syntax без разрешения каждого dictionary ID. Event/envelope
копируются; `FVM_SWFSoundInfo.data` заимствован до успешной перезагрузки source или
уничтожения контекста. Новых callbacks, playback, mutations и allocations нет.

`sound_metadata_budget` проверяет все границы шести routes для SWF1–8, вложенность,
насыщение и неизменность outputs. Независимый wire fixture содержит пять tags,
один definition, два events, две точки envelope. `sound_metadata_worker` использует
шесть настоящих OS workers при нулевом лимите. `sound_metadata_recovery` проверяет
позднее повреждение envelope/повтор и отсутствующий ordinal;
`sound_metadata_allocation` запрещает любые allocations для всех шести queries.

## Канонический кадр (v0.640)

Root с `sound_source` теперь проверяет каждый фактический вход кадра по совпадающему
каноническому SWF, даже если frame record/sounds table удалены. Один независимый
обход root stream собирает глобальные индексы StartSound нужного кадра. Захваченный
managed list должен точно совпадать с этой native последовательностью. Индексы
другого кадра/несуществующие, перестановки, дубликаты, лишние/пропущенные элементы,
очистка/удаление таблицы и удаление frame record дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до
первого host callback. Порядок вставки корректных ordinal slots в hash table свободен.
Снимок по-прежнему поверхностный; защита не разрешает правку metadata в callback.

Private helper [`fvm_swf_sound_frame_ordinals`](../src/swf/sound_event_index.hpp#L8) проверяет framing/SOUNDINFO, разделяет
metadata work и собирает не более [`FVM_SWF_SOUND_MAX_FRAME_ORDINALS`](../src/swf/sound_event_constants.h#L12) элементов.
Выходной vector изменяется только при успехе; parse/work/OOM/cap failure сохраняет
прежний vector и освобождает частичную native память. Helper не разрешает каждый
resource ID, не декодирует codec и не кеширует context/source pointers. Прежняя
проверка ресурсов обеспечивается импортом. Это не аутентификация произвольного
графа после намеренного удаления всех идентифицирующих source metadata. Чисто
визуальные графы без звуковых metadata сохраняют независимый импорт.

Для корректного sound-bound входа с N байтами source, T root tags, E StartSounds,
P envelope points и K выбранными events стоимость равна N+4K+T+E+P плюс внешняя
работа и host queries. Сбор индекса и сравнение добавляют по единице на event;
полного обхода файла K раз нет. Канонически пустой кадр также сравнивает/разбирает
свой source, но не требует callback. Непустой проверенный кадр без callback даёт
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46); плохой source/list или бюджет могут завершиться раньше этой
диагностики. Перезагрузка постороннего source может отказать даже на пустом кадре
sound-bound root; host должен сохранять соответствие source. Native scratch имеет
per-frame caps, а не новую общую гарантию memory budget. Потребление входа кадра,
ранее выполненные visual/host effects и явный seek/recovery прежние; нового
whole-movie lifecycle scheduler нет.

`sound_frame_binding` проверяет все подмены, удалённые records и empty/callback
политику. `sound_canonical_index` проверяет точный бюджет, неизменность outputs,
поздние плохие envelopes и полный лимит. `sound_canonical_index_allocation`
перебирает allocations. `sound_binding_graph` переносит SWF6–8 сериализацией и
memory channel, уничтожает source context, собирает traced strings, проверяет
отказ и явную правку/повтор с callbacks получателя в interpreter/native JIT.
Прежний полный allocation sweep доставки также охватывает подготовку индекса.

[`fvm_movieclip_define_frame`](../include/flashvm/movieclip.h#L110) сохраняет идентичность звуковой таблицы корректного
импортированного кадра при замене actions/label. Новая запись готовится до
публикации; OOM сохраняет прежний кадр/таблицу. Это позволяет допустимой замене
actions не терять звук и не нарушать каноническую полноту. Уже повреждённые metadata
не восстанавливаются; прежняя политика замены visual controls/других полей не
меняется. Повторный вход заменённого текущего кадра снова оценивает звук.
`sound_define_frame_allocation` перебирает OOM замены, binding test проверяет
успешную замену actions с сохранёнными каноническими звуковыми событиями.

Поле повторного входа текущего кадра также готовится до публикации. Если оно уже
существует, после публикации выполняется скалярное присваивание без allocations;
если отсутствует — заранее готовится заменяющая state map. Поэтому OOM при
добавлении `entered` больше не возвращает ошибку после уже видимой замены кадра.
Контрольная сборка с прежним порядком публикации воспроизводит это в OOM-тесте.

## Проверки серии (v0.637–v0.640)

Прошли 141 minimal Linux Release CTest и 141 matching whole-core heap/global ASan +
UBSan CTest. Финальный sanitizer run последовательный; stack instrumentation и
LeakSanitizer отключены. Проверены 14 новых regression executables, прежние scene/
sound/input/frame/initializer/timer/JIT tests, реальные PCM tutorial modes/files и
строгие C89 headers. Финальные бинарники согласованы с ядром; права исполнения
исправлены до финальных прогонов. Контрольные старые исходники воспроизводят ошибки.
[Доказательства и границы](ROADMAP_STATUS.md). Ни одна стадия не закрывается;
устройства и другие ОС этими software-проверками не сертифицируются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:125](../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_define_frame` | function | [include/flashvm/movieclip.h:110](../include/flashvm/movieclip.h#L110) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS` | macro / define | [src/standard/movieclip_sound_constants.h:8](../src/standard/movieclip_sound_constants.h#L8) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:92](../include/flashvm/swf_sound.h#L92) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:90](../include/flashvm/swf_sound.h#L90) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:62](../include/flashvm/swf_sound.h#L62) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:60](../include/flashvm/swf_sound.h#L60) |
| `fvm_swf_sound_event_envelope` | function | [include/flashvm/swf_sound.h:64](../include/flashvm/swf_sound.h#L64) |
| `fvm_swf_sound_frame_ordinals` | function | [src/swf/sound_event_index.hpp:8](../src/swf/sound_event_index.hpp#L8) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:94](../include/flashvm/swf_sound.h#L94) |
| `FVM_SWF_SOUND_MAX_FRAME_ORDINALS` | macro / define | [src/swf/sound_event_constants.h:12](../src/swf/sound_event_constants.h#L12) |
| `FVM_SWFSoundInfo` | struct | [include/flashvm/swf_sound.h:14](../include/flashvm/swf_sound.h#L14) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
