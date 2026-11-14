# Native signal capture / Захват нативного сигнала

```
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module -o signal.dis disVM/examples/jit-signal/main.b
build/disvm-opt/disvm --jit signal.dis
python3 disVM/runtime/test_jit_signal.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

EN: The ordinary guest prints caught=0 second=0 and correct data/results, then
joins its children. No fault injection exists in the production executable.
The regression links a private probe with separate immutable real trap pages:
SIGILL, SIGSEGV, SIGBUS, partial write then trap and repeated different faults
on one owner. Default mode catches the new VM error without an external tool.
Debug mode inspects named CPU registers, original Dis frames/payload/operation
and GC roots while an independent native owner allocates. It continues the
retained source, verifies correct later results and joins the source/collector
threads. Logs include JITSIGNAL, JITCPU and whole-function JITFAULT.

RU: Обычный guest выводит caught=0 second=0, верные данные/результаты и завершает
дочерние потоки. Production executable не содержит fault injection. Регрессия
связывает private probe с отдельными immutable real trap pages: SIGILL, SIGSEGV,
SIGBUS, запись перед аварией и повторные разные faults одного owner. Default mode
ловит новую VM error без внешнего инструмента. Debug mode исследует именованные
CPU-регистры, исходные Dis frames/payload/operation и GC-корни, пока независимый
native owner выделяет память. После continue проверяются результаты и завершение
source/collectors. Логи содержат JITSIGNAL, JITCPU и JITFAULT всей функции.

EN: Unsafe-helper/guard/cleanup cases intentionally require fatal refusal; they
are not recovery examples. Python is required only for the optional host test
client. C99 and system signal/thread facilities implement runtime behavior.
RU: Unsafe-helper/guard/cleanup cases намеренно требуют fatal refusal: это не
примеры восстановления. Python нужен только optional host test client; runtime
использует C99 и системные signal/thread facilities.

[EN contract](../../runtime/JIT-SIGNALS.md) · [Контракт RU](../../runtime/JIT-SIGNALS.ru.md)
