# Снимки глубин native timeline — v0.629

[English](TIMELINE_DEPTH_ORDER.md). [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) теперь действительно
обходит детей по возрастанию native depth. Прежняя документация tick обещала это,
но копия hash-таблицы обходилась в порядке вставки. Исправлен порядок и проверка
эффективного удаления ещё не посещённых потомков.

## Локальный snapshot и порядок

Для каждого nonstatic клипа сохраняется последовательность: frame/scene actions,
retained EnterFrame records, optional onEnterFrame method, затем подготовка детей.
Root идёт перед потомками. У каждого родителя ключи depth сортируются по возрастанию,
после чего дети обходятся depth-first. Signed public depths используют общий bias
родителя, поэтому отрицательные глубины идут раньше неотрицательных. Legacy
nonnegative slot tables сохраняют свою native ordering policy.

Snapshot локален для одного родителя и готовится после его callbacks. Рождённый
родительским handler ребёнок может получить текущий tick. После подготовки identity
и depth order фиксируются: newborn или замена на той же глубине из sibling callback
ждут следующего snapshot родителя. Перенос существующего ребёнка внутри того же
родителя не меняет уже захваченную очередь; следующий tick увидит новую глубину.
Собственное поддерево ребёнка готовится позднее и видит изменения до его очереди.
Общего whole-tree snapshot или глобального once-per-tick registry здесь нет.

Копируются значения, а не только ключи с последующим чтением живой таблицы. Замена
на захваченной глубине не выдаётся за старого получателя. До первого child callback
проверяется вся локальная таблица: native MovieClip того же контекста, private state
того же владельца, без повторных identity или self-child. Malformed/foreign/nonclip/
alias дают INVALID_ARGUMENT. Родитель уже мог исполниться; его эффекты не откатываются.
Перед очередью снова проверяется physical parent; moved/detached дети пропускаются.

Непосредственно перед входом в ребёнка проверяется effective native removal с учётом
предков. Удаление ветви в её onEnterFrame подавляет pending descendants, даже если
их physical parent edges ещё указывают на удалённую ветвь. Это не scheduler onUnload.
Явный выбор уже удалённого root сохраняет прежнюю direct-root policy; она не делает
его удалённых потомков обычными участниками сцены. Уже начатые действия получателя
не откатываются из-за последующего удаления.

## Общая работа и пределы

[depth_order.hpp](../src/core/depth_order.hpp) содержит общий C++98 heap sort для
нового timeline path и прежних retained scene-input routes. STL и квадратичная
insertion sort не добавлены. Каждый sift comparison block и extraction списывает
работу; стоимость сортировки input API сохранена. Как раньше, child entries
предварительно списываются до allocation snapshot. Дополнительная единица теперь
полагается на каждый выполняемый eligibility query. AVM1, parent callbacks, nested
calls и несколько ticks по-прежнему делят outer budget. Childless single-frame tick
остаётся двухъединичным; zero ticks не делает работы.

