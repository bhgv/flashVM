# Live window/control registry

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Index](../README.md)

## Purpose and choice

The backend maintains a small native registry, independent of any GUI toolkit. Writing add/remove commands to ctl publishes/hides entries immediately. Stable IDs are never reused. Hiding a name does not retire an already opened object: catalog_pin/catalog_unpin keep it alive through the original server's open/close callbacks. Adapt these queries to your application's real window/control registry.

## Build and run

```sh
cmake -S . -B build -DSTYX_HOSTED=ON -DBUILD_TESTING=ON  # Hosted example targets / Нативные примеры
cmake --build build -j  # Build the library and examples / Собрать библиотеку и примеры
./build/examples/styx-live 5640  # Keep running in terminal1 / Оставить в терминале1
```

```sh
./build/examples/styx-cat 5640 window-100/title  # Read through original native 9P client / Читать нативным 9P-клиентом
ctest --test-dir build -R '^styx-live$' --output-on-failure  # Independent wire acceptance / Независимая wire-проверка
```

## Parameters

| Parameter | Values | Meaning |
| --- | --- | --- |
| argv1 | `0..65535`, default `0` | TCP port;0 chooses an ephemeral port. First stdout line is the actual port. |
| argv2 | unused | Operator supplies the source explicitly; clients do not supply host paths. |
| names | `ctl; window-ID/title; window-ID/button` | Namespace of this example. |
| file sample | `window-100/title` | Expected text: `Live window` |

## Request flow

Twalk resolves components into a fid. Topen validates mode and pins the object. File Tread uses its held ID and offset. Directory Tread emits only complete Dir entries for immediate children; offset counts encoded bytes. Tclunk/disconnect performs cleanup. The tree may change between requests; no walk is added before every read/write.

## Bounds and replacing the backend

CatalogRow holds a name up to127 UTF-8 bytes and demo text up to2047 bytes; a page has at most16 children. For photos/large content replace text reads with byte-stream/offset reads. 9P does not require a whole file in memory. A listing changed between pages can require rewind to offset0. Example backend calls are synchronous; production asynchronous I/O uses the deferred-read API.

The TSV demo is bounded to128 keys and rereads one flat dataset, not a recursive tree. The live demo has16 visible/held window slots. The HTTP helper bounds responses to128KiB, socket waits to3s and uses a5s process alarm; IDs must preserve exact uint64 values, without JavaScript-double rounding.

## Source and reference

- [Server callbacks](../server.c), [catalog_query / pin / unpin contract](../catalog.h), [backend implementation](../live.c).
- [styxinit](../../docs/reference/styxinit.md), [convD2M](../../docs/reference/convD2M.md), [convM2S](../../docs/reference/convM2S.md).
- [Original client](../client.c), [independent wire client](../wire_client.py), [acceptance](../check.py).
- [Main-project native Styx help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md).

## Changing the registry

```python
from wire_client import Client  # PYTHONPATH=examples; standard 9P client / Стандартный клиент
c = Client(5640)  # Own one connection / Владеть соединением
c.write('ctl', 'add')  # Publish a new window / Опубликовать новое окно
print(c.directory('/'))  # Observe current names / Увидеть текущие имена
c.write('ctl', 'remove 100')  # Hide name, retain open objects / Скрыть имя, удержать open-объекты
c.close()  # Retire fids and connection / Освободить fid и соединение
```
