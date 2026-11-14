# Исследование стека потока

Copyright (c) 2026 bhgv

[English](README.md) · [Контракт](../../runtime/OWNER-STACK.ru.md)

Пример создаёт настоящий гостевой поток с нетерминальной рекурсией, копирует стек
через отдельный диагностический mailbox владельца, завершает гостя и печатает
сохранившиеся данные наблюдателя. Остальные native-потоки не приостанавливаются.

Из корня репозитория с собранными совместимыми NC64 compiler/runtime:

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/residentstack.dis disVM/debug/residentstack.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/resident-stack.dis disVM/examples/resident-stack/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/resident-stack.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/resident-stack.dis
```

residentstack.dis должен лежать рядом с образом вызывающей программы. Успешный
запуск печатает пять кадров, `PASS resident stack example`, затем нулевое число
удерживаемых diagnostic credits при shutdown. Точные PC зависят от образа и границы
снятия: наверху — следующая Dis-инструкция, ниже — сохранённые позиции возврата
вызывающих функций. Это не номера строк исходников.

В приложении вызывайте `Residentstack->inspect(id, 1000)`. Проверяйте status,
sequence и StackComplete/StackTruncated/StackUnavailable. Копируются до восьми
кадров от текущего к вызывающим. Строки — ограниченные UTF-8 подписи, не глобальные
module handles. Ready может не содержать доступных кадров; Pending означает,
что владелец ещё не ответил. Переменные, native-адреса, pause и external attach
этот пример не предоставляет.
