# Stopped MovieClip ticks with a shared work limit

[Русский](README.ru.md). This executable C89 host demonstrates onEnterFrame and
partial completion with [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L66). A stopped one-frame clip still
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
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Fill the configuration ABI before changing instruction_limit; config is host storage. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Copy configuration into a new runtime; synchronous acquisition, no thread/device started. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Create the runtime-owned independent managed heap; owner-thread use. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Install standard classes and methods; propagate installation errors before creating the clip. |
| [fvm_movieclip_create](../../../include/flashvm/movieclip.h#L36) | Create a context-owned one-frame clip; returned pointer is borrowed until retained. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Retain the clip graph across host calls; destruction releases this example's pin. |
| [fvm_object_call](../../../include/flashvm/value.h#L126) | Invoke stop synchronously on the clip; ignored output is borrowed, no argument values retained. |
| [fvm_object_define_method_rooted](../../../include/flashvm/value.h#L115) | Store native onEnterFrame registration on the clip; borrowed counter userdata, null managed capture. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L66) | Synchronously deliver ticks with shared native/AVM1 work; failure preserves completed effects and skips the remainder. |
| [fvm_value_undefined](../../../include/flashvm/value.h#L40) | Initialize the callback's ignored output without allocating or transferring ownership. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release the managed graph/registration before borrowed userdata expires; do not call during a callback. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Release the outer runtime after its context; no callback or device remains. |

Five units is an intentionally exact regression example, not a time duration or
universal instruction count for scenes with scripts/children. [FVM_ERROR](../../../include/flashvm/flashvm.h#L43) is expected
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
| [`fvm_value_string`](../../../include/flashvm/value.h#L46) | Prepare a static child-name argument; the receiving construction call copies retained names. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Prepare a nonmanaged signed-depth scalar in caller-owned [FVM_Value](../../../include/flashvm/value.h#L24) storage. |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | Construct each child synchronously through createEmptyMovieClip; root owns a successful borrowed result. |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../../../include/flashvm/flashvm.h#L43) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../../../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:36](../../../include/flashvm/movieclip.h#L36) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../../../include/flashvm/value.h#L24) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
