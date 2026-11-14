# Whole-function JIT quarantine / Карантин всей JIT-функции

```
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -o quarantine.dis disVM/examples/jit-quarantine/main.b
build/disvm-opt/disvm --jit quarantine.dis
python3 disVM/runtime/test_jit_quarantine.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

EN: Ordinary execution prints caught=0 and correct results. The regression links
an isolated probe that throws an explicit backend exception before one native
addition: caught=1, JITFAULT identifies the whole function, later native entries
in that function are absent, and another function keeps native execution.
The production executable has no injected defect. No external debugger is used.
Its optional GDB test validates argument forwarding with a fake executable;
actual GDB operation is not claimed by that test.
RU: Обычный запуск выдаёт caught=0 и верные результаты. Регрессия подключает
отдельный probe: явная backend exception до native addition даёт caught=1,
JITFAULT указывает всю функцию, последующих native entries в ней нет, другая
функция сохраняет native execution. В production нет injected defect.
Внешний debugger не используется. Optional GDB test проверяет передачу аргументов
фиктивным executable, а не настоящую работу GDB.

[EN contract](../../runtime/JIT-QUARANTINE.md) · [Контракт RU](../../runtime/JIT-QUARANTINE.ru.md)