Каждая post-handler таблица ограничена [`FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L32)
(65 536 slots), включая static artwork и stale links. Oversized отвергается до
сканирования/копирования таблицы. Прежние maximum depth и ticks per call сохраняются;
нового общего node cap на весь multi-tick call нет. Removal query использует свой
прежний ограниченный ancestry walk: единица query не считает каждую внутреннюю
операцию property/hash lookup.

Исчерпание numeric budget или кооперативный worker stop даёт ERROR, allocation —
OUT_OF_MEMORY. Настоящие callback errors/exceptions не маскируются. Эффекты родителя
и ранних детей сохраняются; остальные дети/ticks пропускаются. Следующий outer call
работает с committed state и свежим бюджетом, без скрытого resume cursor. Native
callbacks/allocations не прерываются посередине.

[GC:] Прежний внешний native barrier удерживает copied receiver identities/code
после removal/replacement. Collect запрещён, incremental step не собирает. Vectors,
локальная alias table и work scopes не сериализуются. Обычный graph transport
сохраняет native depths и методы для восстановления порядка. Каждый посещаемый узел
также проверяет native/context state до перехода по нему.

Начальные edges frames/children отдельно проверяются на null и владельца контекста
до parent callbacks. Такие ранние ошибки не исполняют receiver callbacks; отдельные
неверные slots проверяются позднее, при подготовке snapshot.

## Проверки и пример

- [Depth order](../tests/movieclip_tick_depth_order.cpp): host и SWF5–8, перемешанная
  вставка, negative depths, root-first DFS и повторные ticks.
- [Identity](../tests/movieclip_tick_snapshot_identity.cpp): same-depth replacement,
  смена глубины из sibling callback и новая очередь следующего tick.
- [Removal](../tests/movieclip_tick_detached_branch.cpp): удалённый ancestor при
  сохранившихся child links не доставляет pending descendants.
- [Invalid](../tests/movieclip_tick_snapshot_invalid.cpp) и
  [bounds](../tests/movieclip_tick_snapshot_bound.cpp): неверные values/owners,
  aliases, self/nonclip и oversized snapshot до child callbacks.
- [Private edges](../tests/movieclip_tick_private_edges.cpp): null/foreign containers
  и чужой native resource state отвергаются до небезопасного перехода.
- [Budget](../tests/movieclip_tick_depth_budget.cpp),
  [OOM](../tests/movieclip_tick_depth_allocation.cpp), прежние tick workers:
  сортировка, partial ordered completion, allocation sweep и восстановление guards.
- [Общая сортировка](../tests/depth_order.cpp): odd/even, все малые rotations,
  duplicates и точные границы работы без library sort oracle.
- [AVM1 graph/JIT](../tests/movieclip_tick_depth_graph.cpp): decimal trace 123 после
  перемешанной вставки, sender teardown, traced-string GC и настоящие native entries
  в SWF5–8 контекстах.

Отдельный source-linked control со старым v0.628 movieclip.cpp дал
`root, high, low, middle`; новый код на том же host graph дал
`root, low, middle, high`. Это доказывает insertion-order defect, не полную
эквивалентность оригиналу. [Strict C89 tutorial](../examples/embedding/movieclip/README.ru.md)
сохраняет пятиъединичный пример и добавляет независимый finite-budget runtime с
выводом `depth order: low middle high` после shuffled construction.

Прежний clone-allocation regression покрывает десять version/entry-route комбинаций
и тысячи allocation boundaries. Его default 30 секунд не хватило инструментированному
прогону; CTest теперь даёт ему те же ограниченные 180 секунд, что другим exhaustive
allocation matrices. Assertions и sweep не ослаблены. Старые объектные файлы до
v0.628 из восстановленного cache пересобраны перед финальными проверками: смешение
их с текущей private context layout не является корректным runtime result.

Минимальный повтор из корня независимого компонента не требует SDL или устройства:

```sh
cmake -S . -B build-depth -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DFVM_BUILD_EXAMPLES=ON
cmake --build build-depth --target flashvm_tests_depth_order flashvm_tests_movieclip_tick_depth_order flashvm_examples_embedding_movieclip_main
ctest --test-dir build-depth -R '^(flashvm_tests_depth_order|flashvm_tests_movieclip_tick_depth_order|flashvm_examples_embedding_movieclip_main)$' --output-on-failure
```

Это Linux core/timeline evidence. Raster child order, общий lifecycle/hit scheduler,
оригинальный Player, физические устройства и foreign-platform acceptance — отдельные
условия. Все девять стадий остаются открытыми.


Проверка версии: успешно 172 выбранных Release CTest и 112 whole-core ASan/UBSan
CTest на Linux. Финальный sanitizer suite шёл последовательно, leak detection
выключен. Clone-allocation matrix завершился за 81,61 с. Strict C89 example также
прошёл с инструментированным ядром. [Статус и границы приёмки](ROADMAP_STATUS.md).

v0.630: raster traversal now has a separate [EN](RASTER_DEPTH_ORDER.md) / [RU](RASTER_DEPTH_ORDER.ru.md) contract; v0.629 evidence above remains historical.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:32](../src/standard/movieclip_constants.h#L32) |
<!-- source-links:end -->
