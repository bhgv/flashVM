# Copyright (c) 2026 bhgv
# EN: Authored bilingual custom-carrier tutorial; source anchors are checked against actual definitions.
# RU: Авторское EN/RU руководство custom carrier; source anchors сверяются с настоящими определениями.
from pathlib import Path
import re
from .introductions import page


# EN: Link borrowed name to its definition in base/filename; fail instead of guessing a source line.
# RU: Связать name с определением в base/filename; при отсутствии не выдумывать номер строки.
def source_link(base, filename, name):
    source = (base / filename).read_text()
    match = re.search(r'^.*\b' + re.escape(name) + r'\([^;\n]*\)\n\{', source, re.M)
    if match is None:
        raise ValueError(f'{filename}: missing {name}')
    line = source.count('\n', 0, match.start()) + 1
    return f'[{name}]({filename}#L{line})'


# EN: Write EN/RU memory-carrier READMEs beneath borrowed Styx root. Runtime code is never generated here.
# RU: Записать EN/RU README memory carrier под Styx root. Runtime-код здесь не генерируется.
def main(root):
    base = root / 'examples/memory_phy'
    functions = {name: source_link(base, filename, name) for filename, names in [
        ('memory.c', ['memory_pair', 'memory_destroy', 'memory_disconnect']),
        ('io.c', ['memory_read', 'memory_write']), ('wait.c', ['memory_wait', 'memory_wake']),
        ('main.c', ['use_file']), ('server.c', ['memory_server'])] for name in names}
    for ru in (False, True):
        suffix = '.ru.md' if ru else '.md'
        api = lambda name: f'[{name}](../../docs/reference/{name}{suffix})'
        text = '# ' + ('9P через custom memory PHY' if ru else '9P over a custom memory PHY') + '\n\nCopyright (c) 2026 bhgv\n\n'
        text += f'[English](README.md) · [Русский](README.ru.md) · [Examples](../README{suffix}) · [Library](../../README{suffix})\n\n'
        text += '## ' + ('Что и зачем' if ru else 'Purpose') + '\n\n'
        text += ('Настоящий native 9P-клиент и исходный libstyx server обмениваются wire-сообщениями через ограниченные буферы памяти. Носитель не создаёт socket, pipe, eventfd или файловых дескрипторов. Консольный stdout используется только для PASS/диагностики процесса. VM, GUI и Unibus для этого примера не нужны.\n\nЭто пример подключения своего драйвера через PhyOps, а не новый 9P codec. Файловые операции остаются обычными: Tversion, Tattach, Twalk, Topen, Tstat, Tread, Tclunk. Физически оба направления доступны, но это обычная client/server сессия; PHY_BIDIR_SESSION и скрытый reverse-server не включаются.\n\n' if ru else 'A real native 9P client and the original libstyx server exchange wire messages through bounded memory buffers. The carrier creates no socket, pipe, eventfd or file descriptor. Console stdout is only a process PASS/diagnostic channel. No VM, GUI or Unibus is needed.\n\nThis demonstrates a custom PhyOps driver, not a new 9P codec. Operations remain ordinary Tversion, Tattach, Twalk, Topen, Tstat, Tread and Tclunk. Bytes flow both ways, but this is a conventional client/server session: no PHY_BIDIR_SESSION or hidden reverse server is enabled.\n\n')
        text += '## ' + ('Сборка и запуск' if ru else 'Build and run') + '''

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-memory-phy
ctest --test-dir build -R '^styx-memory' --output-on-failure
```

'''
        text += ('Из корня пакета. Аргументов командной строки нет. Ожидается PASS real 9P over memory PHY; код 0 — RPC и окончательное освобождение обоих endpoints проверены. Код 1/диагностика stderr — ошибка setup/RPC/lifetime. Ошибка native join завершает процесс через _Exit, без освобождения памяти потенциально живого worker.\n\nПример Linux-only: pthread, GCC/Clang atomics и binary futex wait/wake. Это зависимости учебного драйвера, не PhyOps ABI или portable codec. Минимальный STYX_HOSTED=OFF не собирает этот пример и остаётся без таких зависимостей.\n\n' if ru else 'Run from the package root. There are no command-line arguments. Expect PASS real 9P over memory PHY; exit 0 verifies RPCs and final destruction of both endpoints. Exit 1/stderr identifies setup/RPC/lifetime failure. Native join failure terminates via _Exit rather than reclaiming a possibly live worker\'s memory.\n\nThe driver is Linux-only: pthread, GCC/Clang atomics and binary futex wait/wake. These are example dependencies, not PhyOps ABI or portable-codec requirements. STYX_HOSTED=OFF excludes the example and retains its minimal dependency profile.\n\n')
        text += '## ' + ('Состав и настройки' if ru else 'Files and settings') + '\n\n'
        text += ('''| Файл/параметр | Значение и назначение |\n| --- | --- |\n| memory.c | Factory, adoption, shared lifetime |
| io.c | Byte prefixes, circular buffers, backpressure |
| wait.c | Native wait/wake без fd; один monotonic deadline на wait |
| server.c / main.c | Исходный libstyx server и реальные client RPC |
| MEMORY_CAPACITY | 16 входных байтов на направление; не размер 9P frame |
| max_read / max_write | 3/2 байта на callback; границы фрагментов не границы сообщений |
| cut_after | 0 в нормальном примере; тестовое значение 5 моделирует подтверждённый prefix с ERROR/UNCERTAIN |
| MemoryStats | Нулевой caller-owned объект, живущий до destroy обоих endpoints |
| msize / timeout / limit | 256 / 3000 мс / 4 для native клиента; не глобальные значения протокола |
''' if ru else '''| File/parameter | Value and purpose |\n| --- | --- |\n| memory.c | Factory, adoption, shared lifetime |
| io.c | Byte prefixes, circular buffers, backpressure |
| wait.c | Descriptor-free native wait/wake; one monotonic deadline per wait |
| server.c / main.c | Original libstyx server and real client RPCs |
| MEMORY_CAPACITY | 16 incoming bytes per direction, not the 9P frame size |
| max_read / max_write | 3/2 bytes per callback; fragment boundaries are not message boundaries |
| cut_after | 0 normally; test value 5 models a confirmed prefix with ERROR/UNCERTAIN |
| MemoryStats | Zeroed caller-owned observations surviving both endpoint destroys |
| msize / timeout / limit | 256 / 3000 ms / 4 for the native client, not protocol-global settings |
''')
        text += '\n## ' + ('Владение и последовательность' if ru else 'Ownership and flow') + '\n\n'
        if ru:
            text += f'''1. {functions['memory_pair']} создаёт два owned RemotePhy. {api('phycreate')} копирует ops/info и принимает state только при успехе; factory освобождает partial construction самостоятельно.
2. Server thread принимает один endpoint через {api('styxphyinit')}; main передаёт другой в {api('styxclientmountphy')}. Оба constructor поглощают grant при любом исходе.
3. {functions['memory_server']} владеет server/ops/file table до styxend. Main ждёт фактического startup через native condition, не обращаясь к активному server объекту.
4. {functions['use_file']} получает отказ write-open на readonly status, затем отдельным fid выполняет read/stat, EOF на signed64 max offset и чтение с offset 7. Rerror открытия не разрушает исправное соединение.
5. {api('styxclientshutdown')} и {api('styxclientfree')} завершают client owner. Peer наблюдает disconnect, server заканчивает работу; main делает join до освобождения startup state.
6. {functions['memory_destroy']} освобождает shared allocation только после второго endpoint. Wake callers обязаны закончить до final free; buffered prefix может пережить уничтожение другого endpoint.

'''
        else:
            text += f'''1. {functions['memory_pair']} creates two owned RemotePhy endpoints. {api('phycreate')} copies ops/info and adopts state only on success; the factory unwinds partial construction itself.
2. The server thread gives one endpoint to {api('styxphyinit')}; main gives the other to {api('styxclientmountphy')}. Both constructors consume their grants on every outcome.
3. {functions['memory_server']} owns server/ops/file table through styxend. Main waits for actual startup through a native condition without touching the active server object.
4. {functions['use_file']} observes denied write-open on readonly status, then uses a separate fid for read/stat, EOF at signed64 max offset and read at offset 7. Open Rerror does not destroy the healthy connection.
5. {api('styxclientshutdown')} and {api('styxclientfree')} retire the client owner. The peer observes disconnect and ends; main joins before reclaiming startup storage.
6. {functions['memory_destroy']} releases the shared allocation only after the second endpoint retires. Wake callers must finish before final free; buffered prefixes can outlive the opposite endpoint.

'''
        text += '## ' + ('Контракт драйвера' if ru else 'Driver contract') + '\n\n'
        text += ('Read/write используют trylock и не ждут peer consumption. Пусто/полно/контеншн дают AGAIN с нулевым count, не EOF. Положительный count — принятый prefix. При cut_after возвращаются count и ERROR/UNCERTAIN вместе: prefix сохраняется, повторять весь запрос нельзя. После peer disconnect сначала дочитываются принятые байты, затем выдаётся EOF.\n\nWake — sticky local work, не wire-сообщение. wait сначала сбрасывает binary latch, потом проверяет persistent predicates и wake flag; kernel отказывается спать, если notifier уже изменил latch. Здесь нет 32-битного номера очереди или счётчика поколений: слово futex принимает только 0/1. Timeout 0 — poll, -1 — infinite wait; положительный timeout использует один CLOCK_MONOTONIC deadline на все EINTR/повторы. Это не обещание realtime scheduler latency.\n\nPHY_STREAM/ORDERED/RELIABLE объявлены потому, что memory carrier сохраняет byte order и сообщает отказ. PHY_PACKET не подходит напрямую 9P. Настоящим SPI/I2C/CAN/радиоканалам могут требоваться reassembly, упорядочивание, контроль ошибок/потерь и half-duplex arbitration ниже этого интерфейса. Не выставляйте флаги вместо реализации этих свойств. connected в PhyInfo — local lifecycle, не heartbeat, разрешение доступа или доказательство состояния peer.\n\n' if ru else 'Read/write use trylock and never wait for peer consumption. Empty/full/contention returns AGAIN with zero count, not EOF. A positive count is the accepted prefix. At cut_after, count and ERROR/UNCERTAIN coexist: retain the prefix and never replay the whole request. Peer disconnect first permits draining accepted bytes, then EOF.\n\nWake is sticky local work, not a wire message. Wait clears its binary latch before inspecting persistent predicates/wake; the kernel refuses to sleep if a notifier already changed the latch. The futex word is only 0/1, not a 32-bit queue ordinal or generation counter. Timeout 0 polls, -1 waits indefinitely; positive timeouts share one CLOCK_MONOTONIC deadline across EINTR/retries. This does not promise realtime scheduler latency.\n\nPHY_STREAM/ORDERED/RELIABLE are declared because memory preserves byte order and reports failure. PHY_PACKET is not directly suitable for 9P. Real SPI/I2C/CAN/radio links may need reassembly, ordering, error/loss handling and half-duplex arbitration below this interface. Setting flags does not implement those properties. PhyInfo.connected is local lifecycle, not heartbeat, access permission or proof of peer state.\n\n')
        text += '## ' + ('Проверки и границы' if ru else 'Checks and limits') + '\n\n'
        text += ('styx-memory-construction внедряет ошибки allocation/mutex и первого/второго adoption, проверяя final frees. styx-memory-contract проверяет bounded prefixes, full/AGAIN, drain/EOF, uncertainty, timeout и 500 concurrent wake rounds без sleep. styx-memory-no-descriptor запускает тот же client/server с link-time отказом socket/connect/open/close/pipe/eventfd/read/poll путей проекта. Это guard linked code, не трассировка всех внутренних kernel calls libc.\n\nПример не реализует authentication, VFS union, реальный DMA/IRQ, потери пакетов, драйвер платы или remote-module migration. Вместо этого он оставляет заменяемую границу: замените PhyOps/context и способ создания endpoint, сохранив клиент/server и wire-протокол. Не переносите Linux futex-код на MCU без собственного native wait/wake adapter.\n\n' if ru else 'styx-memory-construction injects allocation/mutex and first/second adoption failure, checking final frees. styx-memory-contract checks bounded prefixes, full/AGAIN, drain/EOF, uncertainty, timeout and 500 concurrent wake rounds without sleeps. styx-memory-no-descriptor runs the same client/server with link-time refusal of project socket/connect/open/close/pipe/eventfd/read/poll paths. This guards linked code, not every libc-internal kernel call.\n\nThe example does not implement authentication, VFS unions, real DMA/IRQ, packet loss, board drivers or remote-module migration. It leaves a replaceable boundary: change PhyOps/context and endpoint construction while retaining client/server and wire protocol. An MCU needs its own native wait/wake adapter, not a copied Linux futex implementation.\n\n')
        text += f'[Driver interface](../../remote_phy/phy.h) · [Source factory](memory.c) · [SDK integration](../../docs/embedding{suffix}) · [Main-project carrier guide](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)\n'
        page(root, 'examples/memory_phy/README' + suffix, text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1] / 'styx')
