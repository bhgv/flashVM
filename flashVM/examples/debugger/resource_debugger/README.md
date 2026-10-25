# Resource-controlled debugger

A real AVM1 function returns 42 after a stop, two independent observers and an
owner-FIFO resume. Only session one has CONTROL. Console output is presentation;
the control interface is C89.

```sh
cmake -S flashVM -B build-debug -DFVM_BUILD_EXAMPLES=ON
cmake --build build-debug --target flashvm_examples_debugger_resource_debugger_main
./build-debug/flashvm_examples_debugger_resource_debugger_main
```

Requires CMake and a C89/C++98 toolchain; use `-S .` inside the component. No
renderer/SDL SDK is needed. Expect two observer lines, then result=42. The host
owns runtime/context, dispatcher and native adapter; the function is pinned.
Detach the callback before adapter destruction, and disconnect sessions before
dispatcher destruction. Service deterministically queues commands here. A real
transport must wait with cancellation, authenticated identity, quotas and lost-
wakeup protection. Never reenter/collect/destroy this context from service. This
example does not open sockets or demonstrate a [FVM_Channel](../../../src/capi/channel_c.c#L30) graph bridge, workers,
remote authentication or 9P.

| API | Purpose and ownership |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L70) | Initialize the owned configuration before runtime creation. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L82) | Allocate the VM context used only by its owner thread. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L86) | Release managed state after callback detachment and unpin. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_debug_set_breakpoint`](../../../include/flashvm/debug.h#L61) | Owner sets an exact bytecode breakpoint; original interpreter fallback applies. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_debug_set_breakpoint_handler`](../../../include/flashvm/debug.h#L66) | Install/clear the borrowed callback explicitly; clear before freeing userdata. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L62) | Copy AVM1 bytecode into a managed function, with caller pinning across setup. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L79) | Execute on the owner; result 42 proves progress after resume. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | Retain the managed function across host allocations and calls. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | Release exactly the host root previously acquired. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_rdebug_create`](../../../include/flashvm/rdebug.h#L81) | Borrow context, dispatcher and service; register without taking the VM hook. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdebug_destroy`](../../../include/flashvm/rdebug.h#L102) | Unregister/free the adapter after explicit hook detachment; BUSY at a stop. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdebug_snapshot_read`](../../../include/flashvm/rdebug.h#L107) | Decode counted copied event data without borrowing VM objects. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Own a bounded dispatcher; NULL hooks mean single-owner use. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Free only after users and pending command identities are quiesced. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_disconnect`](../../../include/flashvm/revents.h#L99) | Owner revokes session state and discards its queued commands/replies. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L79) | Copy one reader’s event; no other observer loses its copy. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Owner assigns explicit session-specific rights. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L139) | Copy a request; owner execution occurs later, outside the queue lock. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L63) | Create an independent queue requiring SUBSCRIBE authority. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L156) | Acknowledge/copy a completed reply and free its bounded slot. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L74) | Allocate the runtime; caller destroys it after its contexts. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L78) | Release the runtime after context and adapter cleanup. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_sinteger_write`](../../../include/flashvm/srecord.h#L62) | Encode the expected stop as a canonical unsigned S integer into caller storage. [Contract](../../../doc/SRECORD.md). |

[Source](main.c), [guide](../../../doc/RESOURCE_DEBUGGER.md), [catalog](../../README.md#example-debugger-resource_debugger).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../../../src/capi/channel_c.c#L30) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:61](../../../include/flashvm/debug.h#L61) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:66](../../../include/flashvm/debug.h#L66) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:62](../../../include/flashvm/function.h#L62) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../../../include/flashvm/function.h#L79) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_rdebug_create` | function | [include/flashvm/rdebug.h:81](../../../include/flashvm/rdebug.h#L81) |
| `fvm_rdebug_destroy` | function | [include/flashvm/rdebug.h:102](../../../include/flashvm/rdebug.h#L102) |
| `fvm_rdebug_snapshot_read` | function | [include/flashvm/rdebug.h:107](../../../include/flashvm/rdebug.h#L107) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:99](../../../include/flashvm/revents.h#L99) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:79](../../../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:139](../../../include/flashvm/rdispatch.h#L139) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:63](../../../include/flashvm/revents.h#L63) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:156](../../../include/flashvm/rdispatch.h#L156) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:62](../../../include/flashvm/srecord.h#L62) |
<!-- source-links:end -->
