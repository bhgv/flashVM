# Frontend tutorials / Примеры фронтендов

These are independent host examples for the same public FlashVM C89 ABI.
They demonstrate **real frame consumption and input delivery**, not full Flash
player compatibility. The VM never owns a window, actor or GLES context.

| Frontend | Purpose / Назначение | Build target |
| --- | --- | --- |
| [clutter3d](clutter3d/README.md) | Clutter/Cogl texture actor, stage→actor→world input, first touch | flashvm_example_clutter3d (opt-in SDK) |
| [gtk3](gtk3/README.md) | GTK3 DrawingArea/Cairo, scaled pointer and focus loss | flashvm_example_gtk3 (opt-in SDK) |
| [headless](headless/README.md) | Synchronous memory consumer, raster/pointer checks without a window | flashvm_examples_frontends_headless_main |
| [linux_fb](linux_fb/README.md) | Linux fbdev, evdev/mousedev, optional V4L2 MMAP preview | flashvm_example_linux_fb (Linux opt-in) |
| [qt5](qt5/README.md) | Qt5 QWidget/QImage, scaled pointer and deactivation | flashvm_example_qt5 (opt-in SDK) |
| [sdl](sdl/README.md) | SDL2 streaming texture, logical mouse and first-contact touch | flashvm_example_sdl (opt-in SDK) |

`common/demo.h` is shared tutorial implementation, not a separate frontend.
The SDL/Clutter examples show a square at the context's pointer position:
green when released, red while held. Native event loops deliver transitions
in order on the VM owner thread; each frame reads the VM position, rasterizes
RGBA, and calls a synchronous host consumer. Focus/contact cancellation is an
explicit synthetic-up policy at the last valid world coordinate, not Flash
click/focus emulation. Keyboard/IME, movie playback, audio clocks and gesture
recognition are intentionally separate tutorials/subsystems.

Русский: здесь собраны именно хосты вывода/ввода. `headless` перенесён из
`embedding/host_backend`; старое имя CMake заменено указанным в таблице.
Остальных самостоятельных оконных фронтендов среди примеров не найдено.
Ролики `examples/player/animation_audio` — входные SWF-файлы, а примеры аудио,
шрифтов и рендеринга — отдельные API, поэтому они остаются в своих разделах.
SDL/Clutter показывают квадрат: зелёный без контакта, красный при нажатии.
Нативные события обрабатывает поток VM. Код каждой строки объяснён; общая
часть находится в [common/demo.h](common/demo.h).

## Build dependencies / Зависимости

Ordinary builds need no window SDK or direct device access. GTK3, Qt5 and Linux device options also default OFF. SDK options default OFF and are
read only with FVM_BUILD_EXAMPLES=ON. A graphical tutorial requires GRAPHICS_AGG
or GRAPHICS_CAIRO; enabling one under NO_GRAPHICS is a configure error. The
headless example instead explicitly returns/skips 77 without a raster backend.
Third-party window SDKs are explicit external dependencies, never inferred from
sibling repositories. VM and bundled raster dependencies remain static. These
SDK demonstration executables may use system shared SDKs; they are not static
release packages or Android APKs. Supply a static SDK/toolchain and all of its
transitive dependencies separately when preparing such a release.

Обычная сборка не требует SDK. Для оконных примеров включаются отдельные
опции; наличие SDK проверяется явно. Внешний SDK может быть системным shared
для учебного запуска; это не утверждение о готовом статическом выпуске.
Компонент можно скопировать/переименовать: соседние проекты не требуются.

## Common ownership and operations / Общая часть

`Demo` owns runtime, context, RGBA allocation and first-error status. Initialize
with demo_open, stop **all** event/timer callbacks, then call demo_close even
on partial failure. No managed object pointer escapes, so no GC pin is needed.
Never call demo_close or destroy context from a frame callback. Consumer pixels
are borrowed only through return; copy for asynchronous uploads. A renderer
may bound invisible marker geometry but preserves raw outside-world VM input.

