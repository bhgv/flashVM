# Глобальные методы ввода MovieClip — v0.627

[English](CLIP_INPUT_METHODS.md). Новый [`fvm_movieclip_dispatch_input`](../include/flashvm/clip_actions.h#L68) дополняет
[рассылку по дереву](CLIP_INPUT.ru.md) методами `onMouseMove`, `onMouseDown`,
`onMouseUp`, `onKeyDown`, `onKeyUp`. SDL-проигрыватель теперь использует этот маршрут.
Существующий [`fvm_movieclip_dispatch_clip_input`](../include/flashvm/clip_actions.h#L53) по-прежнему вызывает только
сохранённые записи и сохраняет прежний учёт работы. API выбранного клипа
[`fvm_movieclip_dispatch_clip_actions`](../include/flashvm/clip_actions.h#L37) также не меняется.

## Контракт хоста и версии

На потоке владельца передайте root MovieClip того же контекста. Между вызовами
удерживайте граф, например pin корня. Допустим ровно один бит MouseMove/Down/Up
или KeyDown/Up из `clip_actions.h`. Хост отдельно публикует состояние ввода.
API не открывает устройство, не вызывает namespace-слушателей Mouse/Key, не
выбирает фокус, не двигает timeline, не опрашивает таймеры и не рисует.

| Версия источника конкретного получателя | ClipActions | Именованные методы |
| --- | --- | --- |
| 1–4 | Только обход | Нет |
| 5 | Да | Нет, getter/resolver даже не ищется |
| 6–8 | Да | Да |
| 0, неограниченная native/ambient policy | Да | Да |

Созданный хостом клип может наследовать ambient policy контекста; native creation
само по себе не сбрасывает её в ноль. Версия читается в начале очереди конкретного
получателя. Невидимость, остановка playback и отсутствие keyboard focus не
исключают клип из этих глобальных событий. Static artwork обработчиков не получает.
Press/Release/Roll/Drag с hit target, перевод KeyPress, колесо и автоматические
Load/Unload/Construct пока не добавлены. Это ограниченный контракт хоста, а не
подтверждение точного порядка событий оригинального Flash Player.

## Подготовка, поиск и изменения

До первого callback готовится прежний ограниченный снимок topology: сначала root,
затем дети по глубине. Новые клипы ждут следующего вызова; удалённые или перенесённые
ветви пропускаются. Для каждого получателя:

1. Проверить captured ancestry и removed, получить версию источника.
2. Подготовить и исполнить подходящие записи из текущей таблицы ClipActions.
3. Для допустимой версии повторить проверку ancestry/removal. Удаливший себя в
   записи клип уже не получает метод в этой доставке.
4. Списать единицу method bridge и один раз найти метод без аргументов с `this`
   равным текущему клипу.

Function-valued собственные, унаследованные и возвращённые getter методы сохраняют
определяющего владельца через обычный dispatcher. Поиск сохраняет обычную case/
resolver policy контекста: одна загрузка MovieClip resource не устанавливает
контекстный SWF execution profile. Полные mixed-profile semantics оригинального
Player этим контрактом не подтверждаются. Зарегистрированные native methods
используют прежний контракт native entry. Отсутствующий или noncallable member
пропускается; ошибки настоящего getter/resolver/callback распространяются. Fallback
к одноимённой глобальной функции отсутствует.

Запись может установить метод, который выполнится сразу после неё. Ранний получатель
может заменить метод позднего: полного снимка функций при входе нет. Метод может
удалить/перенести ветвь, подавляя поздние очереди. Сам getter тоже может менять
сцену: поиск и вызов возвращённой функции являются одной обычной операцией dispatch,
без дополнительной проверки topology между ними. Native callback синхронен и
не прерывается посередине запросом остановки worker.

## Работа, GC, ошибки и redraw

Capture/sort, записи, повторные проверки ancestry, method bridge и AVM1 делят один
[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9). Bridge списывается и при отсутствующем member, поэтому
стоимость общего маршрута отличается от retained-only API. Пределы
`clip_input_constants.h` сохраняются. Остановка кооперативна; единица работы не
является пределом времени или учётом каждой внутренней операции поиска/выделения.

[GC:] Внешний native barrier удерживает граф и временные функции. Collect возвращает
INVALID_ARGUMENT; incremental step не собирает. Повторный вход в любой tree API
того же контекста и teardown блокируются до возврата. Флаг и активный batch не
сериализуются; функции и сцена передаются обычным graph API.

Поиск и выполнение метода разрешают `updateAfterEvent`, включая getter и вложенные
вызовы. Предыдущее динамическое разрешение восстанавливается при любом выходе;
уже установленный pending redraw сохраняется при ошибке. Проигрыватель показывает
результат после существующего callback batch, без скрытого tick внутри метода.

Неверные root/owner/type/event/phase/reentry дают INVALID_ARGUMENT; исчерпание
бюджета или stop — ERROR; отказ allocation — OUT_OF_MEMORY. Настоящие результаты
обработчиков, включая UNSUPPORTED/EXCEPTION, не маскируются. Выполненные изменения,
состояние ввода и ранние callbacks не откатываются; остаток batch пропускается.
Следующий вызов — новое событие, а не продолжение старого.

## SDL и проверки

Каждый переход FIFO публикует event-time состояние, вызывает namespace callbacks,
затем — при успехе и неизменном reset epoch — общий маршрут сцены. При наличии scene
root клавиатура использует новый [`fvm_input_dispatch_key_listeners`](../include/flashvm/keyboard.h#L89): только rooted
Key observers, без отдельного неявного вызова focused clip. Затем сцена доставляет
метод каждому клипу один раз независимо от фокуса. Это предотвращает дубль для
focused clip в том же дереве. Явный Key.addListener остаётся независимым источником
уведомления даже для клипа сцены. Новый API сохраняет подготовку/публикацию/error/
reentry/GC контракт прежней доставки, не читает/меняет фокус и не даёт observer-only
redraw eligibility. При ручной композиции хост задаёт тот же порядок сам. Старый
[`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L82) и SDL pump без root сохраняют focus-first поведение.
Primary touch сохраняет одноуказательный mapping. Recursive pump запрещён; append
из callback ждёт следующего входного batch. Ошибка метода потребляет текущий переход,
оставляя следующие в очереди. Namespace error/reset подавляет сценическую часть.

- [Методы](../tests/clip_input_methods.cpp): пять событий SWF5–8, own/inherited/getter/
  native routes, изоляция старого API, missing/noncallable.
- [Policy](../tests/clip_input_methods_policy.cpp): native version zero, hidden/stopped,
  resolver, teardown и invalid phase/owner.
- [Мутации](../tests/clip_input_methods_mutation.cpp): установка метода из записи,
  self-removal и deferred newborns.
- [Ошибки](../tests/clip_input_methods_errors.cpp), [OOM](../tests/clip_input_methods_allocation.cpp),
  [бюджет](../tests/clip_input_methods_budget.cpp), [workers](../tests/clip_input_methods_worker.cpp):
  committed errors, getter/native throws, allocation sweep, частичное завершение и
  остановка настоящих OS workers.
- [Graph/JIT](../tests/clip_input_methods_graph.cpp): AVM1-метод в prototype после
  уничтожения sender, traced-string GC и настоящие native entries.
- [Композиция клавиатуры](../tests/keyboard_scene_listeners.cpp) и
  [allocation](../tests/keyboard_scene_allocation.cpp): observers-only, один вызов
  focused scene clip, прежний legacy route, state/error/reentry и OOM cleanup.
- [SDL](../frontend/tests/clip_input.c): namespace → запись → метод, event-time pointer/
  held keys, primary touch, focused actor без дублей, append и ошибки записи/метода.
- [Настоящий player](../frontend/tests/clip_input_methods.py) с прежним
  [injector](../frontend/tests/clip_player.c): независимые SWF5–8, method-only и
  десятичный trace порядка record/method, ошибочные контроли, stats JIT и
  объединённый redraw, запрошенный именно методами.

[C89-пример](../examples/embedding/clip_actions/README.ru.md) показывает три API без
SDL. Linux SDL dummy означает синтетические сообщения, не приёмку физического
устройства, видимого окна или Windows/macOS/Android. Все девять стадий открыты.
Проверка v0.627: успешно 275 выбранных Release CTest, 93 ASan/UBSan CTest и strict
C89 sanitizer run. Отдельно слинкованный player со старым v0.626 retained-only SDL
adapter ожидаемо отверг тот же SWF8 record/method trace movie. Leak detection был
выключен; границы доказательств приведены в [status](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_input_dispatch_key_listeners` | function | [include/flashvm/keyboard.h:89](../include/flashvm/keyboard.h#L89) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../include/flashvm/keyboard.h#L82) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:37](../include/flashvm/clip_actions.h#L37) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:53](../include/flashvm/clip_actions.h#L53) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:68](../include/flashvm/clip_actions.h#L68) |
<!-- source-links:end -->
