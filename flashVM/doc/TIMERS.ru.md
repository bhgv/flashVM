# Явно опрашиваемые таймеры AS1/AS2 — v0.621

[English](TIMERS.md). [fvm_standard_install](../include/flashvm/standard.h#L22) добавляет `setInterval`, `clearInterval`
для host/SWF6–8 и `setTimeout`, `clearTimeout` для host/SWF8. Версия проверяется
при каждом вызове, в том числе после установки до загрузки SWF. Используются
существующие [часы контекста](GET_TIMER.ru.md). [fvm_timers_poll](../include/flashvm/timers.h#L22) выполняет явную
доставку на потоке владельца; общий desktop player опрашивает очередь на активных итерациях цикла, включая промежутки между кадрами,
и после шага timeline. [Подсказки пробуждения](TIMER_WAKEUP.ru.md), v0.622.

## Регистрация, отмена и владение

Поддержаны `setInterval(fn, delay, args...)`,
`setInterval(object, "methodName", delay, args...)` и
`setTimeout(fn, delay, args...)`. Цель — Function своего контекста либо для
интервала невызываемый Object своего контекста с непустым String-именем метода.
Метод разрешается при доставке, поэтому замена свойства влияет на следующий
callback, а receiver остаётся переданным объектом. Имена учитывают полную длину
и активные правила регистра SWF. Вызываемый объект выбирает форму Function.
Object/method для timeout и преобразуемые нестроковые имена — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48).
Недостающие обязательные аргументы и некорректная/чужая цель — [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47).

Задержка проходит числовое преобразование AVM1, включая script и GC. Допустимы
конечные значения от [FVM_TIMERS_MIN_DELAY](../src/standard/timer_constants.h#L6) до [FVM_TIMERS_MAX_DELAY](../src/standard/timer_constants.h#L7); дробная часть
отбрасывается после проверки диапазона. Ноль, отрицательные, NaN, бесконечности,
субмиллисекундные и слишком большие задержки возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48).
Это ограниченный профиль, а не утверждение о правилах оригинального Player.
Не более [FVM_TIMERS_MAX_ARGUMENTS](../src/standard/timer_constants.h#L5) дополнительных аргументов и
[FVM_TIMERS_MAX_PENDING](../src/standard/timer_constants.h#L4) живых записей. Переполнение очереди/ID — [FVM_ERROR](../include/flashvm/flashvm.h#L45),
слишком много аргументов — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48).

Аргументы копируются поверхностно: объекты остаются общими живыми объектами,
заимствованные строки, включая NUL, копируются в память контекста.
[GC:] Function/capture, owner/name метода, объекты аргументов, метатаблицы
примитивов и traced-строки удерживаются до отмены/потребления. Нативный userdata
остаётся памятью host: он должен жить до завершения pending и активных вызовов.
Перед его освобождением отменить таймер либо уничтожить контекст.

Успех возвращает точный положительный Number ID до [FVM_TIMERS_MAX_ID](../src/standard/timer_constants.h#L8).
ID не переиспользуются и не переполняются в пределах контекста; все четыре
функции работают с общей локальной очередью. Числовой ID имеет смысл только
в своём контексте. `clearInterval(id)` и `clearTimeout(id)` преобразуют первый
аргумент в Number и могут отменить любой вид таймера по собственному version gate.
Отсутствующий, дробный, неконечный, недопустимый или неизвестный ID ничего не
меняет; результат undefined. Ошибки регистрации/отмены дают undefined и сохраняют
очередь/счётчик, кроме явных побочных эффектов пользовательского преобразования.
Отсчёт часов может обновиться до последующего отказа памяти или бюджета.

## Контракт fvm_timers_poll

Аргументы — живой контекст на потоке владельца и ненулевой `unsigned int *delivered`.
Повторный вход, уничтожение, сборка и освобождение возвращают
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47), сохраняя счётчик. Корректный вызов устанавливает ноль;
пустая очередь не читает часы. Иначе один отсчёт часов определяет снимок due ID
в порядке регистрации. Ошибка часов — [FVM_ERROR](../include/flashvm/flashvm.h#L45). Готова запись, deadline которой
не больше отсчёта; deadline использует полное время без GetTime-переполнения.

Каждый ID вызывается не более одного раза. Отменённые записи пропускаются, новые
ждут следующего poll, даже если callback продвинул часы. До вызова timeout
удаляется, interval получает deadline=отсчёт+delay. Пропущенные интервалы
объединяются в один вызов без догоняющего пакета. Невозможность увеличить deadline
из-за точности double — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48).

Счётчик включает уже зафиксированную доставку с последующей ошибкой, в том числе
OOM подготовки активации Function. Ошибка/исключение останавливает пакет;
удалённые timeout, переназначенные interval, законченные callback и явные эффекты
пользователя сохраняются. До фиксации ошибка снимка/памяти/бюджета не меняет
соответствующую запись. Interval после ошибки остаётся pending, если пользователь
его не отменил. После [FVM_EXCEPTION](../include/flashvm/flashvm.h#L50) очистить исключение [fvm_exception_clear](../include/flashvm/exception.h#L74) перед
продолжением.

[GC:] Корень активных значений устанавливается до удаления/переназначения.
Callback может отменить себя/других, создать таймер и выполнить GC. Рекурсивный
[fvm_timers_poll](../include/flashvm/timers.h#L22) отвергается; [fvm_context_destroy](../include/flashvm/flashvm.h#L86) во время доставки игнорируется,
как при существующей доставке ввода. Ожидания, OS timer/thread, прерывания
произвольного нативного callback и реализации [fvm_advance](../include/flashvm/flashvm.h#L94) здесь нет. Частота poll
определяется host. Изменение desktop source не является приёмкой другой ОС,
видимого воспроизведения или устройства.

## Работа, перенос и проверки

[FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L11) объединяет бюджет инструкций/отмену worker с регистрацией,
преобразованиями, обходом и callback. Регистрация учитывает аргументы, байты
удерживаемых строк и публикацию; отмена — поиск и освобождение значений/записи;
poll — обход, доставку и копирование/удаление. Исчерпание до фиксации сохраняет
очередь. Время произвольного C callback не ограничивается принудительно.
Числовой лимит ноль не выключает кооперативную отмену worker.

Извлечённые Functions таймеров сериализуются с ID 164–167 после getTimer 163;
восстановленные globals допускают собирающие преобразования с корнями.
Pending-очередь, IDs, deadlines, native userdata и live native bodies не
экспортируются вместе с Function. Импортированная регистрация использует очередь
получателя. Уничтожение контекста снимает pending-корни до финализации без вызовов.

Исходники проверок: [базовые/версии](../tests/timers.cpp),
[GC/reentry/errors](../tests/timers_gc_reentry.cpp),
[бюджет/лимиты](../tests/timers_budget.cpp), [OOM](../tests/timers_allocation.cpp),
[графы/coercion](../tests/timers_graph.cpp), [AVM1/JIT](../tests/jit_timers.cpp),
[version/conflict guards](../tests/timers_versions_conflicts.cpp),
[реальные workers](../tests/timers_worker_cancellation.cpp).
[Пример C89](../examples/embedding/timers/README.ru.md).

[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html)
указывает версии, Function/object-method формы интервала, аргументы callback и
одноразовое назначение timeout. Точное расписание оригинального Player,
frame-rate rounding, обработка недопустимых задержек, крайние правила receiver,
выгрузка фильма и платформенная/длительная приёмка остаются открытыми.
Целая стадия роудмапа этим изменением не закрывается.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:94](../include/flashvm/flashvm.h#L94) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../include/flashvm/flashvm.h#L86) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:50](../include/flashvm/flashvm.h#L50) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:74](../include/flashvm/exception.h#L74) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:11](../src/core/execution_work_scope.hpp#L11) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../include/flashvm/standard.h#L22) |
| `FVM_TIMERS_MAX_ARGUMENTS` | macro / define | [src/standard/timer_constants.h:5](../src/standard/timer_constants.h#L5) |
| `FVM_TIMERS_MAX_DELAY` | macro / define | [src/standard/timer_constants.h:7](../src/standard/timer_constants.h#L7) |
| `FVM_TIMERS_MAX_ID` | macro / define | [src/standard/timer_constants.h:8](../src/standard/timer_constants.h#L8) |
| `FVM_TIMERS_MAX_PENDING` | macro / define | [src/standard/timer_constants.h:4](../src/standard/timer_constants.h#L4) |
| `FVM_TIMERS_MIN_DELAY` | macro / define | [src/standard/timer_constants.h:6](../src/standard/timer_constants.h#L6) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:22](../include/flashvm/timers.h#L22) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