| Helper | Parameters and behavior / Параметры и действие |
| --- | --- |
| [demo_open](common/demo.h#L48) | `demo`: fresh caller-owned structure; allocates a 320×200 RGBA span and independent VM/context; returns/stores first status. Caller cleans partial acquisition. |
| [demo_close](common/demo.h#L38) | `demo`: initialized or zeroed structure; releases context, runtime, pixels, clears fields; call only after callbacks stop. |
| [demo_pointer](common/demo.h#L69) | `demo`, event `type`, borrowed `world_to_event`, finite event `x,y`: invert exactly once, synchronously dispatch, update held color state on success; preserve first failure. |
| [demo_cancel](common/demo.h#L83) | `demo`: if held, synthesize UP at last valid VM position and clear native hold. Cancellation policy is explicit, no automatic click or contact promotion. |
| [demo_frame](common/demo.h#L94) | `demo`, `present`, `userdata`: query VM position, draw initialized RGBA, submit real span; sink runs synchronously, error stops loop, count increments only on success. |

`Demo.frame` describes width/height in pixels and stride in bytes; byte_count
is the actual allocation span. `frames` counts successful uploads, not GPU
scanout. `status` latches the first error. `held` controls color only.
The remaining limits, colors and pacing are documented beside their semantic
macros in common/demo.h. C malloc/free/memset manage native storage; no VM
allocator ownership is transferred to a window library.

## Public FlashVM API used by the shared host

Every call below runs on the same owner thread. The table describes arguments,
return/ownership and links to the detailed contract and real declarations.

| API | Parameters, behavior and lifetime |
| --- | --- |
| [fvm_config_init](../../include/flashvm/flashvm.h#L62) | `config`: writable native configuration storage; initialize ABI defaults before runtime creation; returns status, acquires no runtime. [Embedding](../../doc/EMBEDDING_API.md). |
| [fvm_runtime_create](../../include/flashvm/flashvm.h#L64) | `config`: initialized borrowed settings; `out`: writable runtime owner; allocates outer VM; release only after its contexts. [Embedding](../../doc/EMBEDDING_API.md). |
| [fvm_context_create](../../include/flashvm/flashvm.h#L68) | `runtime`: live owner; `out`: writable context pointer; allocates independent pointer/script state, borrows runtime lifetime. [Embedding](../../doc/EMBEDDING_API.md). |
| [fvm_input_set_pointer](../../include/flashvm/input.h#L72) | `context`, finite world `x,y`: silently publish initial center, no callback/OS query; invalid input preserves state. [Input](../../include/flashvm/input.h). |
| [fvm_host_pointer_to_world](../../include/flashvm/host.h#L26) | borrowed world→event affine, finite event `x,y`, distinct writable world outputs: inverse conversion, no allocation; invalid/singular input preserves outputs. [Host coordinates](../../doc/HOST_BACKENDS.md#event-coordinates). |
| [fvm_input_dispatch_mouse](../../include/flashvm/mouse.h#L26) | live `context`, DOWN/MOVE/UP `type`, finite world `x,y`: publish coordinates and synchronously deliver existing Mouse listeners/drag; returns real VM status. No listener is needed to publish position. [Mouse](../../doc/MOUSE.md#delivery). |
| [`fvm_input_dispatch_mouse_wheel`](../../include/flashvm/mouse.h#L33) | Live owner-thread context, finite signed scroll-line delta and world x/y: snapshot/GC-safe onMouseWheel(delta), pointer/drag publication; invalid input or preparation OOM preserves state, callback errors retain committed effects. [Wheel contract](../../doc/MOUSE.md#wheel-events). |
| [fvm_input_get_pointer](../../include/flashvm/input.h#L75) | `context`, distinct writable `x,y`: copy latest world pixels without OS call/allocation. [Input](../../include/flashvm/input.h). |
| [fvm_raster_fill_polygon](../../include/flashvm/raster.h#L39) | borrowed writable `surface`, finite `points`, vertex `count`, straight-alpha `color`: synchronous nonzero fill clipped to image; retains nothing, unsupported without graphics. [Raster](../../include/flashvm/raster.h). |
| [fvm_host_present](../../include/flashvm/host.h#L18) | borrowed `surface`, actual available `byte_count`, synchronous `callback`, borrowed `userdata`: validate span/stride then call once; return real consumer status; no retained frame. [Frame contract](../../doc/HOST_BACKENDS.md#frame-delivery). |
| [FVM_HostPresentCallback](../../include/flashvm/host.h#L11) | receives copied descriptor and borrowed pixels plus unchanged userdata; upload/copy before return, report [FVM_Result](../../include/flashvm/flashvm.h#L40); do not retain source or destroy its owner. [Frame contract](../../doc/HOST_BACKENDS.md#frame-delivery). |
| [fvm_context_destroy](../../include/flashvm/flashvm.h#L70) | live acquired `context`: release VM state after callbacks stop; pointer invalid afterward. [Embedding](../../doc/EMBEDDING_API.md). |
| [fvm_runtime_destroy](../../include/flashvm/flashvm.h#L66) | acquired `runtime`: release outer owner after context destruction; pointer invalid afterward. [Embedding](../../doc/EMBEDDING_API.md). |

These tutorials do not install Mouse script namespaces or register receivers.
For that next integration step use [mouse_listeners](../embedding/mouse_listeners/README.md).
Полные контракты на русском: [host](../../doc/HOST_BACKENDS.ru.md),
[Mouse](../../doc/MOUSE.ru.md). Ошибки не подменяются успешным пустым кадром.
## GTK3, Qt5 and direct Linux additions / Дополнительные фронтенды

The [GTK3](gtk3/README.md), [Qt5](qt5/README.md) and [Linux devices](linux_fb/README.md)
examples share the same portable Demo and public ABI. Each owns its event loop and
native resources, explains each source line and lists every native API with
parameters and documentation. GTK3/Qt5 demonstrate real SDK paint paths; linux_fb
uses actual kernel device protocols, never a simulated successful device.
Qt5's host alone uses its SDK-required C++11 dialect; VM implementation remains
C++98 and public ABI C89. Direct Linux device validation does not certify hardware.

Русский: GTK3/Qt5 — отдельные исполняемые оконные примеры, Linux — консольный
frontend без оконного SDK. Каждый включается собственной опцией CMake и не
становится обязательной зависимостью VM. Подробности, ограничения и команды
находятся в README каждого примера. Общая часть по-прежнему использует прямую
доставку Mouse; миграция на общую очередь — отдельный шаг.
## Wheel normalization (v0.607) / Единицы колеса

[common/wheel.h](common/wheel.h) supplies demo_wheel(demo, world_to_event, x, y,
delta): borrow live Demo/forward transform, inverse-map native event position once,
and synchronously dispatch finite normalized lines through the public VM API.
It latches the first error and records successful wheel_events/last_wheel_delta;
button held state is unchanged. No wheel script listener is installed by the
marker demo; the separate [wheel tutorial](../embedding/mouse_wheel/README.md)
demonstrates receivers and the copied FIFO. These GUI examples retain direct
owner-thread delivery, not an implicit pump or cross-thread producer.

All four adapters use an explicit **3 lines per detent** tutorial policy, without
querying OS scroll preferences. SDL2 converts vertical wheel units, honors FLIPPED,
uses preciseY when SDK >=2.0.18 and last accepted logical VM position for baseline
SDK compatibility. GTK3 converts UP/DOWN and negated smooth Y units at event
coordinates. Qt5 converts angleDelta.y/120 into detents; zero-angle/pixel-only and
horizontal-only streams remain unhandled, not relabeled as lines. evdev prefers
advertised REL_WHEEL_HI_RES (120 counts/detent), ignores its legacy compatibility
packets even in separate reports, otherwise uses REL_WHEEL detents. Completed
SYN_REPORT publishes position then vertical wheel; partial/drop packets do not
produce a wheel. Three-byte mousedev, headless and Clutter adapters gain no native
wheel decoder here. Clutter SDK/device and other OS acceptance remain open.

Русский: единицы ОС не попадают в VM без явного перевода. Дробь сохраняется,
горизонталь и пиксельный-only Qt поток не выдаются за строки. В evdev источник
выбирается по capabilities устройства, чтобы legacy и high-resolution не дали
два события. Счётчики вывода показывают успешные доставки и последнюю дельту,
не физические щелчки. Нажатие/фокус/scrollTarget и прокрутка содержимого не
синтезируются. Сенсорный хост без колеса продолжает передавать основной контакт.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../include/flashvm/flashvm.h#L70) |
| `fvm_host_pointer_to_world` | function | [include/flashvm/host.h:26](../../include/flashvm/host.h#L26) |
| `fvm_host_present` | function | [include/flashvm/host.h:18](../../include/flashvm/host.h#L18) |
| `FVM_HostPresentCallback` | callback typedef | [include/flashvm/host.h:11](../../include/flashvm/host.h#L11) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:26](../../include/flashvm/mouse.h#L26) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:33](../../include/flashvm/mouse.h#L33) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../../include/flashvm/input.h#L75) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../../include/flashvm/input.h#L72) |
| `fvm_raster_fill_polygon` | function | [include/flashvm/raster.h:39](../../include/flashvm/raster.h#L39) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../../include/flashvm/flashvm.h#L40) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../include/flashvm/flashvm.h#L66) |
<!-- source-links:end -->
