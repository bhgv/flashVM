# Живое пространство имён: окна и контролы

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.ru.md)

## Что и зачем

Этот сервер показывает разницу между именем и уже открытым объектом. Окно имеет каталог и два файла: title и button. Запись в ctl добавляет окно или скрывает его имя. Это модель реестра приложения, без GUI toolkit, DisVM и FlashVM; вместо окон здесь могут быть датчики, задачи или записи БД.

Новые Twalk и Tread корня видят текущие имена. Уже открытый fid удерживает ID объекта до Tclunk или разрыва своего соединения. Удаление имени не перенаправляет такой fid на новое окно. Дерево не замораживается и не передаётся целиком.

## Сборка и запуск

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-live 5640
```

Команды выполняются из корня пакета. Сервер остаётся в первом терминале; в другом:

```sh
./build/examples/styx-cat 5640 window-100/title
ctest --test-dir build -R '^styx-live' --output-on-failure
```

Ожидается `Live window` без завершающего LF. Порт — единственный параметр: decimal 0..65535, default 0 выбирает свободный. Первая stdout-строка сервера содержит фактический порт. Выберите свободный порт и одинаково задайте его клиенту. Ошибка startup даёт ненулевой exit; Ctrl-C завершает пример. Второй backend source параметр не используется. Hosted-профиль требует Linux64/libc/pthread; минимальный codec не запускает этот сервер.

## Имена, права и ID

| Path | mode | ID / contents |
| --- | --- | --- |
| / | directory 0555 | 0 |
| /ctl | 0222 | 1 |
| /window-N | directory 0555 | N |
| /window-N/title | 0444 | N+1 / `Live window` |
| /window-N/button | 0444 | N+2 / `enabled` |

Первое N=100. Успешный add резервирует три следующих ID: 103, 106 и далее. Неуспешный add не расходует ID. Всего 16 мест, включая скрытые окна с открытыми fid. Счётчик ID не оборачивается: исчерпание даёт отказ. Mode проверяет сервер, а ctl дополнительно проверяет синтаксис и смысл команды. Это демонстрационная политика, не authentication/изоляция юзеров.

## ctl

```text
add
remove <decimal-window-ID>
```

Одна Twrite — одна полная команда, offset строго 0. Разрешён один необязательный завершающий LF или CRLF. Длина 1..79 байтов, включая окончание; Rwrite подтверждает всю исходную длину только при успешном изменении. Команда не накапливается между writes.

ID содержит только decimal digits, положителен и помещается в uint64; ведущие нули разрешены. Знак, лишние пробелы, NUL, внутренние CR/LF, несколько команд, overflow, неизвестный/уже скрытый ID и отсутствие свободного места дают Rerror без изменения реестра. Нельзя послать `add\nadd`: вторую команду сервер не отбрасывает и первую не исполняет. Rerror не закрывает исправное соединение.

## Проверяемый сценарий

Из второго терминала, с PYTHONPATH=examples. В wire_client каждый успешный open возвращает fid; его владелец закрывает его через clunk, а finally закрывает соединение даже при ошибке.

```python
from wire_client import Client  # Standard 9P test client / Стандартный 9P-клиент примера.
c = Client(5640)  # Negotiate session and attach root / Создать сессию и attach корня.
try:
    held = c.open('/window-100/title')  # Pin original object / Удержать исходный объект.
    c.write('/ctl', 'add')  # Publish window-103 / Опубликовать window-103.
    c.write('/ctl', 'remove 100')  # Hide only the old name / Скрыть старое имя.
    print(c.directory('/'))  # ctl, window-103 / Текущие имена корня.
    print(c.read(held, 0, 64))  # b'Live window'; fid still works / fid ещё работает.
    c.clunk(held)  # Final close permits slot reuse / Разрешить reuse места.
    c.write('/ctl', 'add')  # Publish window-106, never window-100 / Новый ID.
finally:
    c.close()  # Retire remaining fids on failure too / Освободить fid и при ошибке.
```

Список корня может выводиться в другом порядке; сравнивайте набор имён. Запускайте сценарий на свежем сервере: ID монотонны и не сбрасываются между клиентами.

## Backend и время жизни

[catalog_query](../live.c#L73) `(listing,id,start,out,capacity,count)` выдаёт stat или страницу непосредственных детей; start — число строк, не byte offset 9P. Сервер кодирует целые Dir и преобразует byte offset в проход по страницам. Никакой recursive preload нет. Изменение списка между чтениями может потребовать rewind offset=0.

[catalog_pin](../live.c#L236) `(id)` удерживает общее место окна для directory/title/button; [catalog_unpin](../live.c#L262) `(id)` освобождает одно успешное удержание. Remove скрывает имя сразу, а место освобождает только без удержаний. [catalog_init](../live.c#L36) `(source)` вызывается один раз до запросов, source не сохраняется. Это private backend API примера, не установленный ABI библиотеки. Все query/write/pin/unpin сериализует один server owner; при подключении внешнего изменяющего потока потребуется синхронизация. Стандартные клиентские потоки при этом не приостанавливаются.

## Проверки и исходники

`styx-live-registry` проверяет grammar, bounded pages, 16 мест, два pin одного окна, final close и новый ID после reuse. `styx-live-wire` использует две настоящие 9P-сессии: Rerror не меняет реестр и не разрывает связь; изменение одним клиентом не разрушает opened fid другого. Исходный `styx-live` проверяет базовую навигацию и обновления.

[Registry source](../live.c) · [Backend declarations](../catalog.h) · [Server callbacks](../server.c) · [Registry tests](../../tests/live_registry.c) · [Wire tests](../../tests/live_wire.py) · [Client helper](../wire_client.py)

[styxinit](../../docs/reference/styxinit.ru.md) · [styxprocess](../../docs/reference/styxprocess.ru.md) · [styxend](../../docs/reference/styxend.ru.md) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
