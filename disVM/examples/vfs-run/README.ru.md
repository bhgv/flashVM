# Запуск Dis по пути VFS

[English](README.md) · [Клиент](runexample.b) · [Интерфейс](runexample.m) · [Руководство](TUTORIAL.ru.md) · [Обзор](../../runtime/VFS-LAUNCH.ru.md)

Пример показывает, как отправить владельцу команду по смонтированному имени,
передать бинарные данные по маршруту, который их принимает, и прочитать код
завершения обычным `Sys->open/read`. Клиент загружает `Vfsctl` обычным `load`.

```limbo
example := load Runexample Runexample->PATH;
payload := array[] of {byte 'x', byte 0, byte 'y'};
code := example->execute("/routed.dis", "/routed.dis", "/run-result", payload);
```

В этом сценарии `/routed.dis` — имя команды в VFS клиента, `/run-result` —
выданный владельцем файл её результата. Повторение пути команды отправляет байты в тот же запрос. У владельца
`init` читает их через `Launchcontext.arguments()`. Отдельный маршрут
`/payload.dis` доступен для проверки callback; `nil` отправляет пустой запрос.

## Запустить проверку

Из корня репозитория, на Linux с CMake, C-компилятором и Python:

```sh
cmake -S disVM/runtime -B build/launch-runtime -DDVM_RUNTIME_TESTS=OFF
cmake --build build/launch-runtime --target disvm --parallel
cmake -S disVM/native -B build/launch-sdk \
  -DINFERNO_SOURCE="$PWD/disVM/native/src" \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD" \
  -DINFERNO_DISVM_RUNTIME="$PWD/build/launch-runtime/disvm" \
  -DINFERNO_LIMBO_COMPILER="$PWD/build/launch-runtime/bootstrap/limbo-bootstrap"
cmake --build build/launch-sdk --target terminal-ordered-files --parallel
ctest --test-dir build/launch-sdk -R '^terminal-ordered-files-guest$' --output-on-failure
```

Тест сам компилирует и публикует `runexample.dis` и `vfsctl.dis`, связывает
пространства имён по TCP и запускает интерпретатор и JIT. Успех означает,
что `init` прочитал файл у владельца, владелец получил `x\0y`, а клиент
прочитал код выхода 0 через ещё открытое подключение.
