# Снимки действий кадра — v0.631

[`fvm_exec_call_frame`](../include/flashvm/execution.h#L342) — публичный синхронный сервис ActionCall (0x9e) для потока
владельца контекста. Он заимствует принадлежащий контексту scope receiver (для
не-MovieClip используется `_root`) и допустимый операнд [`FVM_Value`](../include/flashvm/value.h#L24). Числовой
селектор приводится к Uint32, остальные — к строке по правилам AVM1. Counted-путь
`clip:frame` разрешается до выбора действий. Отсутствующий target/кадр — no-op;
неподдерживаемая slash-only форма даёт [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). Внутренний
[`fvm_movieclip_call_frame`](../src/standard/movieclip.cpp#L3377) получает уже выбранный клип и число/строку.
Оба пути не перемещают playhead, не применяют scene controls, не вызывают onEnterFrame,
не опрашивают таймеры и не двигают детей. Контекст и аргументы сохраняются до возврата.

Раньше прямой нативный вызов мог начинать instruction budget заново для каждого
DoAction, а tick имел собственный механизм копирования. Теперь оба используют
[`snapshot_frame_blocks`](../src/standard/movieclip.cpp#L2350). При отсутствии `segments` выбирается одно значение `code`.
Если поле присутствует, оно является основным и должно указывать на объект того же
контекста с максимум [`FVM_MOVIECLIP_MAX_FRAME_BLOCKS`](../src/standard/movieclip_constants.h#L23) (65536) индексированными слотами.
Ключи обязаны составлять плотную последовательность 0..N-1; порядок добавления не
влияет на исполнение. Пустой список допустим. Старое placeholder-поле `code` при
наличии `segments` больше не требуется. Все значения копируются до первого блока:
замена/удаление исходной таблицы из callback не меняет текущий снимок. Следующий
вызов видит замену. Операндный стек и локальное исполнение каждого блока независимы.

Null/чужой контейнер кадров, выбранная запись или объект сегментов, дырки/неверный
диапазон ключей и превышение границы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до первого выбранного
действия. Код декодируется в момент исполнения блока: неверный тип, нечётный/неверный
hex или некорректный AVM1 позже в списке могут дать ошибку после эффектов ранних блоков.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), ошибки исполнения и исключения передаются вызывающему. Эффекты
coercion и уже исполненных действий сохраняются; tick до ошибки снимка также мог
изменить playhead/display list и вызвать sound notification. Автоматического отката
или повтора нет.

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) сохраняет общий внешний instruction/work budget для coercion
операнда в публичном сервисе, разрешения target, подготовки кадра и вложенных блоков.
Счётчик сбрасывается только при входе без активного execution/work scope. Сегментированный кадр списывает
единицу за копируемый ordinal и единицу за вход в блок, плюс обычные checkpoints AVM1
(включая End). Legacy ActionCall списывает одну единицу входа в блок; прежняя стоимость
legacy tick сохраняется. Строковый селектор классифицируется по единице за байт;
поиск нечисловой метки списывает единицу за каждую сохранённую запись кадра. Проверяются
все сохранённые метки: из точных дубликатов выбирается минимальный индекс кадра,
независимо от порядка добавления. Стоимость нативного frame call изменилась.

Нулевой instruction_limit отключает числовой предел, сохраняя существующие проверки
кооперативной отмены. Это ограниченная подготовка таблицы, а не новый побайтовый бюджет
для hex, всех сравнений меток или каждого target helper. Прежние границы размера одного
блока сохраняются. Slash-only селекторы, полные исторические case/version правила меток
и сравнение с оригинальным Player остаются открытыми.

[GC:] [FrameScope](../src/standard/movieclip.cpp#L3429) связывает копии значений и receiver с цепочкой корней, восстанавливая
debug cursor и вложенность, сохранённые на входе кадра, при любом возврате/исключении. Здесь намеренно разрешён
реальный script GC после удаления исходной таблицы callback-ом. У tick сохраняется
другое ограничение существующего native barrier. Указатели host callbacks не передаются
при сериализации; в получающем контексте их регистрируют заново.
[`FVM_MOVIECLIP_CALL_FRAME_MAX_DEPTH`](../src/standard/movieclip_constants.h#L29) именует прежнюю границу рекурсии 64 activation.
Конкурентный доступ к контексту и новый C ABI не добавляются.

Проверки охватывают private/public native входы, интерпретатор, настоящий native JIT
ActionCall и tick, перемешанные ordinal, повреждения, граничные бюджеты, пустые блоки,
вложенные вызовы/coercion, counted-дубликаты меток и отказы выделения. Импортированный
код передаётся в другой контекст, исходный уничтожается, затем таблица удаляется с
реальной traced-string сборкой между блоками.
[C89-пример](../examples/embedding/movieclip/README.ru.md) дважды исполняет кадр два
с отдельными бюджетами по пять единиц и проверяет playhead один и ноль tick callbacks.
[English contract and reproduction commands](FRAME_BLOCKS.md).

Все девять стадий открыты. Это FlashVM/Linux регрессии, а не сертификация Android,
Windows/macOS на физических устройствах или совместимости с оригинальным Player.

## Проверки v0.631

Прошли 196 выбранных Linux Release CTests с AGG/статическими зависимостями и
139 минимальных whole-core ASan/UBSan CTests последовательно, без raster backend
и с отключённым LeakSanitizer. Девять новых frame-регрессий проверены вместе с
прежними ActionCall/missing operand/JIT dispatch, timeline/graph/input/timers.
Строгий C89/pedantic-errors пример связан с санитайзерным ядром и выдал пять
документированных строк. Source-link и debugger-example проверки прошли.

Контроль с точным movieclip.cpp из v0.630 воспроизводит обход лимита: при limit 10
два блока по шесть инструкций завершаются с seen=2 и steps=6. Новый код возвращает
[FVM_ERROR](../include/flashvm/flashvm.h#L43), сохраняя seen=1 и steps=11. Checkpoint AVM1 учитывает попытку инструкции
до отклонения; native work отклоняется до увеличения счётчика на пределе.
Это сравнение реализаций FlashVM, а не доказательство original Player semantics.
Новый SDL/device/foreign-OS прогон и полная LeakSanitizer-приёмка не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_call_frame` | function | [include/flashvm/execution.h:342](../include/flashvm/execution.h#L342) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_call_frame` | function | [src/standard/movieclip.cpp:3377](../src/standard/movieclip.cpp#L3377) |
| `FVM_MOVIECLIP_CALL_FRAME_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:29](../src/standard/movieclip_constants.h#L29) |
| `FVM_MOVIECLIP_MAX_FRAME_BLOCKS` | macro / define | [src/standard/movieclip_constants.h:23](../src/standard/movieclip_constants.h#L23) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
