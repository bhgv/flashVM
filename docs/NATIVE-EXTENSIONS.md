# Native extensions: FlashVM and DisVM

Both engines already expose C extension interfaces. Reuse the owning engine's
interface and retain separate heaps; a C module is not a license to pass VM pointers
through the Dis/Flash channel boundary.

| Integration | Interface and runnable starting point |
| --- | --- |
| FlashVM named native function | `fvm_register_function_rooted`; [extending](../flashVM/examples/extending/README.md) |
| FlashVM namespace methods/accessors | `fvm_object_define_method_rooted`, `fvm_object_define_accessor_rooted`; [separately compiled module](../flashVM/examples/embedding/native_module/README.md) |
| FlashVM stack convenience interface | [Lua-style API](../flashVM/doc/LUA_STYLE_API.md) and [example](../flashVM/examples/extending/lua_style/README.md) |
| Limbo/Dis C builtin module | Typed `.m` declarations, generated checked C bindings; [contract](../disVM/runtime/NATIVE_MODULES.md), [example](../disVM/examples/native-module/README.md) |
| IDE drawing extension | Generic rooted property adapter in `disVM/runtime/desktop_drawing_capability.c`; [AS/Limbo composition](../ide/dis/DESKTOP-CONTROLS.md) |

For FlashVM, keep host userdata and callback machine code alive until all contexts
that can retain callbacks are destroyed. Rooted callback captures trace managed
objects; a native `void *` does not. Temporarily pin an unpublished namespace until
it becomes reachable from a retained VM object. Validate argument type/count/range
before side effects; return an explicit `FVM_Result` and preserve output on failure
where the declared factory contract requires it. The example actively tests GC,
foreign-context refusal, arithmetic validation and real compiled-SWF setter calls.

These examples statically link modules or let an embedding host manage library
loading; they provide no automatic `.so`/`.dll` discovery/unload protocol. Removing
a global property does not prove that callbacks are unreachable. Calls using VM
handles stay on that context's owning thread. Independent native jobs can process
copied scalar/immutable data and report results through the engine's explicit
queue/channel interfaces; they do not concurrently mutate the same VM context.

For Dis, use the compiler-generated frame/signature/root-map contract for the
selected profile. Classic32/NC64 examples are not interchangeable with experimental
Dis64 Frame64/map8 declarations. The agreed internal typed variadic-tail design
has not enabled variadic execution or changed file admission. Keep builtin and
JIT-generated C/native code distinctions explicit: linking a C callback is not
proof that a script was JIT compiled.


Native Dis shared ownership: [source-slot protocol](../disVM/runtime/OWNER-SLOTS.md),
[public header](../disVM/include/disvm/owner_slot.h), and
[commented helpers](../disVM/examples/native-owner-slot/README.md).
