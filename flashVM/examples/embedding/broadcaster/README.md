# Application event object (C89 and AS2)

[Русский](README.ru.md). [main.c](main.c) is a runnable host without a window or
physical device; [events.as](events.as) is the script companion for a host that
explicitly installed AsBroadcaster. Every nonempty code line is explained.
The AS source is not a SWF: use a separate compatible AS2 compiler to produce
SWF6–8; this tutorial assumes no particular external compiler command.

The host creates source/listener, installs onValue, registers the object,
synchronously sends 42, snapshots its graph through a FIFO into a second context,
delivers 84 through the copy and removes the original registration. Use this for application events
above any frontend. It does not poll devices or replace Key/Mouse/Selection.

## Build and expected result

Requires a C89/C++98 toolchain, CMake and an ordinary FlashVM profile. No window
SDK, network, permission or physical device is needed. From flashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_broadcaster_main -j2
./build/flashvm_examples_embedding_broadcaster_main
```

```text
onValue: 42
onValue: 84
```

## Execution, ownership and limits

All calls are synchronous on one VM owner thread. The context owns objects;
pins protect partial setup and later explicit removal. The source's live Array
retains listener until removal/reset/unreachability. The callback saves no borrowed
self/args/out. Cleanup drops only acquired pins and destroys context before runtime;
context destruction also releases registration left after an error. Errors print
to stderr and return process code 1; success returns 0. Ordinary error handling
preserves failures without assertions. See the [guide](../../../doc/AS_BROADCASTER.md)
for snapshot ordering, mutation, the 4096 bound, version policy and open acceptance.
This is an embedding tutorial, not a player, compiler or historical conformance gate.


The second context installs its own standard objects and independently registers
receive under the same stable identity. No machine code or object addresses travel.
It needs no AsBroadcaster namespace: prepared methods and the live Array are
imported. Pin the imported root before collection; after dispatch unpin it, release
the queue and destroy the second context. This is sequential exchange between two
heaps on one owner thread, not a threading test. Separate fixtures cover files,
processes and network transport.

## Every demonstrated C API

| API | Parameters, behavior and ownership |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | config output: initialize ABI defaults; no resource acquired. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | config input, runtime output: acquire outer owner; destroy after context. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | runtime, context output: acquire a separate managed heap. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L19) | context: install Object/Array prerequisite; may allocate synchronously. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_standard_broadcaster_install`](../../../include/flashvm/broadcaster.h#L13) | context: opt-in script namespace; preserve conflicting bindings. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_broadcaster_initialize`](../../../include/flashvm/broadcaster.h#L19) | context, same-context source: atomically prepare three methods/live Array after binding validation. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | context, object output: create context-owned object; output is borrowed. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | context, object: retain construction/later native access; pair acquired pin with unpin. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | context, listener, name, callback, userdata, root: install receiver-bound handler; zero capture here. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | value output: initialize every field before assigning object identity; no allocation. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_value_string`](../../../include/flashvm/value.h#L46) | value output, static UTF-8: borrow event name for this synchronous call. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | value output, double: copy numeric payload; no managed allocation. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | context, source, method, argc/argv, out: synchronous add/broadcast/remove; borrow inputs until return. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | context, object: drop one acquired root after last native access. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | context: release heap, registrations and namespace, ending borrowed object lifetimes. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | runtime: release outer owner after context teardown. [Guide](../../../doc/AS_BROADCASTER.md#installation-and-ownership). |
| [`fvm_serialization_register_callback`](../../../include/flashvm/serialization.h#L30) | context, stable name, callback: bind local code in each heap; no native userdata is copied. [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |
| [`fvm_channel_create`](../../../include/flashvm/channel.h#L23) | channel output: acquire one unbounded FIFO handle, released after receive. [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |
| [`fvm_channel_send_value`](../../../include/flashvm/transfer.h#L22) | source context, queue, root, blocking=0: copy graph before enqueue; no source pointers retained. [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |
| [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h#L25) | destination, queue, output, blocking=0: decode one message into destination-owned objects; empty open queue returns [FVM_NOT_FOUND](../../../include/flashvm/flashvm.h#L47). [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L44) | destination, optional freed count=NULL: explicitly collect unreachable state while the imported root is pinned. [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |
| [`fvm_channel_release`](../../../include/flashvm/channel.h#L47) | queue: release acquired handle after operations stop; last owner frees pending messages. [Guide](../../../doc/AS_BROADCASTER.md#transport-and-player-v0616). |

## Callback and values

[`FVM_Config`](../../../include/flashvm/flashvm.h#L51) is host storage; [`FVM_Runtime`](../../../src/core/runtime.hpp#L175) and [`FVM_Context`](../../../src/core/runtime.hpp#L247) are acquired owners.
[`FVM_Object`](../../../src/core/runtime.hpp#L122) is borrowed and needs a pin/reachable owner. [`FVM_Value`](../../../include/flashvm/value.h#L24) has a tag:
[`FVM_VALUE_OBJECT`](../../../include/flashvm/value.h#L21) selects object and [`FVM_VALUE_NUMBER`](../../../include/flashvm/value.h#L19) selects number.
[`FVM_Result`](../../../include/flashvm/flashvm.h#L40) carries the real operation status.

For receive, context is the owning VM, self is the registered listener, count is
the payload count excluding the event name, and args borrows count values until
return. out is writable result storage, initialized here to undefined; data is
the registration's userdata pointer, NULL here. The callback is synchronous and
retains no argument or pointer beyond the call.

## Script APIs

| API | Purpose |
| --- | --- |
| AsBroadcaster.initialize(source) | Install/reset the user object's methods and live Array. |
| source.addListener(observer) | Strongly retain recipient identity; return true on success. |
| source.broadcastMessage("onValue", 42) | Invoke onValue(42) on snapshot listeners, with this=observer. |
| source.removeListener(observer) | Drop registration and return whether it was removed. |
| trace(value) | Print through the script host; the C tutorial itself uses printf. |

Contracts and Adobe source: [guide](../../../doc/AS_BROADCASTER.md#script-methods-and-delivery).

Registration may return [`FVM_ERROR`](../../../include/flashvm/flashvm.h#L43) when the shared execution budget is exhausted; the live list remains unchanged. See [work limits](../../../doc/AS_BROADCASTER.md#registration-work-limits-v0617) / [ограничение работы](../../../doc/AS_BROADCASTER.ru.md#ограничение-работы-регистрации-v0617).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broadcaster_initialize` | function | [include/flashvm/broadcaster.h:19](../../../include/flashvm/broadcaster.h#L19) |
| `fvm_channel_create` | function | [include/flashvm/channel.h:23](../../../include/flashvm/channel.h#L23) |
| `fvm_channel_receive_value` | function | [include/flashvm/transfer.h:25](../../../include/flashvm/transfer.h#L25) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:47](../../../include/flashvm/channel.h#L47) |
| `fvm_channel_send_value` | function | [include/flashvm/transfer.h:22](../../../include/flashvm/transfer.h#L22) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../../../include/flashvm/flashvm.h#L51) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `FVM_Context` | struct | [src/core/runtime.hpp:247](../../../src/core/runtime.hpp#L247) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../../../include/flashvm/flashvm.h#L43) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../../../include/flashvm/gc.h#L44) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../../../include/flashvm/flashvm.h#L47) |
| `FVM_Object` | struct | [src/core/runtime.hpp:122](../../../src/core/runtime.hpp#L122) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../../../include/flashvm/flashvm.h#L40) |
| `FVM_Runtime` | struct | [src/core/runtime.hpp:175](../../../src/core/runtime.hpp#L175) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_serialization_register_callback` | function | [include/flashvm/serialization.h:30](../../../include/flashvm/serialization.h#L30) |
| `fvm_standard_broadcaster_install` | function | [include/flashvm/broadcaster.h:13](../../../include/flashvm/broadcaster.h#L13) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../../../include/flashvm/standard.h#L19) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../../../include/flashvm/value.h#L24) |
| `FVM_VALUE_NUMBER` | enum constant | [include/flashvm/value.h:19](../../../include/flashvm/value.h#L19) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../../../include/flashvm/value.h#L21) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->
