# Explicit retained ClipActions in C89

[Русский](README.ru.md). The host loads an embedded SWF8 sprite with three authored
records: EnterFrame writes `seen=17`; Load writes `seen=42`. A normal tick places
and advances the sprite. The host selects that actor and explicitly delivers Load,
then verifies its marker. It next publishes pointer state, broadcasts retained MouseMove
through the scene and consumes a coalesced redraw request. No window, OS input,
compiler, external file or SDL is needed.

## Build and run

From the independent FlashVM component root:

```sh
cmake -S . -B build-clips -DDEFS=INTERPRETER -DFVM_BUILD_EXAMPLES=ON
cmake --build build-clips --target flashvm_examples_embedding_clip_actions_main
./build-clips/flashvm_examples_embedding_clip_actions_main
```

Expected exit code zero and output:

```text
explicit Load marker: 42
scene MouseMove marker: 84; redraw: 1
scene MouseMove method: 126; redraw: 1
```

[main.c](main.c) is standalone C89; [clip_action_movie.h](clip_action_movie.h) contains
immutable independently encoded wire bytes, not a textual AS program or a borrowed
resource handle. Loading copies the records. The host owns runtime/context and pins
the root, which retains actor/library/code. Context destruction releases the graph
and pin before local storage expires. No pointer is used after cleanup.

## API contracts

Detailed [ClipActions guide](../../../doc/CLIP_ACTIONS.md). Every call runs on the
context owner thread. All delivery is synchronous; partial script effects survive
errors. The native barrier blocks collection during bytecode/callback execution;
collect at host safe points. Explicit Load is repeatable: the embedding host owns
its once-only lifecycle policy. This API does not call a native onLoad method.

| API | Purpose and ownership |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Initialize the host configuration ABI before runtime creation. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Copy configuration and acquire the outer owner; no worker or device is opened. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Acquire the managed heap; use its owner thread for all later calls. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Install classes and methods before constructing the loaded scene. |
| [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L43) | Copy/inspect the embedded SWF and retain private handlers; returned root is borrowed, no script runs while loading. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Keep the complete root/actor/library graph reachable between host calls; this example releases its pin by destroying the context. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L62) | Perform one tick, place the actor and deliver its EnterFrame record; does not automatically deliver Load. |
| [fvm_object_get](../../../include/flashvm/value.h#L72) | Read authored actor and scalar marker synchronously; object results are borrowed and kept reachable through root. |
| [fvm_movieclip_dispatch_clip_actions](../../../include/flashvm/clip_actions.h#L37) | Execute matching retained Load bytecode with the actor receiver and key_code zero; no target selection, native onX call, timeline advance or presentation. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Release graph/registration/pin after calls return; managed pointers become unusable. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Release the outer owner after the context. |

[FVM_CLIP_EVENT_LOAD](../../../include/flashvm/clip_actions.h#L5) selects the raw authored event. Other event bits and KeyPress
selectors are described individually in the public header and guide. The tutorial
checks selected-record, tree and method markers, not original-player event scheduling, networking, input
hit testing, visible output or foreign-platform acceptance. No automatic retry is
performed after an error. [Catalog](../../README.md#example-embedding-clip_actions).
The v0.626 tree path is documented in [CLIP_INPUT.md](../../../doc/CLIP_INPUT.md).
The embedded MouseMove record writes marker 84 and calls updateAfterEvent; the host
observes one pending request. This example intentionally uses raw state publication;
use namespace dispatch first as well when your integration needs its listeners/drag policy.

In v0.627 the example then installs native onMouseMove and calls the combined
route: the record writes 84 again, the method verifies it and writes 126, and both
redraw requests coalesce. [Method contract](../../../doc/CLIP_INPUT_METHODS.md).

| API | Purpose, errors and ownership |
| --- | --- |
| [`fvm_input_set_pointer`](../../../include/flashvm/input.h#L72) | Publish coordinates synchronously; no event delivery or target selection. |
| [`fvm_movieclip_dispatch_clip_input`](../../../include/flashvm/clip_actions.h#L53) | Traverse the retained tree for MouseMove records only; completed effects survive errors. |
| [`fvm_movieclip_dispatch_input`](../../../include/flashvm/clip_actions.h#L68) | Use the same pinned root for records then optional method; errors skip the rest of the batch. |
| [`fvm_stage_take_update`](../../../include/flashvm/stage.h#L45) | Consume one coalesced redraw flag without rendering or advancing timeline. |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | Register the native method on the borrowed actor; userdata/root are null, with no temporary capture. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L74) | Write the scalar marker synchronously; propagate errors without rolling back the preceding record. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | Construct an unmanaged numeric scalar in the local [FVM_Value](../../../include/flashvm/value.h#L24). |
| [`fvm_call`](../../../include/flashvm/value.h#L138) | Invoke updateAfterEvent synchronously inside the event scope; propagate errors without presenting. |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../../../include/flashvm/value.h#L138) |
| `FVM_CLIP_EVENT_LOAD` | macro / define | [include/flashvm/clip_actions.h:5](../../../include/flashvm/clip_actions.h#L5) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../../../include/flashvm/input.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:62](../../../include/flashvm/movieclip.h#L62) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:37](../../../include/flashvm/clip_actions.h#L37) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:53](../../../include/flashvm/clip_actions.h#L53) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:68](../../../include/flashvm/clip_actions.h#L68) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../../../include/flashvm/movieclip.h#L43) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../../../include/flashvm/value.h#L74) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:45](../../../include/flashvm/stage.h#L45) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../../../include/flashvm/value.h#L24) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
