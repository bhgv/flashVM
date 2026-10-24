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
example does not open sockets or demonstrate a [FVM_Channel](../../../src/capi/channel_c.c#L27) graph bridge, workers,
remote authentication or 9P.

| API | Purpose and ownership |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize the owned configuration before runtime creation. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Allocate the VM context used only by its owner thread. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Release managed state after callback detachment and unpin. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_debug_set_breakpoint`](../../../include/flashvm/debug.h#L51) | Owner sets an exact bytecode breakpoint; original interpreter fallback applies. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_debug_set_breakpoint_handler`](../../../include/flashvm/debug.h#L54) | Install/clear the borrowed callback explicitly; clear before freeing userdata. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L52) | Copy AVM1 bytecode into a managed function, with caller pinning across setup. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_function_invoke`](../../../include/flashvm/function.h#L65) | Execute on the owner; result 42 proves progress after resume. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | Retain the managed function across host allocations and calls. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | Release exactly the host root previously acquired. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_rdebug_create`](../../../include/flashvm/rdebug.h#L73) | Borrow context, dispatcher and service; register without taking the VM hook. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdebug_destroy`](../../../include/flashvm/rdebug.h#L88) | Unregister/free the adapter after explicit hook detachment; BUSY at a stop. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdebug_snapshot_read`](../../../include/flashvm/rdebug.h#L91) | Decode counted copied event data without borrowing VM objects. [Contract](../../../doc/RESOURCE_DEBUGGER.md). |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L95) | Own a bounded dispatcher; NULL hooks mean single-owner use. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L131) | Free only after users and pending command identities are quiesced. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_disconnect`](../../../include/flashvm/revents.h#L79) | Owner revokes session state and discards its queued commands/replies. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L65) | Copy one reader’s event; no other observer loses its copy. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L108) | Owner assigns explicit session-specific rights. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L115) | Copy a request; owner execution occurs later, outside the queue lock. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L53) | Create an independent queue requiring SUBSCRIBE authority. [Contract](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L126) | Acknowledge/copy a completed reply and free its bounded slot. [Contract](../../../doc/RESOURCE_DISPATCH.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Allocate the runtime; caller destroys it after its contexts. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the runtime after context and adapter cleanup. [Contract](../../../doc/API_INDEX.md). |
| [`fvm_sinteger_write`](../../../include/flashvm/srecord.h#L56) | Encode the expected stop as a canonical unsigned S integer into caller storage. [Contract](../../../doc/SRECORD.md). |

[Source](main.c), [guide](../../../doc/RESOURCE_DEBUGGER.md), [catalog](../../README.md#example-debugger-resource_debugger).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:27](../../../src/capi/channel_c.c#L27) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:51](../../../include/flashvm/debug.h#L51) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../../../include/flashvm/debug.h#L54) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:52](../../../include/flashvm/function.h#L52) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../../../include/flashvm/function.h#L65) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_rdebug_create` | function | [include/flashvm/rdebug.h:73](../../../include/flashvm/rdebug.h#L73) |
| `fvm_rdebug_destroy` | function | [include/flashvm/rdebug.h:88](../../../include/flashvm/rdebug.h#L88) |
| `fvm_rdebug_snapshot_read` | function | [include/flashvm/rdebug.h:91](../../../include/flashvm/rdebug.h#L91) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../../../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../../../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:79](../../../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:65](../../../include/flashvm/revents.h#L65) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../../../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:53](../../../include/flashvm/revents.h#L53) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:126](../../../include/flashvm/rdispatch.h#L126) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:56](../../../include/flashvm/srecord.h#L56) |
<!-- source-links:end -->
