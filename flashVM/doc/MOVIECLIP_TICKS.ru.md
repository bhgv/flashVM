# Ticks MovieClip и необязательные обработчики — v0.624

[English](MOVIECLIP_TICKS.md). [fvm_movieclip_advance](../include/flashvm/movieclip.h#L66) синхронно исполняет ticks в потоке
владельца контекста: начальные блоки, переход кадра, действия, onEnterFrame и обход
снимка детей по глубине. Остановленный и однокадровый клип также получает обработчик.
Здесь нет poll таймеров, растеризации или показа устройства; отдельный контракт
[перерисовки](UPDATE_AFTER_EVENT.ru.md) сохраняется.

Исправление v0.629: прежняя реализация копировала insertion-ordered hash table,
несмотря на обещанный порядок depth. Теперь identity проверяются, глубины сортируются
с бюджетом, effective removed ancestors подавляют потомков.
[Порядок, пределы и новая стоимость](TIMELINE_DEPTH_ORDER.ru.md). Childless tick
по-прежнему стоит две единицы; сцена с детьми дополнительно оплачивает sort/query.

## Общий бюджет и частичное выполнение

На весь вызов теперь создаётся один [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9). Все ticks, потомки,
вложенные advance и AVM1 callback используют общий внешний instruction/work-бюджет.
Счётчик сбрасывает только внешний host-вызов. Ноль в [FVM_Config](../include/flashvm/flashvm.h#L51).instruction_limit
отключает числовой предел, но не кооперативную остановку worker. Исчерпание бюджета
или обнаруженная остановка возвращает [FVM_ERROR](../include/flashvm/flashvm.h#L43), а не script exception.

Нативные единицы: вход в узел (также static art), необязательный обработчик, каждая
запись снимка initializers/action segments/children и каждая доставка initializer
или отдельного action segment. AVM1-инструкции сохраняют свой учёт. Записи детей
оплачиваются до выделения снимка. Однокадровый клип без scripts и детей расходует
две единицы на tick, даже без обработчика. Ноль ticks ничего не исполняет.
Пределы [FVM_MOVIECLIP_MAX_TICKS_PER_CALL](../src/standard/movieclip_constants.h#L26) и [FVM_MOVIECLIP_ADVANCE_MAX_DEPTH](../src/standard/movieclip_constants.h#L23)
ограничивают размер запроса и глубину; превышение даёт [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45).

Это границы учёта и кооперативной остановки, а не real-time preemption. Внутренняя
работа scene placement/resources, native callback, поиска свойств, выделения памяти,
рендеринга и прочих неоплачиваемых helpers не становится полностью ограниченной.
Нативный callback должен сотрудничать сам. Остановка worker проверяется на первой
единице и затем через каждые 64 единицы согласно существующей политике.

Отката нет: кадр уже мог измениться, initializer/action/handler мог исполниться до
[FVM_ERROR](../include/flashvm/flashvm.h#L43), [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) или [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48). Остаток обработчиков, детей и ticks
не исполняется. Следующий вызов работает с опубликованным состоянием, без скрытого
курсора продолжения пакета. Автоматически повторять весь неудачный пакет нельзя.

## Поиск метода и изменение сцены

onEnterFrame использует [fvm_object_call_optional](../src/capi/value_c.cpp#L1368), как другие явные события.
Собственные/унаследованные Function, getter и зарегистрированные native methods
получают исходный клип как receiver. Сохраняются владелец определения и метаданные
superclass; getter вызывается один раз. Отсутствующее или невызываемое свойство
игнорируется; ошибка настоящего getter/handler передаётся вызывающему. Одноимённая
global function не подставляется. Действуют обычные правила методов, включая SWF
case/resolver, без extension metatable read fallback. Нативные и script Function
вызывались и раньше: это не новая поддержка AS Function и не полная правка всех
правил super оригинального Player.

Scene/actions/handler родителя исполняются до снимка его детей. Ребёнок, созданный
родительским handler, может получить tick сразу. Созданный более поздним дочерним
handler ребёнок не добавляется в уже взятый снимок родителя. Запись снимка с другим
физическим родителем пропускается. Порядок — native child depth, без утверждения
полного совпадения с оригинальным Player. Невидимость не останавливает timeline.
Полные ClipActions/load/unload и автоматический input hit-target fanout не добавлены.

## Владение и ошибки

API заимствует same-context клип: удерживайте его pin/другим корнем между host-вызовами.
Работайте в потоке владельца контекста. Null/чужой клип, destroying/collecting/releasing,
reentry существующего sound callback и слишком большой ticks дают
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до входа в бюджет. Ноль ticks также проверяет эти аргументы.
Ошибка глубины обнаруживается в обходе и может следовать за выполненной работой.

[GC:] [FVM_NativeCallGuard](../src/core/native_call_guard.hpp#L9) остаётся активным на весь advance и callback. Запрос сборки
внутри откладывается существующей политикой GC. Эта версия не разрешает сборку
сырого снимка сцены во время callback. Области roots/owner/work восстанавливаются
при нормальном выходе, ошибках и OOM. Заимствованный userdata должен жить до снятия
регистрации/уничтожения контекста. Перенос графа сохраняет поддерживаемое managed
состояние, но не активный work scope и не native stack snapshot.

## Проверки и пример

[Dispatch](../tests/movieclip_tick_dispatch.cpp) проверяет identity владельца, getter,
native methods, пропуск отсутствующих/невызываемых свойств, отсутствие global fallback,
ошибки/исключения и lifecycle phases. [Budget](../tests/movieclip_tick_budget.cpp)
проверяет точные единицы, zero ticks, staging детей и вложенные host-вызовы.
[Frame actions](../tests/movieclip_tick_actions.cpp) проверяет общий предел между
отдельными scripts в interpreter/native JIT. [Mutation](../tests/movieclip_tick_mutation.cpp)
проверяет добавление ребёнка родителем, удаление sibling и deferred newborn из child.
[Workers](../tests/movieclip_tick_worker.cpp) использует реальные OS threads с
отключённым числовым пределом. [OOM](../tests/movieclip_tick_allocation.cpp) проходит
отказы выделения и последующий рабочий вызов/GC.
[Graph](../tests/movieclip_tick_graph.cpp) вызывает импортированный AVM1 handler после
уничтожения отправителя и traced-string GC. [JIT](../tests/jit_movieclip_tick.cpp)
сверяет inherited superclass calls для SWF6–8 в interpreter/manual/automatic native
режимах с контролем native entries.

Временная программа с movieclip.cpp версии v0.623 отвергает новые контроли метаданных
владельца и общего бюджета. Superclass-call тест проходит и на старом коде и является
только регрессией. Это не эталонная совместимость и не приёмка устройств иных ОС.

[C89-пример](../examples/embedding/movieclip/README.ru.md) останавливает однокадровый
клип, запрашивает три ticks с бюджетом пяти единиц, сохраняет два callback после
ошибки и получает новый бюджет следующим внешним вызовом. Соседний AS2-файл остаётся
отдельным эскизом для внешнего компилятора. Все девять этапов роудмапа открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_ADVANCE_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:23](../src/standard/movieclip_constants.h#L23) |
| `FVM_MOVIECLIP_MAX_TICKS_PER_CALL` | macro / define | [src/standard/movieclip_constants.h:26](../src/standard/movieclip_constants.h#L26) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `fvm_object_call_optional` | function | [src/capi/value_c.cpp:1368](../src/capi/value_c.cpp#L1368) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
