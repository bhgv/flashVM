# Generated scalar owner из Limbo .m

Copyright (c) 2026 bhgv

[English](README.md) · [Интерфейс](compute.m) ·
[C-генератор](../../../compiler/limbo/frontend/remote_owner.c) ·
[Сетевой контракт owner](../../remote/NETWORK-OWNERS.md)

Пример генерирует мост на стороне владельца из обычного `.m`, компилирует мост в
`.dis` и вызывает нормальную реализацию в другой настоящей DisVM. `.m` описывает
интерфейс; requests/results идут по существующему соединению Proxynet. Сам `.m`
ничего не передаёт, байткод провайдера не загружается в VM клиента.

Первый профиль точный: один module с одним экспортом `fn(value: int): int`.
Имена параметра/экспорта берутся из существующего C99 Coco/R catalog, без regex
или второго Limbo parser. Generated adapter делает обычный typed `load`
выбранной реализации: исходный Dis linker проверяет фактическую сигнатуру.
Исходник компилируется существующим matching native-cell producer; новый
универсальный Dis backend здесь не заявлен.

## Сборка и генерация

Соберите `compiler/limbo/frontend` с явными Coco/R SDK settings из
[руководства frontend](../../../compiler/limbo/frontend/README.ru.md), затем:

```sh
cmake --build build/remote-owner-compiler --target limbo_remote_owner
build/remote-owner-compiler/limbo_remote_owner \
  disVM/examples/remote-scalar/compute.m compute.m implementation.dis handler.b
```

| Аргумент CLI | Смысл |
| --- | --- |
| INPUT.m | Явный interface source, максимум 2MiB |
| INCLUDE.m | Basename с тем же интерфейсом, доступный Dis compiler |
| IMPLEMENTATION.dis | Provider-local module path от trusted launcher |
| OUTPUT.b | Generated Proxyhandler implementation для обычной Dis compilation |

Include/path ограничены255 ASCII bytes: letters/digits, `_`, `-`, `.` и, только
для implementation path, `/`. Конфликт с imports/locals адаптера даёт отказ.
Includes, ADT, aliases, дополнительные exports, variadics, polymorphism, другая
арность и non-int argument/result пока не поддержаны и отклоняются до открытия
destination. Syntax error также сохраняет прежний output. Write/close error
может оставить prefix; exit2 — ошибка, exit0 — успех. Это host C tool без
зависимости от VM и сети.

## Полная проверка

```sh
python3 disVM/runtime/test_generated_remote.py \
  build/remote-owner-compiler/limbo_remote_owner \
  build/native-dynamic/disvm \
  build/native-dynamic/bootstrap/limbo-bootstrap disVM/upstream/module
```

Driver владеет двумя temporary directories. Только провайдер получает
`implementation.dis` и `provider-secret`; клиент — connection libraries и caller.
Запускаются два реальных VM process через trusted inherited TCP stream:
интерпретатор/JIT, оба register polling modes.

Generated handler загружает оригинальную реализацию на permanent owner,
удерживает module и проверяет owner PID при calls. Реализация читает свой файл
и увеличивает module state между calls, включая call с exception. Клиент
проверяет scalar results, OwnerError, recovery после exception и явный Stop/join.
Для обеих VM действуют существующие проверки root/resident/RC drain.
Запросы не повторяются после uncertain completion.

NC64 `int` использует signed scalar representation существующего private network
owner. Совместимость с classic32 peer здесь не обещается; новые public wire fields,
operation IDs и S-record tags не назначаются.

## Текущая граница

Реализован generated **owner dispatch**, не автоматический remote `load` и не
полный клиентский module proxy. Клиент примера пока явно вызывает Proxynet API.
Strings, ADT, произвольные argument lists, channels и FD ещё не маршалируются;
неподдержанные handler operations явно бросают exception. Выбор реализации и
transport admission остаются ответственностью host.

VFS root не сбрасывается, remote tree не фиксируется, code/device dependencies
не переносятся на терминал. Следующий участок — связать проверенную генерацию
интерфейса с клиентским module binding и расширять marshalling по поддержанным
категориям типов. Сейчас это работающий ограниченный путь, а не заглушка,
объявляющая поддержку любых сигнатур.

[C API: limbo_remote_owner](../../../compiler/limbo/frontend/REMOTE-OWNER.ru.md)
