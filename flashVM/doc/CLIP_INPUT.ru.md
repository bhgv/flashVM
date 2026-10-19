# Сохраняемые input handlers дерева MovieClip — v0.626

[English](CLIP_INPUT.md). [fvm_movieclip_dispatch_clip_input](../include/flashvm/clip_actions.h#L53) рассылает по выбранному
native-поддереву одно глобальное retained-событие: [FVM_CLIP_EVENT_MOUSE_MOVE](../include/flashvm/clip_actions.h#L8),
[FVM_CLIP_EVENT_MOUSE_DOWN](../include/flashvm/clip_actions.h#L9), [FVM_CLIP_EVENT_MOUSE_UP](../include/flashvm/clip_actions.h#L10), [FVM_CLIP_EVENT_KEY_DOWN](../include/flashvm/clip_actions.h#L11) либо
[FVM_CLIP_EVENT_KEY_UP](../include/flashvm/clip_actions.h#L12). Требуется ровно один бит. Это дополнение к
[доставке выбранной записи](CLIP_ACTIONS.ru.md), а не замена Mouse/Key/focus API
или полная реализация планировщика событий Flash.

## Контракт C89

API заимствует context и root в потоке владельца. Удерживайте root через pin или
другой корень между вызовами. Host сначала публикует pointer/keyboard state, например
[fvm_input_dispatch_mouse](../include/flashvm/mouse.h#L29) или [fvm_input_dispatch_keyboard](../include/flashvm/keyboard.h#L82). Сам API не меняет input,
не двигает timeline, не делает timer poll/рендер, не выбирает focus и не вызывает
native onX. Публикация состояния и порядок namespace/tree принадлежат host.

Невидимость, stop и focus не фильтруют эти пять глобальных records. Static artwork
инспектируется, но не получает handlers. Узлы source SWF<5 обходятся без доставки,
поддерживаемые потомки остаются eligible. Native clips без библиотеки используют
существующую ambient/host version policy. Callback видит состояние host на момент
своего исполнения; его явные изменения state не откатываются и не замораживаются.

Null/чужой/nonclip/static-art root, lifecycle/sound phase, неверный/combined event и
recursive tree entry дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Проверка root и подготовка входят
в границу перехвата OOM: возвращается [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44), а не наружный bad_alloc.
Отсутствующие handlers — успех. Исполнение может вернуть ошибку/exception байткода.
Нет выходного счётчика, постоянного batch cursor или автоматического retry.

## Два уровня снимков

До первого callback захватывается полное поддерево: root первым, дети каждого
родителя отсортированы по native depth. Charged heap sort не зависит от порядка
вставки и не использует квадратичный insertion sort. Подготовка проверяет owner/type,
повторы/cycles; возможна нормализация legacy child-slot таблиц. Это внутреннее
изменение, не script callback и не rollback transaction. Ошибка capture не исполняет
пользовательские handlers.

Получатель хранит clip identity и индекс захваченного родителя. Перед вызовом
проверяются вся захваченная цепочка родителей и effective removed state. Удаление
или перенос ветви исключает ещё не вызванных потомков, даже если предок уже получил
свой callback. Новые дети ждут следующей рассылки. Изменение depth не переставляет
текущий пакет. Root остаётся явно выбранным объектом этого вызова; глобального
реестра всех display objects не создаётся.

На очереди каждого получателя [fvm_movieclip_dispatch_clip_actions](../include/flashvm/clip_actions.h#L37) фиксирует именно
его selectors/code. Ранний получатель может заменить таблицу позднего — поздний
увидит замену. Внутри выбранной записи действует прежний snapshot: изменение её
более поздних records уже не меняет prepared code. Это не один общий frozen-снимок
всех обработчиков при входе в дерево.

## Бюджет, ошибки и GC

Один [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) включает capture узлов, staging children, heap sort,
очередь получателей, сравнения ancestor chain, selected-record entry, байты actions,
доставку records и AVM1-инструкции. Вложенные вызовы делят внешний бюджет; свежий
счётчик создаётся только внешним host-вызовом. Пустой live native root без children/
handlers расходует три единицы. Числовой предел отключается независимо от worker stop.

| Граница | Назначение |
| --- | --- |
| [FVM_CLIP_INPUT_MAX_DEPTH](../src/standard/clip_input_constants.h#L5) | Глубина потомков выбранного root, root имеет глубину ноль. |
| [FVM_CLIP_INPUT_MAX_NODES](../src/standard/clip_input_constants.h#L6) | Все инспектированные native nodes, включая static art. |
| [FVM_CLIP_INPUT_MAX_CHILDREN](../src/standard/clip_input_constants.h#L7) | Число depth keys для сортировки одного родителя. |
| [FVM_CLIP_INPUT_MAX_RECIPIENTS](../src/standard/clip_input_constants.h#L8) | Число prepared live receivers. |

Превышение depth/count даёт [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до callbacks. Budget/обнаруженный
worker stop дают [FVM_ERROR](../include/flashvm/flashvm.h#L43). Поздние ошибки, OOM и exceptions сохраняют уже выполненные
callbacks, input state и redraw requests, пропуская остаток. Следующий вызов — новая
доставка, не продолжение cursor. Native helpers, hash, allocation и callback internals
не становятся hard real-time preemptible; действует прежняя charged-work stop policy.

[GC:] [FVM_NativeCallGuard](../src/core/native_call_guard.hpp#L9) действует на весь raw topology/receiver/code snapshot.
[fvm_gc_collect](../include/flashvm/gc.h#L44) внутри callback даёт [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), [fvm_gc_step](../include/flashvm/gc.h#L90) — [FVM_OK](../include/flashvm/flashvm.h#L42) без
сборки. GC нужен на следующей безопасной host-границе. clip_input_dispatching блокирует
tree reentry и context teardown; RAII восстанавливает flag при любом результате/OOM.
Flag и активный пакет не экспортируются, managed scene/code сохраняет прежний transport.

Все пять событий дают динамическую eligibility updateAfterEvent на время retained
байткода. Старый committed request переживает ошибки; разрешённый внешний interval/
input scope сохраняется. Никакого показа пикселей сам API не выполняет.

## SDL player

Плеер передаёт pinned root в fvm_desktop_window_pump_clip_input. Для каждого FIFO
transition сначала восстанавливает authored world coordinates и публикует copied
input state, затем исполняет Mouse/Key namespace handlers. С v0.627 маршрут с root
не вызывает focused clip отдельно: его метод доставит сцена без дубля; pump без
root сохраняет прежнюю focus-first доставку. Если этот этап успешен
и reset epoch не изменился, доставляет retained scene records и с v0.627 допустимые
именованные методы через [`fvm_movieclip_dispatch_input`](../include/flashvm/clip_actions.h#L68), затем переходит к следующему
transition ([контракт методов](CLIP_INPUT_METHODS.ru.md)).

Старый fvm_desktop_window_pump_input и keyboard wrapper остаются namespace-only.
Новый API заимствует root на время синхронного main-thread pump. Namespace error/reset
пропускает tree-stage данного события. Tree error сохраняет state/callback effects,
потребляет текущее событие, оставляя последующие в очереди. Recursive pump запрещён,
callback append ждёт следующего entry batch. Focus/background reset отменяет старые
пакеты прежним способом.

Primary touch использует существующую single-contact down/move/up трансляцию;
secondary contacts/generated mouse duplicates фильтруются как раньше. Новых keyboard/
mouse APK и предположения о физической клавиатуре Android нет. KeyPress mapping,
wheel ClipActions, Press/Release/Roll/Drag hit targets и автоматические lifecycle
остаются открытыми. Сам retained-only API по-прежнему не вызывает именованные методы. Полный порядок оригинального Player
не сертифицируется.

## Проверки и пример

[Core](../tests/clip_input.cpp), [order](../tests/clip_input_order.cpp),
[topology](../tests/clip_input_topology.cpp), [mutation](../tests/clip_input_mutation.cpp)
проверяют пять событий, invisible/stopped, порядок depth, malformed/deep graph,
removed/moved ancestors и deferred newborns. [Budget](../tests/clip_input_budget.cpp),
[OOM](../tests/clip_input_allocation.cpp), [workers](../tests/clip_input_worker.cpp),
[state/guards](../tests/clip_input_state.cpp) проверяют общий предел, committed effects,
отказы allocation, реальные OS threads, native state, GC/teardown/reentry/phase guards
и exception. [Graph/JIT](../tests/clip_input_graph_jit.cpp) использует sender teardown,
traced strings, imported tree и настоящие automatic-native entries.

[SDL adapter](../frontend/tests/clip_input.c) использует настоящую SDL dummy video с
синтетическими mouse/finger/key messages: координаты, held keys, namespace-before-record,
append, partial errors и recursive pump. [Настоящий player](../frontend/tests/clip_input_player.py)
с [SDL queue injector](../frontend/tests/clip_player.c) проверяет SWF5–8 interpreter/JIT,
пять событий после первого показа, сумму marker и один coalesced interframe redraw.
Wrong-value controls обязаны завершаться ошибкой. Production adapter используется
без замены; injector есть только в отдельной test executable.

[C89-пример](../examples/embedding/clip_actions/README.ru.md) отдельно публикует pointer,
доставляет MouseMove дереву и снимает redraw без renderer. Linux dummy-video/protocol
не является физическим input, видимым устройством или приёмкой Windows/macOS/Android.
Все девять этапов остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_EVENT_KEY_DOWN` | macro / define | [include/flashvm/clip_actions.h:11](../include/flashvm/clip_actions.h#L11) |
| `FVM_CLIP_EVENT_KEY_UP` | macro / define | [include/flashvm/clip_actions.h:12](../include/flashvm/clip_actions.h#L12) |
| `FVM_CLIP_EVENT_MOUSE_DOWN` | macro / define | [include/flashvm/clip_actions.h:9](../include/flashvm/clip_actions.h#L9) |
| `FVM_CLIP_EVENT_MOUSE_MOVE` | macro / define | [include/flashvm/clip_actions.h:8](../include/flashvm/clip_actions.h#L8) |
| `FVM_CLIP_EVENT_MOUSE_UP` | macro / define | [include/flashvm/clip_actions.h:10](../include/flashvm/clip_actions.h#L10) |
| `FVM_CLIP_INPUT_MAX_CHILDREN` | macro / define | [src/standard/clip_input_constants.h:7](../src/standard/clip_input_constants.h#L7) |
| `FVM_CLIP_INPUT_MAX_DEPTH` | macro / define | [src/standard/clip_input_constants.h:5](../src/standard/clip_input_constants.h#L5) |
| `FVM_CLIP_INPUT_MAX_NODES` | macro / define | [src/standard/clip_input_constants.h:6](../src/standard/clip_input_constants.h#L6) |
| `FVM_CLIP_INPUT_MAX_RECIPIENTS` | macro / define | [src/standard/clip_input_constants.h:8](../src/standard/clip_input_constants.h#L8) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../include/flashvm/keyboard.h#L82) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:29](../include/flashvm/mouse.h#L29) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:37](../include/flashvm/clip_actions.h#L37) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:53](../include/flashvm/clip_actions.h#L53) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:68](../include/flashvm/clip_actions.h#L68) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
