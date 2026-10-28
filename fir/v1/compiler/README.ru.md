# Компилятор FIR 0.3.2 и C API

Компилятор создаёт объектники `.swo`, не SWF. Линкер — независимый проект
[SWL](../../../linker/README.ru.md). Оптимайзер выполняется отдельно или по
явному `FirRunOptions.optimise`; консольные приложения вызывают тот же C API.

```sh
build/fir1/fir1_source as2 --input a.as a.fir --module a
build/fir1/fir1 check a.fir
build/fir1/fir1 print a.fir canonical.fir
build/fir1/fir1_opt a.fir optimized.fir
build/fir1/fir1 object optimized.fir a.swo
build/linker/swl link movie.swf a.swo
```

[Встраиваемый API](API.ru.md), [формат SWO1](../../../linker/FORMAT.ru.md),
[покрытие](../doc/COVERAGE.ru.md). `fir1 swf`, `fir1 actions` и `fir_swf`
удалены; скрытого обхода линкера больше нет. `fir1_source` поддерживает числовые
учебные профили AS1/AS2/AS3/Limbo, не полные языки. Аннотации AS2/AS3 — Number;
SWF-цель понижает числовой AS3 в AVM1, не ABC; прямая Dis-цель принимает явное f64-подмножество. Имена модулей при связывании должны различаться.

Источники: [fir1.atg](fir1.atg), [src1.atg](src1.atg), [lower.c](lower.c),
[source_api.c](source_api.c), [pipeline.c](../src/pipeline.c),
[object.c](object.c), [верификатор](../src/ir.c).
Парсеры генерируются внешним bhgv/coco-r-C в build. Wrappers отдельно сбрасывают
ошибки и переводят фатальные пути сканера в возвращаемые ошибки. Namespace
позволяет связать парсеры в одну программу, но вызовы одного парсера нужно
сериализовать. Модули IR достаточно большие — выделяйте их в heap.

Потоки принадлежат вызывающему коду; NULL/0 допустим для диагностики. `fir_run`
принимает задачу, потоки и настройки, `fir_optimise` работает с модулем напрямую,
`fir_object` проверяет модуль до генерации SWO1. Версии API/структур пока внутренние,
стабильный бинарный ABI не обещается. CLI: 0 успех, 1 ошибка pipeline/вывода,
2 неверные аргументы/начальная ошибка открытия. Ошибка валидации не портит
существующий выход; ошибка записи может оставить частичный файл.

[Рабочий source-синтаксис](../doc/SOURCE_SYNTAX.ru.md).

[Backend Dis 0.9.0: скалярный CFG со значениями и флагами исполнения](../doc/DIS_BACKEND.ru.md).

[Нативная арифметика big и точная семантика оптимизатора](../doc/BIG_ARITHMETIC.ru.md).

[Numeric cells and structured source flow / Числовые ячейки и структурный CFG — FIR 0.57](../doc/NUMERIC_FLOW.md) · [Русский](../doc/NUMERIC_FLOW.ru.md).
