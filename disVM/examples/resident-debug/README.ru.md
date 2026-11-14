# Исследовать работающий resident

Copyright (c) 2026 bhgv

[English](README.md) · [Контракт runtime](../../runtime/OWNER-LOCATION.ru.md)

Пример запускает гостя на его permanent OS thread, получает exact resident ID,
запрашивает copied location у owner и присоединяет гостя. Выводятся модуль/путь,
next Dis PC, число инструкций и validity/truncation flags. Пример работает внутри
исследуемой VM и не подключается к другому OS process.

Из корня репозитория, с уже собранными matched NC64 runtime/compiler:

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/residentdebug.dis disVM/debug/residentdebug.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/resident-debug.dis disVM/examples/resident-debug/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/resident-debug.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/resident-debug.dis
```

Reader должен лежать рядом с caller image: sibling-module resolver найдёт там
residentdebug.dis. Оба модуля компилируются с тем же Processctl interface. Успех —
`PASS resident debug example` и отсутствие retained native diagnostic credits.
Конкретный next PC может зависеть от instruction quantum.

В своём приложении используйте `Residentdebug->inspect(id, 1000)`. До чтения PC
проверяйте status и LocationValid. Имя/путь — подписи, не глобальные handles Module.
Truncated strings остаются целыми UTF-8 prefixes. Пример не обходит stack frames,
не читает переменные и не приостанавливает другого гостя.
