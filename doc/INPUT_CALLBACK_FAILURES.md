# Input callback exception boundary / Исключения callbacks ввода — v0.324

A C++ host callback could previously throw through [fvm_input_dispatch](../include/flashvm/input.h#L82). During a
queued pump, unwinding bypassed the manual pumping_input reset, so every later
pump returned invalid argument. Direct dispatch now contains callback exceptions:
std::bad_alloc maps to [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44), all other C++ exceptions to [FVM_ERROR](../include/flashvm/flashvm.h#L43).
C callbacks and the synchronous callback signature are unchanged.

The pump observes the returned error, releases its existing reentry flag and
retains remaining FIFO items. The failed event was already removed; processed
counts successful callback deliveries only, excluding that failed attempt.
Events accepted by a callback before it throws remain queued. No callback side
effects are rolled back. Callbacks still must not destroy their context.
This change adds no collector barrier: host callback collection remains allowed.

input_callback_failures tests both direct and queued bad_alloc/other exceptions,
successful recovery, GC inside callbacks, rejected nested pumping, FIFO delivery
of remaining events and enqueue-before-throw behavior. It verifies root/native/
script state and the exact successful-delivery count after each failure.
Existing input validation, queue, dispatch and recognizer checks remain enabled.

## Русский

Исключение C++ из callback раньше выходило через C-интерфейс и оставляло очередь
в состоянии pumping_input: дальнейшие pump отвергались. Теперь dispatch
преобразует bad_alloc в [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44), остальные исключения — в [FVM_ERROR](../include/flashvm/flashvm.h#L43).
Очередь получает код ошибки и восстанавливает флаг; следующий вызов работает.

Неудачное событие уже удалено, processed считает только успешные доставки.
Оставшиеся события и события, добавленные callback до исключения, сохраняются;
откат побочных эффектов не заявляется. Сборка мусора в host callback разрешена,
уничтожать его контекст по-прежнему нельзя. Тест проверяет оба вида исключений,
восстановление, порядок FIFO, вложенный pump, GC и состояние корней.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
