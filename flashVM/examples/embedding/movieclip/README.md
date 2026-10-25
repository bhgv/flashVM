# Stopped MovieClip ticks with a shared work limit

[Русский](README.ru.md). This executable C89 host demonstrates onEnterFrame and
partial completion with [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160). A stopped one-frame clip still
receives tick handlers. Three requested ticks share five work units: two callbacks
complete; the third node entry consumes the last unit and its handler is skipped.
A new outer call resets the budget and delivers one further callback.

## Build and run

From the independent FlashVM component root (no renderer/SDL/device required):

```sh
cmake -S . -B build-ticks -DDEFS=INTERPRETER -DFVM_BUILD_EXAMPLES=ON
cmake --build build-ticks --target flashvm_examples_embedding_movieclip_main
./build-ticks/flashvm_examples_embedding_movieclip_main
```

Expected output and exit code zero:

```text
bounded batch: 2 callbacks retained
after next call: 3 callbacks
depth order: low middle high
render: no timeline callbacks
frame call: frame 2 actions; playhead 1; callbacks 0
initializers: next two ticks fit five units
unload: instance and property retained
```

[main.c](main.c) is executable; [tutorial_limits.h](tutorial_limits.h) names its
intentional bounds. [example_as2.as](example_as2.as) remains a separate integration
sketch requiring an external AS compiler; it is not executed by this CMake target.
The example does not render, load a SWF, poll timers or demonstrate OS input.

## API and ownership

Detailed contract: [MovieClip ticks](../../../doc/MOVIECLIP_TICKS.md).
Every VM call runs on the owner thread. The host counter outlives context teardown;
the callback borrows it. The clip is pinned; context destruction releases the pin,
registration and graph before the counter goes out of scope. Ordinary callback GC
barriers remain active. There is no automatic retry of the failed batch.

