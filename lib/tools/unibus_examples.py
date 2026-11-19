# Copyright (c) 2026 bhgv
# EN: Maintain the two small libunibus tutorial READMEs and their shared example index.
# RU: Хранить README двух малых примеров libunibus и общий индекс примеров.
from pathlib import Path
from .introductions import page
from .gateway_guide import main as gateway_guide


# EN: Write authored EN/RU guides below borrowed base, the independent libunibus root.
# Source/API links are package-local. The larger gateway chapter is owned by its own module.
# RU: Записать EN/RU guides под base — корнем независимого libunibus. Source/API links
# остаются внутри пакета. Большая глава шлюза находится в отдельном модуле.
def main(base):
    descriptions = {
        'nested': (
            'Encode tag-zero schema assignment, then feed a nested client message one byte at a time. Outer tag 7 and inner tag 1 have different scopes. This smallest example needs no allocation, threads or physical connection.',
            'Назначить схему через тег 0, затем подать вложенную запись побайтно. Внешний тег 7 и внутренний тег 1 имеют разные области видимости. Минимальный пример не требует allocation, потоков и физического соединения.'),
        'queues': (
            'A pthread consumer waits only for client 7 while the dispatcher delivers client 8. The barrier establishes concurrent lifetime without sleeps, not exact wait admission. Main then delivers client 7 and retires the worker and resources.',
            'Pthread-потребитель ждёт только клиента 7, пока диспетчер выдаёт клиенту 8 запись. Barrier задаёт совместное время жизни без sleep, но не точный момент wait admission. Затем main выдаёт клиенту 7 и освобождает worker/ресурсы.'),
    }
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        links = []
        for kind in ('nested', 'queues'):
            executable = 'unibus-' + kind
            profile = 'OFF' if kind == 'nested' else 'ON'
            body = f'# {executable}\n\nCopyright (c) 2026 bhgv\n\n'
            body += f'[English]({kind}.md) · [Русский]({kind}.ru.md) · [Index](README{suffix})\n\n'
            body += '## ' + ('Что и зачем' if ru else 'Purpose and design') + '\n\n' + descriptions[kind][ru] + '\n\n'
            body += '## ' + ('Сборка и запуск' if ru else 'Build and run') + f'''

```sh
cmake -S . -B build -DUNIBUS_QUEUES={profile} -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/{executable}
ctest --test-dir build -R '^{executable}$' --output-on-failure
```

'''
            body += '## ' + ('Параметры и результаты' if ru else 'Parameters and results') + '\n\n'
            body += ('Аргументов командной строки нет. Код 0 означает точный ожидаемый результат; код 1 — отказ с названием этапа в stderr. stdout содержит диагностический PASS, не двоичные байты носителя.\n\n' if ru else 'No command-line arguments. Exit 0 means the exact expected result; exit 1 reports failure and stderr identifies the stage. Stdout is a PASS diagnostic, not binary carrier bytes.\n\n')
            if kind == 'nested':
                body += ('Буферы 128/256 байт — учебный выбор, не предел протокола. slots содержит два места, используется одно. Проверяются назначение, версия, schema и четыре байта 21.5. MORE между фрагментами допустим, только последний байт завершает запись. После выдачи borrowed payload нельзя перезаписывать storage до обработки и reset.\n\n' if ru else '128/256-byte buffers are sample capacities, not protocol limits. Two table slots are supplied; one is used. Checks cover assignment, version, schema and four sensor bytes 21.5. MORE between fragments is expected; only the final byte completes this record. Keep storage alive until the borrowed payload is consumed and reset.\n\n')
            else:
                body += ('styxordercreate задаёт максимум сообщения 256 и окно 16. Wait клиента 7 ограничен 5000 мс; это параметры примера, не политика всех клиентов. Старое C-имя styxorder сохраняется для совместимости исходников и не создаёт зависимости от libstyx.\n\nПри setup failure уже созданные engines/barrier освобождаются. После создания worker receiver закрывается до join; готовые записи остаются доступны для drain. unibus-example-failures внедряет четыре ошибки частичной инициализации. Неожиданная ошибка join завершает учебный процесс через _Exit: без подтверждённого завершения worker нельзя освобождать его заимствованное состояние. Встроенное приложение выбирает собственную политику такого фатального отказа.\n\n' if ru else 'styxordercreate selects maximum message 256 and window 16. Client 7 wait is bounded to 5000 ms; these are example settings, not policy for every client. Historical styxorder C names preserve source compatibility without creating a libstyx dependency.\n\nSetup failures release all constructed engines/barriers. After worker creation, receiver close precedes join; ready records remain drainable. unibus-example-failures injects four partial-initialization failures. Unexpected join failure terminates the teaching process through _Exit: borrowed worker state cannot be reclaimed before retirement is proven. An embedding application chooses its own fatal-failure policy.\n\n')
            body += '## ' + ('Исходник и справка' if ru else 'Source and reference') + '\n\n'
            body += f'- [Implementation]({kind}.c)\n- [API](../docs/index{suffix})\n- [Protocol](../docs/protocol{suffix})\n- [Tutorial](../docs/tutorial{suffix})\n'
            body += '- [Main project](https://github.com/bhgv/flashVM/blob/master/disVM/native/terminal/bridge/ORDERED.md)\n'
            body = body.replace('styxordercreate', f'[styxordercreate](../docs/reference/styxordercreate{suffix})')
            page(base, 'examples/' + kind + suffix, body)
            links.append(f'- [{executable}]({kind}{suffix})')
        links.append(f'- [unibus-9p-bridge](bridge/README{suffix})')
        index = '# libunibus: ' + ('примеры' if ru else 'examples') + '\n\nCopyright (c) 2026 bhgv\n\n'
        index += '\n'.join(links) + f'\n\n[Introduction](../docs/introduction{suffix}) · [Tutorial](../docs/tutorial{suffix}) · [API](../docs/index{suffix})\n\n'
        index += ('Примеры файловых деревьев находятся в отдельной libstyx; gateway соединяет wire-форматы без библиотечной зависимости. Межбиблиотечная проверка запускает все пять моделей через gateway.\n' if ru else 'File-tree examples belong to separate libstyx; the gateway connects wire formats without a library dependency. Repository interoperability checks run all five models through the gateway.\n')
        page(base, 'examples/README' + suffix, index)
    gateway_guide(base)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'libunibus')
