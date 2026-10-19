# Подсказки пробуждения host для таймеров — v0.622

[English](TIMER_WAKEUP.md). [fvm_timers_next_delay](../include/flashvm/timers.h#L29) ограничивает обычное ожидание
event loop временем до ближайшего callback. Запрос использует те же полные
[часы контекста](GET_TIMER.ru.md) и очередь, что [fvm_timers_poll](../include/flashvm/timers.h#L19), не вызывая и не
потребляя таймер. Ограничения регистрации/доставки: [TIMERS.ru.md](TIMERS.ru.md).

## API и владение

Вызвать [fvm_timers_next_delay](../include/flashvm/timers.h#L29) на потоке владельца контекста с пределом
`max_wait_ms` и ненулевыми `unsigned long *wait_ms`, `int *pending`.
Выбирать предел, представимый API ожидания host. Память не выделяется,
callback/coercion пользователя не вызывается, управляемого результата для pin нет.

Пустая очередь даёт [FVM_OK](../include/flashvm/flashvm.h#L42), wait=max_wait_ms, pending=0 без чтения платформенных
часов. Непустая — pending=1 и минимум предела/ближайшей задержки с округлением
вверх до целой миллисекунды. Готовая запись даёт ноль. Положительный дробный
остаток даёт хотя бы единицу, кроме явно нулевого предела host. Допускаются ноль
и ULONG_MAX; пустая очередь возвращает ULONG_MAX без потери точности.

[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45): нулевой контекст/результаты и фазы уничтожения, сборки,
освобождения. Ошибка/некорректный отсчёт часов или исчерпание работы — [FVM_ERROR](../include/flashvm/flashvm.h#L43).
При любой ошибке сохраняются оба результата, pending-записи и их deadlines.
Сам отсчёт может обновить сохранённое полное время до последующей ошибки бюджета.
Откат часов и modulo-2^32 результата GetTime не переполняют deadlines.

Запрос разрешён внутри callback таймера и делит текущий [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9).
Каждая просмотренная запись стоит единицу работы. Лимит ноль не выключает
кооперативную отмену worker. Нахождение уже готовой записи не пропускает учёт
остальных записей. Запрос не создаёт GC-корни, строки, Function или OS timer.

## Использование в event loop

Доставить готовые события, запросить подсказку с пределом следующего
frame/audio/host-события, затем ждать OS event либо указанное время и повторить.
Нулевая подсказка означает повторить доставку без блокирующего ожидания, а не
рекурсивный poll из callback. После изменения очереди запросить заново.
Подсказка — снимок, а не регистрация OS wakeup или гарантия realtime.
[fvm_timers_poll](../include/flashvm/timers.h#L19) по-прежнему отвергает повторный вход.

Общий desktop player теперь обрабатывает ввод и таймеры на активных итерациях
цикла, в том числе между кадрами. Ожидание SDL ограничено ближайшим таймером и
существующими frame/audio-пределами. Активации callback собираются в безопасной
host-точке и между редкими кадрами. Timeline, rendering и счётчик кадров сохраняют
частоту фильма; изменения состояния callback видны при следующем render.
В v0.623 добавлены [допустимые updateAfterEvent redraw](UPDATE_AFTER_EVENT.ru.md).

Существующая приостановка в фоне сохраняется: неактивный player не доставляет
таймеры; после возобновления interval объединяет пропущенные срабатывания.
Ограниченное ожидание показа последнего кадра также не продолжает активный
timeline и доставку таймеров. Точное расписание/lifecycle оригинального Player
и приёмка устройств остаются открытыми.

## Подтверждения

[Часы/бюджет/lifecycle](../tests/timers_next_delay.cpp): округление вверх, пределы,
пустая очередь с недоступными часами, ошибки без изменения результатов, откат,
GetTime wrap, округление deadline при больших double, запрос внутри доставки
и общий бюджет.
[Отказы памяти](../tests/timers_next_delay_allocation.cpp): все new/new[] выключены
при повторных запросах; корни, объекты, строки, deadlines и счётчики не меняются.
[Настоящие workers](../tests/timers_worker_cancellation.cpp): отмена запроса до
poll и из первого callback.

[Регрессия player](../frontend/tests/timers.py) запускает настоящий SDL player с
dummy video, SWF6–8 и 2 fps. Интервал 10 мс должен успеть минимум три раза до
второго кадра в interpreter и native JIT. Испорченный счётчик обязан дать ошибку.
Временный player с циклом v0.621, доставлявшим только по кадрам, также отверг
положительный фильм: тест обнаруживает прежнее поведение. Это проверки протокола
и широкого временного окна, а не latency benchmark, видимое окно, звук или другая ОС.

[Пример C89](../examples/embedding/timers/README.ru.md) запрашивает подсказку до
poll и проверяет пустую очередь после отмены. Ядро VM не зависит от SDL;
любой host может использовать C89 API со своим источником событий.

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
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_timers_next_delay` | function | [include/flashvm/timers.h:29](../include/flashvm/timers.h#L29) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:19](../include/flashvm/timers.h#L19) |
<!-- source-links:end -->
