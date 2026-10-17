# Lua-inspired C extension API

Reference model: https://www.lua.org/manual/5.1/manual.html#3
This API uses Lua's stack-index and C callback conventions, not Lua's binary
ABI, runtime semantics or full library. Public headers remain C89-compatible.
Include `flashvm/auxlib.h`. See `examples/extending/lua_style/main.c`.

## Registration and call contract

`fvm_stack_register_cfunction(context, name, callback, capture)` registers a
callback of type `int (*)(FVM_Stack *)`. The optional context-owned capture
object is a GC root until this registration is replaced. Store captured VM
values in that object's traced properties; arbitrary C memory is not scanned.
Registered functions can be called by [`fvm_call`](../include/flashvm/value.h#L138), [`fvm_stack_call`](../include/flashvm/stack.h#L50) and AVM1
ActionCallFunction. Native [FVM_NativeFunction](../include/flashvm/value.h#L37) registrations and script
functions take precedence over stack registrations with the same name.
The registration entry is copied before invoking user code, so a callback
can replace its own registration without invalidating its current call.

At entry, the stack contains exactly the arguments, indexed from 1.
Arguments need not be removed. Push a result and return 1, or return 0 for
undefined. A successful callback's topmost value becomes the return value;
its other entries are discarded. AVM1 has one return value, so counts greater
than 1 are rejected. Negative [FVM_Result](../include/flashvm/flashvm.h#L40) values report errors; no `longjmp`
crosses C++ frames. C++ exceptions are caught at the native bridge.
Ordinary native callbacks retain the existing GC reentry guard.

## Stack operations

| FlashVM | Lua reference convention | Contract |
| --- | --- | --- |
| [fvm_stack_size](../include/flashvm/stack.h#L22) | lua_gettop | Entry count |
| [fvm_stack_settop](../include/flashvm/auxlib.h#L38) | lua_settop | Positive target size; negative relative index; growth fills undefined |
| [fvm_stack_pushvalue](../include/flashvm/auxlib.h#L40) | lua_pushvalue | Copy indexed entry to top |
| [fvm_stack_remove](../include/flashvm/auxlib.h#L42) | lua_remove | Remove indexed entry |
| [fvm_stack_insert](../include/flashvm/auxlib.h#L44) | lua_insert | Move top to indexed position, preserving remaining order |
| [fvm_stack_replace](../include/flashvm/auxlib.h#L46) | lua_replace | Assign top to index and pop top |
| [fvm_stack_pushnumber](../include/flashvm/auxlib.h#L48)/boolean/string | lua_push* | Push primitives; pushstring copies into context storage |
| [fvm_stack_checknumber](../include/flashvm/auxlib.h#L57)/string | luaL_check* | Strict type check; return error rather than raising |
| [fvm_stack_newtable](../include/flashvm/auxlib.h#L61) | lua_newtable | Create a traced VM object and push it |
| [fvm_stack_getfield](../include/flashvm/auxlib.h#L63)/setfield | lua_getfield/setfield | Object property access; setter pops only on success |
| [fvm_stack_getglobal](../include/flashvm/auxlib.h#L68)/setglobal | lua_getglobal/setglobal | Global access; setter pops only on success |
| [fvm_stack_context](../include/flashvm/auxlib.h#L36) | lua_State association | Retrieve owning context |

Indices are positive from 1, negative from -1 at the top. Zero and indices
outside the current stack are invalid. `settop(-1)` keeps the current stack,
`settop(-2)` removes one item. No pseudoindices or implicit numeric/string
coercion are provided. The old [FVM_StackFunction](../include/flashvm/stack.h#L16) interface remains compatible:
it returns [FVM_Result](../include/flashvm/flashvm.h#L40) and must leave zero or one entry, with arguments removed.

## Ownership and outstanding work

A stack must be destroyed before its context and must not be shared between
threads. All its object entries are traced. Strings pushed through pushstring
are copied; the lower-level [fvm_stack_push](../include/flashvm/stack.h#L29) retains its borrowed-string contract.
Strings are context-lifetime allocations, not individually collected.
Returned objects need a traced VM reference or explicit root across later GC.

Lua-style module registration, pseudoindices, C closure upvalues, binary
strings, registry, multiple host return values, protected error objects and
a general callable-property dispatch remain outstanding. None are implied by
this first Lua-inspired bridge.

Validation: `tests/auxlib.c` covers stack manipulation, relative/INT_MIN
indices, type errors, copied strings, GC retention, callback result counts,
C invocation and actual AVM1 invocation. Existing stack tests cover the old API.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../src/capi/stack_c.cpp#L16) |
| `fvm_stack_call` | function | [include/flashvm/stack.h:50](../include/flashvm/stack.h#L50) |
| `fvm_stack_checknumber` | function | [include/flashvm/auxlib.h:57](../include/flashvm/auxlib.h#L57) |
| `fvm_stack_context` | function | [include/flashvm/auxlib.h:36](../include/flashvm/auxlib.h#L36) |
| `fvm_stack_getfield` | function | [include/flashvm/auxlib.h:63](../include/flashvm/auxlib.h#L63) |
| `fvm_stack_getglobal` | function | [include/flashvm/auxlib.h:68](../include/flashvm/auxlib.h#L68) |
| `fvm_stack_insert` | function | [include/flashvm/auxlib.h:44](../include/flashvm/auxlib.h#L44) |
| `fvm_stack_newtable` | function | [include/flashvm/auxlib.h:61](../include/flashvm/auxlib.h#L61) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:29](../include/flashvm/stack.h#L29) |
| `fvm_stack_pushnumber` | function | [include/flashvm/auxlib.h:48](../include/flashvm/auxlib.h#L48) |
| `fvm_stack_pushvalue` | function | [include/flashvm/auxlib.h:40](../include/flashvm/auxlib.h#L40) |
| `fvm_stack_register_cfunction` | function | [include/flashvm/auxlib.h:18](../include/flashvm/auxlib.h#L18) |
| `fvm_stack_remove` | function | [include/flashvm/auxlib.h:42](../include/flashvm/auxlib.h#L42) |
| `fvm_stack_replace` | function | [include/flashvm/auxlib.h:46](../include/flashvm/auxlib.h#L46) |
| `fvm_stack_settop` | function | [include/flashvm/auxlib.h:38](../include/flashvm/auxlib.h#L38) |
| `fvm_stack_size` | function | [include/flashvm/stack.h:22](../include/flashvm/stack.h#L22) |
| `FVM_StackFunction` | callback typedef | [include/flashvm/stack.h:16](../include/flashvm/stack.h#L16) |
<!-- source-links:end -->
