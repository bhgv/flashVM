# Live namespace: windows and controls

Copyright (c) 2026 bhgv

[English](README.md) · [Русский](README.ru.md) · [Examples](../README.md)

## What and why

This server demonstrates the difference between a name and an already opened object. A window has a directory and two files: title and button. Writing ctl adds a window or hides its name. It models an application registry without a GUI toolkit, DisVM or FlashVM; sensors, jobs or database records can replace windows.

Fresh Twalk and root Tread operations see current names. An opened fid retains its object ID until Tclunk or connection loss. Removing a name never redirects that fid to a new window. The tree is neither frozen nor transmitted wholesale.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON
cmake --build build -j
./build/examples/styx-live 5640
```

Run from the package root. Leave the server running in the first terminal; in another:

```sh
./build/examples/styx-cat 5640 window-100/title
ctest --test-dir build -R '^styx-live' --output-on-failure
```

Expect `Live window` without a trailing LF. The port is the only meaningful parameter: decimal 0..65535, default 0 selects an available port. The first server stdout line contains the actual port. Choose an available port and pass the same value to the client. Startup failure exits nonzero; Ctrl-C terminates this example. A second backend source argument is unused. The hosted profile requires Linux64/libc/pthread; the minimal codec does not run this server.

## Names, permissions and IDs

| Path | mode | ID / contents |
| --- | --- | --- |
| / | directory 0555 | 0 |
| /ctl | 0222 | 1 |
| /window-N | directory 0555 | N |
| /window-N/title | 0444 | N+1 / `Live window` |
| /window-N/button | 0444 | N+2 / `enabled` |

The initial N is 100. Each successful add reserves the next three IDs: 103, 106 and so on. Failed add does not consume IDs. There are 16 slots, including hidden windows retained by opened fids. The ID counter never wraps: exhaustion fails. The server checks mode; ctl also checks command syntax and meaning. This is demonstration policy, not authentication or user isolation.

## ctl

```text
add
remove <decimal-window-ID>
```

One Twrite carries one complete command, strictly at offset 0. One optional trailing LF or CRLF is allowed. The length is 1..79 bytes including the ending; Rwrite acknowledges the entire original length only after successful mutation. Commands are not accumulated across writes.

The ID contains only decimal digits, is positive and fits uint64; leading zeroes are allowed. A sign, extra spaces, NUL, internal CR/LF, multiple commands, overflow, unknown/already hidden ID or unavailable slot produces Rerror without changing the registry. Sending `add\nadd` executes neither command. Rerror does not close a healthy connection.

## Worked scenario

In the second terminal, with PYTHONPATH=examples. Each successful wire_client open returns an owned fid; clunk releases it, and finally closes the connection even on failure.

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

Root listing order may differ; compare the set of names. Run against a fresh server: IDs are monotonic and do not reset between clients.

## Backend and lifetime

[catalog_query](../live.c#L73) `(listing,id,start,out,capacity,count)` returns stat or a page of immediate children; start counts rows, not 9P byte offsets. The server encodes whole Dir records and converts the byte offset into page traversal. There is no recursive preload. A listing changed between reads may require rewinding offset=0.

[catalog_pin](../live.c#L236) `(id)` retains a shared window slot for directory/title/button; [catalog_unpin](../live.c#L262) `(id)` releases one successful retention. Remove hides the name immediately and frees the slot only without references. [catalog_init](../live.c#L36) `(source)` runs once before requests and never retains source. This is the private example backend API, not an installed library ABI. One server owner serializes query/write/pin/unpin; an external mutating thread needs synchronization. Client execution threads are not paused.

## Checks and sources

`styx-live-registry` checks grammar, bounded pages, 16 slots, two pins of one window, final close and fresh identity after reuse. `styx-live-wire` uses two actual 9P sessions: Rerror preserves both registry and connection; one client cannot destroy another client’s opened fid by hiding its name. The original `styx-live` test checks basic navigation and updates.

[Registry source](../live.c) · [Backend declarations](../catalog.h) · [Server callbacks](../server.c) · [Registry tests](../../tests/live_registry.c) · [Wire tests](../../tests/live_wire.py) · [Client helper](../wire_client.py)

[styxinit](../../docs/reference/styxinit.md) · [styxprocess](../../docs/reference/styxprocess.md) · [styxend](../../docs/reference/styxend.md) · [Main-project Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
