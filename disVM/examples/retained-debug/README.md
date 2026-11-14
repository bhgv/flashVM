# Retained debug example / Пример сохранённой отладки

EN: Build the ordinary runtime/bootstrap compiler, then run from repository root:
RU: Соберите обычные runtime/bootstrap compiler, затем из корня репозитория:

```
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module -o retained.dis disVM/examples/retained-debug/main.b
python3 disVM/examples/retained-debug/demo.py build/disvm-opt/disvm retained.dis
```

EN: The optional host demo uses independent private pipes to inspect an original
managed throw, cyclic/aliased payloads, stack/REG/opcode and root census while
another guest allocates and GC progresses. It pauses a loop, native wait
and channel wait, then explicitly terminates/joins its guests. Python is required
only for this demonstration client, never the VM. No GDB is required.
RU: Необязательный host demo через независимые private pipes исследует исходное
managed exception, циклические данные/алиасы, stack/REG/opcode и GC-корни, пока
другой поток выделяет память и работает GC. Приостанавливает loop,
native wait и channel wait, затем явно завершает/ожидает свои потоки. Python нужен
только клиенту примера, не VM. GDB не требуется.

```
python3 disVM/tools/debugger.py build/disvm-opt/disvm --jit retained.dis
# Or / либо:
build/disvm-opt/disvm --debug /private/new-debug-socket --jit retained.dis
python3 disVM/tools/debugger.py --socket /private/new-debug-socket
# Optional native debugger / Необязательный native debugger:
build/disvm-opt/disvm --gdb --jit retained.dis
```

EN: Type e.g. `list 0`, `status PID`, `pause PID`, `census PID`, `continue PID`.
EOF detaches and preserves frozen guests. See the [contract](../../runtime/DEBUGGER.md).
RU: Введите например `list 0`, `status PID`, `pause PID`, `census PID`, `continue PID`.
EOF отсоединяет клиента, сохраняя frozen guests. См. [контракт](../../runtime/DEBUGGER.ru.md).
