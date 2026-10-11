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
Registered functions can be called by `fvm_call`, `fvm_stack_call` and AVM1
ActionCallFunction. Native FVM_NativeFunction registrations and script
functions take precedence over stack registrations with the same name.
The registration entry is copied before invoking user code, so a callback
can replace its own registration without invalidating its current call.

At entry, the stack contains exactly the arguments, indexed from 1.
Arguments need not be removed. Push a result and return 1, or return 0 for
undefined. A successful callback's topmost value becomes the return value;
its other entries are discarded. AVM1 has one return value, so counts greater
than 1 are rejected. Negative FVM_Result values report errors; no `longjmp`
crosses C++ frames. C++ exceptions are caught at the native bridge.
Ordinary native callbacks retain the existing GC reentry guard.

## Stack operations

| FlashVM | Lua reference convention | Contract |
| --- | --- | --- |
| fvm_stack_size | lua_gettop | Entry count |
| fvm_stack_settop | lua_settop | Positive target size; negative relative index; growth fills undefined |
| fvm_stack_pushvalue | lua_pushvalue | Copy indexed entry to top |
| fvm_stack_remove | lua_remove | Remove indexed entry |
| fvm_stack_insert | lua_insert | Move top to indexed position, preserving remaining order |
| fvm_stack_replace | lua_replace | Assign top to index and pop top |
| fvm_stack_pushnumber/boolean/string | lua_push* | Push primitives; pushstring copies into context storage |
| fvm_stack_checknumber/string | luaL_check* | Strict type check; return error rather than raising |
| fvm_stack_newtable | lua_newtable | Create a traced VM object and push it |
| fvm_stack_getfield/setfield | lua_getfield/setfield | Object property access; setter pops only on success |
| fvm_stack_getglobal/setglobal | lua_getglobal/setglobal | Global access; setter pops only on success |
| fvm_stack_context | lua_State association | Retrieve owning context |

Indices are positive from 1, negative from -1 at the top. Zero and indices
outside the current stack are invalid. `settop(-1)` keeps the current stack,
`settop(-2)` removes one item. No pseudoindices or implicit numeric/string
coercion are provided. The old FVM_StackFunction interface remains compatible:
it returns FVM_Result and must leave zero or one entry, with arguments removed.

## Ownership and outstanding work

A stack must be destroyed before its context and must not be shared between
threads. All its object entries are traced. Strings pushed through pushstring
are copied; the lower-level fvm_stack_push retains its borrowed-string contract.
Strings are context-lifetime allocations, not individually collected.
Returned objects need a traced VM reference or explicit root across later GC.

Lua-style module registration, pseudoindices, C closure upvalues, binary
strings, registry, multiple host return values, protected error objects and
a general callable-property dispatch remain outstanding. None are implied by
this first Lua-inspired bridge.

Validation: `tests/auxlib.c` covers stack manipulation, relative/INT_MIN
indices, type errors, copied strings, GC retention, callback result counts,
C invocation and actual AVM1 invocation. Existing stack tests cover the old API.
