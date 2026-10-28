# Native C modules: Dis/DisVM ownership and ABI

[Русский](NATIVE_MODULES.ru.md) · [Runnable tutorial](../examples/native-module/README.md) ·
[Our .m → C generator](../../compiler/limbo/frontend/NATIVE_C.md) ·
[Mixed UI ownership](../../ide/RENDER_OWNERS.md)

A native Dis module implements a typed Limbo `.m` interface using C callbacks.
A native call is still an ordinary resolved module call: the loader compares
export signatures, the VM supplies a mapped frame and the callback publishes a
result or raises. C does not acquire permission to edit another VM heap or bypass
DisVM → channel → Flash-content control.

## Development sequence

1. Write `.m` first, including ownership/error/blocking contracts. Keep the API
   independent of native pointers and OS struct layouts.
2. Generate frames, signatures and GC maps with an explicit target. Our
   `limbo_bindings --format native-c-nc64` supports the documented direct scalar
   profile; `native-c-classic32` is distinct. The matched bootstrap remains needed
   for complex existing interfaces not yet resolved by our native backend.
3. Implement generated `Module_export(void *frame)` callbacks. Use generated
   field/return declarations, not guessed offsets or copied legacy headers.
4. Compile the implementation into the host and register its generated table
   with `builtinmod` after allocator initialization. Handle registration failure.
5. Compile the caller with the same interface/target; test signature mismatches,
   errors, GC, boundaries and both dispatch modes before exposing it to the IDE.

The optional `$Nativedemo` example implements every stage. Enabling
`DVM_RUNTIME_NATIVE_DEMO=ON` requires an explicit `DVM_LIMBO_BINDINGS` executable.
The option defaults OFF. A `.m` file alone does not install a module, and this
example is not a platform `.so`/DLL loader. The C frame ABI is an internal host
integration boundary, not a portable serialized network format.

## Target rules

Current runnable DisVM uses **NC64**: eight-byte WORD, pointer/map slots and
native argument cells, magic 819264. Classic Dis has four-byte cells and magic
819248. Neither is silently converted. NC64 is not a declaration that every
standard Inferno int became 64-bit everywhere. Existing signatures are structural;
their hash does not replace matching frame layout, module resolution or lifetime.
The new generator checks pointer/WORD/big/real sizes, NREG and field offsets in C.
Do not reuse experimental renderer `e.h` from a different target as our ABI.

`big` is exact signed 64-bit. Implement wrap using unsigned arithmetic and publish
bits; signed overflow in C is not a Limbo wrap implementation. Use generated WORD
for int slots, LONG for big and REAL for real. Bytewise `memmove` avoids packed
boundary alignment assumptions, but it does not validate an arbitrary forged
pointer. Only the VM may supply the trusted frame used by these examples.

## GC and result ownership

A native frame/its typed arguments are borrowed for the synchronous call. Managed
arguments remain rooted through the generated map. The return slot belongs to
the caller; the return pointer itself is not an input GC object. A Dis `String *`
is not a C UTF-8 buffer; a native `malloc` pointer or stack buffer has no Heap
prefix and must never be passed to `D2H`, `destroy` or retain helpers.

Use checked existing runtime publication helpers for supported results. The example
uses `retstr` with a C literal: allocate a Dis string, publish it, retire the old
slot owner. If allocating while retaining other managed values, preserve roots
through the actual VM mechanism and protect partial construction/cleanup. A raw C
local pointer alone is not an additional root. Arrays require the correct element
descriptor/map and full length/payload validation, not only a pointer cast.

Returning private native state requires a VM-managed opaque owner, an exact
private descriptor/finalizer, validation before every use, and a defined shutdown
path. A numeric address in an ADT does not become a capability merely by its type.
See the existing Bigtable owner implementation for a more advanced hosted example;
its lease/waiter machinery is not a universal public asynchronous module API.

## Errors, blocking and workers

`error()` belongs to the bound execution owner and unwinds through Dis. A callback
must arrange C-resource cleanup before errors can bypass it. Never invoke VM error,
allocation, channel send or GC APIs from arbitrary worker threads. The tutorial's
void export raises with no resources acquired, and the Limbo client catches it.

The current runtime schedules Limbo processes above one native owner. A blocking
native disk/network call can block that owner. Prefer short native primitives and
Limbo channel services for coordination. For expensive pure work, submit copied
immutable data to native workers with separate outputs, join or post completion to
the owner, validate revision/generation and only then publish managed results.
Long-lived asynchronous exports additionally require pending-call roots,
cancellation, timeout and exception-safe teardown. None is implied by `spawn` or
by launching a pthread around a native callback.

## UI and portability

Dis may address Flash content directly through existing typed channels. Complex
visual assembly can run in AS; a C colorizer/render helper is an AS/native extension
or a checked service, not a second IDE policy engine. Every AS context/heap has an
owner. Independent render snapshots/regions may run concurrently without running
one mutable AVM1 context on several threads. See the linked rendering ownership
contract for implemented boundaries versus next steps.

Put filesystem/window/thread operations in selected OS ports. Keep compute cores
independent, input/result messages typed and caller-owned buffers explicit.
C modules are rebuilt for each host ABI. Limbo/AS bytecode portability does not
make a compiled native library portable between operating systems or architectures.
