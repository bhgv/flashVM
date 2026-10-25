# Headless display/event host / Хост кадров и событий без SDL

Executable strict-C89 tutorial: [main.c](main.c), every source line commented.
It rasterizes a red square into a host-owned 16×16 RGBA buffer, submits the
actual allocation span through a synchronous consumer, converts event-space
(120,90) into world (10,20), then dispatches a down/up pair into an independent
VM context. The consumer checks the center pixel and retains no pointers.
No files, window, SDL, GPU, platform SDK or script standard namespace is needed.

Исполняемый пример C89 рисует красный квадрат, передаёт RGBA-буфер callback,
обращает матрицу события и доставляет down/up в VM. Каждая строка объяснена.
Это проверка общего контракта, а не реализация Clutter или проигрыватель SWF.

## Build and run / Сборка и запуск

From the component directory, enable GRAPHICS_AGG (default) or GRAPHICS_CAIRO
in DEFS.txt, with examples enabled:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_frontends_headless_main
./build/flashvm_examples_frontends_headless_main
ctest --test-dir build -R '^flashvm_examples_frontends_headless_main$' --output-on-failure
```

Expected output / ожидаемый вывод:

```text
frame 1: 16x16 RGBA center=255,0,0,255
pointer world=10,20
```

Exit 0 means frame and pointer checks passed; ordinary failures exit 1.
NO_GRAPHICS returns 77 and is explicitly skipped by CTest, not accepted as a
successful render. Нужен включённый графический backend; отсутствие графики
явно пропускается. A script listener variant is demonstrated separately by
[mouse_listeners](../../embedding/mouse_listeners/README.md).

## Ownership and integration / Владение и подключение

The stack owns pixels and callback userdata until submission returns; context
is destroyed before runtime on every acquired-owner path. No managed objects
are exposed to the host, so no GC pins are needed. All VM calls run on one
owner thread. The frame callback checks bytes synchronously; replace its body
with a real upload/copy on the graphics-context thread. An asynchronous adapter
must own a copy after returning. Input events from other threads must first be
copied to the VM owner. Compose actor/window scale and translation once before
mapping. The example uses no contact recognition, cancellation, FIFO, keyboard,
IME, audio or SWF scene; no native-device acceptance is claimed.

Стек владеет кадром и userdata до возврата callback; context освобождается
раньше runtime. GC-корни не нужны: managed-объекты наружу не выдаются.
Для GPU замените consumer реальной загрузкой в потоке графического контекста;
для отложенной отправки сохраните собственную копию. Ввод доставляет поток VM.
Полные контракты: [EN](../../../doc/HOST_BACKENDS.md) /
[RU](../../../doc/HOST_BACKENDS.ru.md).

## Individual APIs / Отдельные API

| API | Purpose, ownership and blocking contract |
| --- | --- |
| [`fvm_raster_fill_polygon`](../../../include/flashvm/raster.h#L46) | Synchronously fill the borrowed pixel surface; no retained corners. [Raster API](../../../include/flashvm/raster.h). |
| [`fvm_host_present`](../../../include/flashvm/host.h#L23) | Validate the actual byte span then call the borrowed synchronous consumer; no buffer transfer. [Contract](../../../doc/HOST_BACKENDS.md#frame-delivery). |
| [`FVM_HostPresentCallback`](../../../include/flashvm/host.h#L14) | Read/copy/upload borrowed pixels before return on the caller thread. [Contract](../../../doc/HOST_BACKENDS.md#frame-delivery). |
| [`fvm_host_pointer_to_world`](../../../include/flashvm/host.h#L33) | Convert one finite event using inverse affine; preserve outputs on error, no allocation. [Contract](../../../doc/HOST_BACKENDS.md#event-coordinates). |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Initialize caller-owned configuration without acquiring a VM. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Acquire a runtime, returned through caller storage; release after contexts. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Acquire isolated context input state owned by the runtime. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_input_dispatch_mouse`](../../../include/flashvm/mouse.h#L37) | Synchronously publish/deliver a world-coordinate contact transition on the VM owner. [Mouse](../../../doc/MOUSE.md#delivery). |
| [`fvm_input_get_pointer`](../../../include/flashvm/input.h#L96) | Copy the latest context coordinates without OS queries or allocation. [Input](../../../include/flashvm/input.h). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Release acquired VM state after callbacks finish, before runtime release. [Embedding](../../../doc/EMBEDDING_API.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Release the outer owner after its context has been destroyed. [Embedding](../../../doc/EMBEDDING_API.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:33](../../../include/flashvm/host.h#L33) |
| `fvm_host_present` | function | [include/flashvm/host.h:23](../../../include/flashvm/host.h#L23) |
| `FVM_HostPresentCallback` | callback typedef | [include/flashvm/host.h:14](../../../include/flashvm/host.h#L14) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:37](../../../include/flashvm/mouse.h#L37) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:96](../../../include/flashvm/input.h#L96) |
| `fvm_raster_fill_polygon` | function | [include/flashvm/raster.h:46](../../../include/flashvm/raster.h#L46) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
<!-- source-links:end -->
