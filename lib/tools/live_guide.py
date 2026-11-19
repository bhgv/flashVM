# Copyright (c) 2026 bhgv
# EN: Authored EN/RU tutorial for mutable names and pinned object lifetimes.
# RU: Авторское EN/RU руководство изменяемых имён и удержанных объектов.
from pathlib import Path
from .introductions import page
from .memory_guide import source_link


# EN: Write the two live tutorial pages beneath borrowed package root.
# Content describes actual example behavior; no runtime source is generated.
# RU: Записать две страницы live tutorial под root заимствованного пакета.
# Текст описывает поведение примера; runtime source не генерируется.
def main(root):
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        heading = 'Живое пространство имён: окна и контролы' if ru else 'Live namespace: windows and controls'
        text = f'# {heading}\n\nCopyright (c) 2026 bhgv\n\n[English](README.md) · [Русский](README.ru.md) · [Examples](../README{suffix})\n\n'
        text += '''## Что и зачем

Этот сервер показывает разницу между именем и уже открытым объектом. Окно имеет каталог и два файла: title и button. Запись в ctl добавляет окно или скрывает его имя. Это модель реестра приложения, без GUI toolkit, DisVM и FlashVM; вместо окон здесь могут быть датчики, задачи или записи БД.

Новые Twalk и Tread корня видят текущие имена. Уже открытый fid удерживает ID объекта до Tclunk или разрыва своего соединения. Удаление имени не перенаправляет такой fid на новое окно. Дерево не замораживается и не передаётся целиком.
''' if ru else '''## What and why

This server demonstrates the difference between a name and an already opened object. A window has a directory and two files: title and button. Writing ctl adds a window or hides its name. It models an application registry without a GUI toolkit, DisVM or FlashVM; sensors, jobs or database records can replace windows.

Fresh Twalk and root Tread operations see current names. An opened fid retains its object ID until Tclunk or connection loss. Removing a name never redirects that fid to a new window. The tree is neither frozen nor transmitted wholesale.
'''
        text += '\n## ' + ('Сборка и запуск' if ru else 'Build and run') + '''

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-live 5640
```

'''
        text += ('Команды выполняются из корня пакета. Сервер остаётся в первом терминале; в другом:\n' if ru else 'Run from the package root. Leave the server running in the first terminal; in another:\n')
        text += '''
```sh
./build/examples/styx-cat 5640 window-100/title
ctest --test-dir build -R '^styx-live' --output-on-failure
```

'''
        text += ('Ожидается `Live window` без завершающего LF. Порт — единственный параметр: decimal 0..65535, default 0 выбирает свободный. Первая stdout-строка сервера содержит фактический порт. Выберите свободный порт и одинаково задайте его клиенту. Ошибка startup даёт ненулевой exit; Ctrl-C завершает пример. Второй backend source параметр не используется. Hosted-профиль требует Linux64/libc/pthread; минимальный codec не запускает этот сервер.\n' if ru else 'Expect `Live window` without a trailing LF. The port is the only meaningful parameter: decimal 0..65535, default 0 selects an available port. The first server stdout line contains the actual port. Choose an available port and pass the same value to the client. Startup failure exits nonzero; Ctrl-C terminates this example. A second backend source argument is unused. The hosted profile requires Linux64/libc/pthread; the minimal codec does not run this server.\n')
        text += '\n## ' + ('Имена, права и ID' if ru else 'Names, permissions and IDs') + '\n\n'
        text += '''| Path | mode | ID / contents |
| --- | --- | --- |
| / | directory 0555 | 0 |
| /ctl | 0222 | 1 |
| /window-N | directory 0555 | N |
| /window-N/title | 0444 | N+1 / `Live window` |
| /window-N/button | 0444 | N+2 / `enabled` |

'''
        text += ('Первое N=100. Успешный add резервирует три следующих ID: 103, 106 и далее. Неуспешный add не расходует ID. Всего 16 мест, включая скрытые окна с открытыми fid. Счётчик ID не оборачивается: исчерпание даёт отказ. Mode проверяет сервер, а ctl дополнительно проверяет синтаксис и смысл команды. Это демонстрационная политика, не authentication/изоляция юзеров.\n' if ru else 'The initial N is 100. Each successful add reserves the next three IDs: 103, 106 and so on. Failed add does not consume IDs. There are 16 slots, including hidden windows retained by opened fids. The ID counter never wraps: exhaustion fails. The server checks mode; ctl also checks command syntax and meaning. This is demonstration policy, not authentication or user isolation.\n')
        text += '\n## ctl\n\n```text\nadd\nremove <decimal-window-ID>\n```\n\n'
        text += ('Одна Twrite — одна полная команда, offset строго 0. Разрешён один необязательный завершающий LF или CRLF. Длина 1..79 байтов, включая окончание; Rwrite подтверждает всю исходную длину только при успешном изменении. Команда не накапливается между writes.\n\nID содержит только decimal digits, положителен и помещается в uint64; ведущие нули разрешены. Знак, лишние пробелы, NUL, внутренние CR/LF, несколько команд, overflow, неизвестный/уже скрытый ID и отсутствие свободного места дают Rerror без изменения реестра. Нельзя послать `add\\nadd`: вторую команду сервер не отбрасывает и первую не исполняет. Rerror не закрывает исправное соединение.\n' if ru else 'One Twrite carries one complete command, strictly at offset 0. One optional trailing LF or CRLF is allowed. The length is 1..79 bytes including the ending; Rwrite acknowledges the entire original length only after successful mutation. Commands are not accumulated across writes.\n\nThe ID contains only decimal digits, is positive and fits uint64; leading zeroes are allowed. A sign, extra spaces, NUL, internal CR/LF, multiple commands, overflow, unknown/already hidden ID or unavailable slot produces Rerror without changing the registry. Sending `add\\nadd` executes neither command. Rerror does not close a healthy connection.\n')
        text += '\n## ' + ('Проверяемый сценарий' if ru else 'Worked scenario') + '\n\n'
        text += ('Из второго терминала, с PYTHONPATH=examples. В wire_client каждый успешный open возвращает fid; его владелец закрывает его через clunk, а finally закрывает соединение даже при ошибке.\n' if ru else 'In the second terminal, with PYTHONPATH=examples. Each successful wire_client open returns an owned fid; clunk releases it, and finally closes the connection even on failure.\n')
        text += '''
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

'''
        text += ('Список корня может выводиться в другом порядке; сравнивайте набор имён. Запускайте сценарий на свежем сервере: ID монотонны и не сбрасываются между клиентами.\n' if ru else 'Root listing order may differ; compare the set of names. Run against a fresh server: IDs are monotonic and do not reset between clients.\n')
        text += '\n## ' + ('Backend и время жизни' if ru else 'Backend and lifetime') + '\n\n'
        text += ('`catalog_query(listing,id,start,out,capacity,count)` выдаёт stat или страницу непосредственных детей; start — число строк, не byte offset 9P. Сервер кодирует целые Dir и преобразует byte offset в проход по страницам. Никакой recursive preload нет. Изменение списка между чтениями может потребовать rewind offset=0.\n\n`catalog_pin(id)` удерживает общее место окна для directory/title/button; `catalog_unpin(id)` освобождает одно успешное удержание. Remove скрывает имя сразу, а место освобождает только без удержаний. `catalog_init(source)` вызывается один раз до запросов, source не сохраняется. Это private backend API примера, не установленный ABI библиотеки. Все query/write/pin/unpin сериализует один server owner; при подключении внешнего изменяющего потока потребуется синхронизация. Стандартные клиентские потоки при этом не приостанавливаются.\n' if ru else '`catalog_query(listing,id,start,out,capacity,count)` returns stat or a page of immediate children; start counts rows, not 9P byte offsets. The server encodes whole Dir records and converts the byte offset into page traversal. There is no recursive preload. A listing changed between reads may require rewinding offset=0.\n\n`catalog_pin(id)` retains a shared window slot for directory/title/button; `catalog_unpin(id)` releases one successful retention. Remove hides the name immediately and frees the slot only without references. `catalog_init(source)` runs once before requests and never retains source. This is the private example backend API, not an installed library ABI. One server owner serializes query/write/pin/unpin; an external mutating thread needs synchronization. Client execution threads are not paused.\n')
        text += '\n## ' + ('Проверки и исходники' if ru else 'Checks and sources') + '\n\n'
        text += ('`styx-live-registry` проверяет grammar, bounded pages, 16 мест, два pin одного окна, final close и новый ID после reuse. `styx-live-wire` использует две настоящие 9P-сессии: Rerror не меняет реестр и не разрывает связь; изменение одним клиентом не разрушает opened fid другого. Исходный `styx-live` проверяет базовую навигацию и обновления.\n' if ru else '`styx-live-registry` checks grammar, bounded pages, 16 slots, two pins of one window, final close and fresh identity after reuse. `styx-live-wire` uses two actual 9P sessions: Rerror preserves both registry and connection; one client cannot destroy another client’s opened fid by hiding its name. The original `styx-live` test checks basic navigation and updates.\n')
        text += f'\n[Registry source](../live.c) · [Backend declarations](../catalog.h) · [Server callbacks](../server.c) · [Registry tests](../../tests/live_registry.c) · [Wire tests](../../tests/live_wire.py) · [Client helper](../wire_client.py)\n\n[styxinit](../../docs/reference/styxinit{suffix}) · [styxprocess](../../docs/reference/styxprocess{suffix}) · [styxend](../../docs/reference/styxend{suffix}) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)\n'
        for name in ('catalog_init', 'catalog_query', 'catalog_pin', 'catalog_unpin'):
            link = source_link(root / 'examples/live', '../live.c', name)
            text = text.replace('`' + name + '(', link + ' `(')
        page(root, 'examples/live/README' + suffix, text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'styx')
