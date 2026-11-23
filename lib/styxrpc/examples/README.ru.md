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

## Запуск DisVM из C

```sh
python3 examples/disvm/launch.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/disVM/module \
    --driver build/styxrpc-launch
```

C-клиент выбирает существующее обрамление двустороннего транспорта и монтирует
прямой поток Styx. Он сверяет профиль запуска raw, передаёт байты по `.unibus`
и опрашивает `.exit` по тому же соединению. Сценарий проверяет успешный и
неуспешный запуск в обоих режимах VM. Статус 2 означает подтверждённый успех,
статус 3 — подтверждённую ошибку программы.

## Именованные запуски и встречный контекст

```sh
python3 examples/disvm/named.py --runtime /path/to/disvm \
    --limbo /path/to/limbo-bootstrap --module /path/to/upstream/module \
    --native-module /path/to/disVM/module --driver build/styxrpc-named
```

Сценарий запускает постоянного провайдера DisVM и C-клиента на двух концах
сокета. C-клиент публикует `.prog/42` во встречном потоке, вызывает две программы
по публичным именам, закрывает fid запущенной задачи с бесконечным циклом и
проверяет следующий вызов на том же соединении. Первый образ читает `.pid` и
файл из `.vfs`, обновляет `.var/count`, читает FD 0 и пишет в FD 1 и 7.
В обоих режимах ожидается `PASS one socket, two named Dis modules, reverse .var/.fd/.vfs`.

## Сгенерированный интерфейс и вложенный ref

Бинарник `styxrpc-interface` печатает `device.m` из того же описателя,
который использует устройство. Сценарий `examples/disvm/run.py` запускает
генератор из каталога `--device` или по явно переданному `--interface`, затем
компилирует Limbo с `-D RG`. После вызова `bump` проверяется, что свойство
вложенного объекта обновилось, а ссылка по-прежнему указывает на тот же
объект. В обоих режимах ожидается `PASS Dis load of standalone C device`.

## Ветвление каналов через UniBus

```sh
ctest --test-dir build -R styxrpc-unibus-socket --output-on-failure
```

Два C-конца используют `.prog/42/.unibus` по Styx. Тестовый планировщик
выбирает готовую ветку со строкой, пока соседний fid ещё ожидает приём.
После clunk проверяется ровно один вызов обработчика отмены. Для межсредовых
`chan of T` ещё требуется связать выдачу полномочий и планировщик Limbo.
