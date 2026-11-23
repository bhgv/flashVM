# Вызов независимого устройства на C

[English](README.md) · [libstyxrpc](../README.ru.md)

Соберите примеры с `-DSTYXRPC_EXAMPLES=ON` и установленной серверной библиотекой
Styx. Из каталога исходников `styxrpc`, если каталог сборки называется `build`:

```sh
python3 examples/socketpair.py build/styxrpc-device build/styxrpc-client
```

Сценарий запускает два C-процесса на разных концах сокета. Устройство
публикует `sample` в `/mounted/device.dis`, удерживает состояние на fid и
вызывает колбэк инкремента один раз после полного запроса. Клиент открывает
новый fid для каждого из двух вызовов и проверяет ответы. При успехе он
печатает `PASS native C calls standalone C device`. Клиент дочитывает ответ
до clunk. Неподтверждённая передача завершает проверку с ошибкой, сценарий
завершает оба процесса.

Для проверки из DisVM укажите пути к собранной VM, bootstrap-компилятору
Limbo и каталогу интерфейсов Limbo:

```sh
python3 examples/disvm/run.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/upstream/module \
    --device build/styxrpc-device
```

Ожидаемый вывод — `PASS Dis load of standalone C device`. Для JIT добавьте
`--jit`. Клиент также читает байт `value`, пишет новый и снова читает его.
Сценарий передаёт подключённый дескриптор через
`--styx-mount-fd FD --raw --at /device`. Режим `--raw` здесь монтирует
однонаправленный Styx. Сценарий ограничивает время и дожидается завершения
обоих процессов. Клиент Limbo компилируется до запуска устройства.