| Demonstrated API | Purpose and lifetime |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Fill the configuration ABI before changing instruction_limit; config is host storage. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Copy configuration into a new runtime; synchronous acquisition, no thread/device started. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Create the runtime-owned independent managed heap; owner-thread use. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Install standard classes and methods; propagate installation errors before creating the clip. |
| [fvm_movieclip_create](../../../include/flashvm/movieclip.h#L120) | Create a context-owned one-frame clip; returned pointer is borrowed until retained. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L60) | Retain the clip graph across host calls; destruction releases this example's pin. |
| [fvm_object_call](../../../include/flashvm/value.h#L184) | Invoke stop synchronously on the clip; ignored output is borrowed, no argument values retained. |
| [fvm_object_define_method_rooted](../../../include/flashvm/value.h#L171) | Store native onEnterFrame registration on the clip; borrowed counter userdata, null managed capture. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160) | Synchronously deliver ticks with shared native/AVM1 work; failure preserves completed effects and skips the remainder. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L46) | Initialize the callback's ignored output without allocating or transferring ownership. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) | Release the managed graph/registration before borrowed userdata expires; do not call during a callback. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Release the outer runtime after its context; no callback or device remains. |

Five units is an intentionally exact regression example, not a time duration or
universal instruction count for scenes with scripts/children. [FVM_ERROR](../../../include/flashvm/flashvm.h#L45) is expected
only at the deliberate batch boundary; every other error follows normal cleanup.
Full reference ordering, ClipActions, media, JIT/platform acceptance and all roadmap
stages remain open. [Catalog](../../README.md#example-embedding-movieclip).


The v0.629 addition uses a second independent runtime with a 64-unit budget. It
creates high(10), low(-5), middle(2), registers stack-borrowing native handlers and
verifies low/middle/high delivery. Root pins retain child registrations; context
cleanup precedes expiration of every entry/log. Same-depth replacements or newborns
from sibling callbacks wait for the next snapshot. This is native depth policy,
not full original Player event-order certification. [Contract](../../../doc/TIMELINE_DEPTH_ORDER.md).

| Added API use | Purpose and lifetime |
| --- | --- |
| [`fvm_value_string`](../../../include/flashvm/value.h#L58) | Prepare a static child-name argument; the receiving construction call copies retained names. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L50) | Prepare a nonmanaged signed-depth scalar in caller-owned [FVM_Value](../../../include/flashvm/value.h#L26) storage. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L184) | Construct each child synchronously through createEmptyMovieClip; root owns a successful borrowed result. |

The v0.630 extension renders the empty scene into caller-owned RGBA memory and checks that rendering does not dispatch more timeline callbacks. No backend is needed for empty geometry; real paths need a raster backend. Expected fourth line: `render: no timeline callbacks`. See [raster contract](../../../doc/RASTER_DEPTH_ORDER.md).

| Added API use | Purpose and lifetime |
| --- | --- |
| [`fvm_movieclip_render`](../../../include/flashvm/movieclip.h#L174) | Synchronously borrow the caller-owned surface and pinned scene; no tick or asynchronous memory retention. Errors preserve prior pixels/getter effects. |

v0.631 adds a third independent runtime / третий независимый runtime: define frame
two, invoke its actions twice through the public execution service with a fresh five-unit
limit per outer host call, and verify `called=42`, playhead one and zero tick callbacks.
The fifth output line is shown above. / Пятая строка показана выше.
[EN contract](../../../doc/FRAME_BLOCKS.md) / [RU](../../../doc/FRAME_BLOCKS.ru.md).

| Added API / API | Purpose, ownership and blocking / Назначение и владение |
| --- | --- |
| [`fvm_movieclip_define_frame`](../../../include/flashvm/movieclip.h#L143) | Copy/validate caller-owned AVM1 bytes and label into the clip's graph; synchronous, does not execute code. / Копирует bytes/метку; без исполнения. |
| [`fvm_exec_call_frame`](../../../include/flashvm/execution.h#L456) | Borrow a rooted receiver and scalar operand synchronously; selected actions share one limit, preserve earlier effects on failure, and do not seek/tick. / Синхронно заимствует receiver/операнд; эффекты до ошибки сохраняются, playhead не меняется. |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | Read committed scalar properties into caller-owned values; callbacks/errors follow normal property semantics. / Читает числовой результат и playhead; обычные getters могут вернуть ошибку. |

v0.632 adds a fourth independent runtime / четвёртый независимый runtime. A raw
33-byte SWF8 holds an empty sprite and one End-only DoInitAction. Loading runs no code.
The first tick fits five units; the next outer call completes two ticks in four units,
with three total onEnterFrame callbacks. / Первый tick укладывается в пять единиц,
следующие два вместе — в четыре; итого три onEnterFrame callback. Sixth output line
is shown above. [EN](../../../doc/INITIALIZER_PROGRESS.md) / [RU](../../../doc/INITIALIZER_PROGRESS.ru.md).

| Added API / API | Purpose and ownership / Назначение и владение |
| --- | --- |
| [`fvm_movieclip_load_swf`](../../../include/flashvm/movieclip.h#L129) | Synchronously validate/import caller-owned bytes, retain copied code and return a borrowed context-owned clip; no initializer runs until advance. Pin the graph before later host GC. / Копирует код без исполнения; закрепить возвращённый граф до GC. |
Loaded-scene control snapshots are described in the [contract](../../../doc/SCENE_CONTROLS.md). This example does not edit private metadata; mutation fixtures live in tests/.
## Native content clearing / Очистка содержимого

The executable also prints `unload: instance and property retained` after an
independent safe-point clear. / Пример также очищает экземпляр в безопасной
точке и проверяет сохранение собственного свойства.

| API | Purpose / Назначение |
| --- | --- |
| [`fvm_movieclip_unload`](../../../include/flashvm/movieclip.h#L264) | Atomically clear contents; caller retains the instance. / Атомарная очистка, экземпляр удерживает хост. |
| [`fvm_movieclip_attached_to`](../../../include/flashvm/movieclip.h#L252) | Borrowed native ancestry snapshot; output changes only on success. / Проверка физических связей, выход меняется при успехе. |

Contracts, ownership, errors and remaining lifecycle limits:
[MOVIE_UNLOAD.md](../../../doc/MOVIE_UNLOAD.md) /
[MOVIE_UNLOAD.ru.md](../../../doc/MOVIE_UNLOAD.ru.md).
No network/audio cancellation or synthetic unload event is demonstrated.

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
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../../../include/flashvm/flashvm.h#L45) |
| `fvm_exec_call_frame` | function | [include/flashvm/execution.h:456](../../../include/flashvm/execution.h#L456) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:160](../../../include/flashvm/movieclip.h#L160) |
| `fvm_movieclip_attached_to` | function | [include/flashvm/movieclip.h:252](../../../include/flashvm/movieclip.h#L252) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:120](../../../include/flashvm/movieclip.h#L120) |
| `fvm_movieclip_define_frame` | function | [include/flashvm/movieclip.h:143](../../../include/flashvm/movieclip.h#L143) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:129](../../../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_render` | function | [include/flashvm/movieclip.h:174](../../../include/flashvm/movieclip.h#L174) |
| `fvm_movieclip_unload` | function | [include/flashvm/movieclip.h:264](../../../include/flashvm/movieclip.h#L264) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../../../include/flashvm/value.h#L184) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:171](../../../include/flashvm/value.h#L171) |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../../../include/flashvm/value.h#L26) |
| `fvm_value_number` | function | [include/flashvm/value.h:50](../../../include/flashvm/value.h#L50) |
| `fvm_value_string` | function | [include/flashvm/value.h:58](../../../include/flashvm/value.h#L58) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
<!-- source-links:end -->
