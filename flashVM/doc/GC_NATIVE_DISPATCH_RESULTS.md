# Результаты нативных вызовов v0.76 / Native dispatch results

Проверки обнаружили пропуски snapshot после native callback в Function.invoke,
прямом [fvm_call](../include/flashvm/value.h#L138), registered stack functions, callable objects и Lua-style callbacks.
Возвращённый string view мог зависеть от userdata-owner, уже недостижимого после
возврата. Внутри native barriers и function/capture root frames теперь выполняется
общая нормализация результата. Counted NUL/WTF-8 bytes сохраняются, context-owned
buffers переиспользуются без дополнительной копии; foreign buffers копируются
атомарно. Ошибки snapshot allocation возвращают OUT_OF_MEMORY без orphan index.

Для stack/Lua callback нормализуется только единственный выбранный результат.
Lua callback сначала выбирает zero/one result по count; normal callback должен
оставить zero/one value. Невалидное число результатов отклоняется прежним правилом.
Обычный [fvm_stack_push](../include/flashvm/stack.h#L29) копирует [FVM_Value](../include/flashvm/value.h#L24), а не внешний buffer: host string view
остаётся borrowed и должен жить, пока используется stack. Snapshot относится к
успешной границе возвращения native callback, не ко всем pushes.

Общий native-result handler теперь **сначала** проверяет enum representation,
same-context object и primitive-metatable ownership для всех успешных типов,
затем читает тип и копирует строку. Invalid enum не читается как C++ enum до
проверки. Null string/foreign object/foreign primitive metadata/owned view beyond
buffer отклоняются с INVALID_ARGUMENT. Callback error status не меняется; output
при ошибке нельзя использовать. Конструкторы, где результат обязан быть object,
и ignored metamethod results сохраняют собственные правила.

gc_native_dispatch_result проверяет пять dispatch paths и обе string policies,
callback replacement, освобождение capture с poison и удержание return string.
gc_native_dispatch_allocation перебирает new failures во всех пяти paths, учитывает
snapshot indexes/bytes и teardown balance. gc_native_result_validation проверяет
6 invalid return forms через 6 bridges, включая object getter/index и primitive
metamethod. Existing native-result/getter/GC/script tests проверяют совместимость.

Это завершает описанные return paths, но не полную приёмку всех native extensions,
foreign buffer contracts, длительных concurrent fault cases и debugger/stage 7.

## English

Function invocation, direct global native calls, registered stack callbacks,
callable objects and Lua-style callbacks now normalize successful return values
inside native barriers and capture/function root lifetimes. Foreign counted
strings are atomically copied into context storage; existing context buffers are
reused and embedded NUL/WTF-8 counts are preserved. Snapshot OOM leaves ownership
indexes consistent. Stack/Lua dispatch normalizes the one selected result after
count processing; ordinary stack_push still copies a borrowed [FVM_Value](../include/flashvm/value.h#L24) rather
than arbitrary external string storage.

All successful native value types are validated before reading their enum tag:
objects and primitive metatables must belong to the current context. Invalid enum
representations, null strings, foreign ownership and context-buffer views exceeding
allocation length return INVALID_ARGUMENT. Failed callback statuses are preserved
and outputs on errors must not be consumed. Constructors and ignored hook results
keep their separate contracts.

Regressions cover five bridges/two policies, registration replacement, capture
poison/release, rooted returned strings, every allocation failure, ownership indexes
and teardown balance. Thirty-six invalid return cases cover six bridges and six
malformed value forms, including getters/index/value hooks. Full native extension,
concurrent failure and debugger acceptance remains open.

Проверки v0.76 / verification: full headless 260/260, AGG 276/276, Cairo
276/276, portable 251/251; targeted ASan/UBSan 77/77. All five bridges
include failure injection and both-policy poison tests; invalid enum cases
run under UBSan. Fonts execute in AGG/Cairo, TCP excluded from portable.
Heap/global ASan and UBSan active; leak detection/stack ASan disabled.
Linux profiles only; complete stage 7 remains open.

## v0.457 — snapshot direct global-call arguments

[fvm_call_bytes](../include/flashvm/value.h#L98) copies arguments into a rooted vector before clearing the output
or resolving a global Function. A result may alias an argument, and callbacks
may mutate the caller's value array without changing the active invocation.
Caller-owned external string buffers must still remain valid for the call.
Registered native callbacks retain their normal GC barrier.
global_call_arguments first failed after caller mutation; host/SWF4–8 check
native/script/getter/resolver routes, counted NUL names and traced strings,
cyclic unpinned arguments, aliased outputs, caller mutation, collecting callbacks,
plain errors/exceptions and root/depth cleanup/reclamation. Host skips __resolve.
This does not establish complete C embedding or concurrent-context safety.

Validation v0.457: complete Linux minimal 899/899; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.458 — replace native registrations without stale capture roots

Native registration updates retain the selected existing spelling in SWF4–6.
Publication removes a cached wrapper only when callback, userdata, capture or
collector-entry policy changed. An identical registration keeps Function identity;
an explicitly saved old Function retains its original callback/capture. Stale
cache entries no longer keep an otherwise unreachable old capture alive.
The stored name is copied before publication, so allocation failure preserves
the previous registration and cache. Host/SWF7–8 alternate names remain distinct.
global_native_replacement reproduced the stale root and covers versioned updates,
real GC, cycles, saved old Functions, new callbacks and identical registration.
global_native_replacement_allocation sweeps new/new[] boundaries through first
successful replacement, checks old identity/root retention on OOM and retry/GC.
This is not a full allocator/concurrent registration proof; all stages remain open.

Validation v0.458: complete Linux minimal 901/901; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.459 — wrapper identity includes collector-entry policy

Cached native Function reuse compares collector_entry as well as callback,
userdata and capture. New reads after an internal policy change get a wrapper
with the current policy; saved Functions retain their original policy. Public
native callbacks keep the normal collection barrier. native_wrapper_policy first
failed on stale cache reuse and checks host/SWF4–8, global/method wrappers,
ordinary-to-trusted-to-ordinary changes, saved Function invocation, actual GC
from the trusted fixture and clean roots/depth. The internal fixture does not
expose a public barrier opt-out or establish full concurrent collector safety.

Validation v0.459: complete Linux minimal 902/902; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.460 — root named-constructor arguments before global lookup

[fvm_construct_bytes](../include/flashvm/value.h#L155) snapshots and roots caller arguments before resolving a
constructor name. Global getters/resolvers can collect or mutate the caller's
array before Function construction; registered C factories also receive the
stable snapshot. The ordinary native barrier remains. Constructor output is
still an Object pointer; no overlapping output-pointer contract is added.
global_construct_arguments first failed after a native factory mutated the
caller array. Host/SWF4–8 cover native/script/getter/resolver factories, counted
NUL names/strings, cyclic unpinned arguments, mutation, collecting lookup/body,
object results, plain errors/exceptions and root cleanup/reclamation. Host skips
__resolve. Complete constructor/allocator/concurrency compatibility remains open.

Validation v0.460: complete Linux minimal 903/903 and AGG 922/922; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.462 — direct Function construction snapshots arguments

[fvm_function_construct](../include/flashvm/function.h#L69) retains an unpinned input Function and snapshots/roots
arguments before native factory dispatch or script prototype access. A prototype
getter may collect or mutate the caller's array before constructor invocation.
External borrowed string buffers remain caller-owned for the complete call;
ordinary native callbacks retain their GC barrier. No overlapping output-pointer
contract is introduced. function_construct_arguments first failed after native
factory mutation and checks host/SWF4–8, direct script/native construction,
collecting prototype getters, traced NUL strings, cyclic arguments, mutation,
object returns, errors/exceptions and root/depth cleanup/reclamation. The native
constructable entry is an internal fixture for built-in factory dispatch.
Full constructor/allocator/concurrency compatibility and all stages remain open.

Validation v0.462: complete Linux minimal 906/906; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.463 — validate direct native factory results

Direct Function construction initializes the native factory result to undefined
and publishes only a non-null Object belonging to the calling context. Untouched,
scalar, null-Object and foreign-context results produce ERROR with a null output;
actual callback errors/exceptions remain unchanged. This matches the existing
named-factory object ownership boundary. function_construct_native_result first
failed on an invalid Object and checks host/SWF4–8, valid/invalid output shapes,
untouched output, callback errors, retained exception payloads and GC/root cleanup.
This validates well-formed API pointers, not arbitrary invalid host addresses.
All constructor, allocator, concurrency and foreign OS acceptance remains open.

Validation v0.463: complete Linux minimal 907/907; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 3/3 targeted checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.464 — invoke the retained Function argument snapshot

Direct [fvm_function_invoke](../include/flashvm/function.h#L65) now passes its retained copy to the callback, rather
than the caller's mutable array. The output is cleared after copying, so it may
alias any argument. Validation/OOM errors still clear the output; external string
buffers remain caller-owned. Existing single-frame root layout and native barrier
policy are retained. function_invoke_arguments first failed after callback mutation
and checks host/SWF4–8, native/script/trusted-internal callbacks, implicit global
or explicit unpinned receiver, both aliased argument slots, cyclic inputs, traced
NUL strings, actual GC where permitted, retained successful strings, errors,
pending cyclic exception payloads and post-call reclamation/root cleanup.
Full C embedding, allocator and concurrent collector acceptance remains open.

Validation v0.464: complete Linux minimal 908/908; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 4/4 targeted checks each. The duplicate
error-output clear was removed afterwards and the invocation test reran successfully.
Stack ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `fvm_call_bytes` | function | [include/flashvm/value.h:98](../include/flashvm/value.h#L98) |
| `fvm_construct_bytes` | function | [include/flashvm/value.h:155](../include/flashvm/value.h#L155) |
| `fvm_function_construct` | function | [include/flashvm/function.h:69](../include/flashvm/function.h#L69) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `fvm_stack_push` | function | [include/flashvm/stack.h:29](../include/flashvm/stack.h#L29) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
