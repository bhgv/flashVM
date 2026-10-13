# Результаты нативных вызовов v0.76 / Native dispatch results

Проверки обнаружили пропуски snapshot после native callback в Function.invoke,
прямом fvm_call, registered stack functions, callable objects и Lua-style callbacks.
Возвращённый string view мог зависеть от userdata-owner, уже недостижимого после
возврата. Внутри native barriers и function/capture root frames теперь выполняется
общая нормализация результата. Counted NUL/WTF-8 bytes сохраняются, context-owned
buffers переиспользуются без дополнительной копии; foreign buffers копируются
атомарно. Ошибки snapshot allocation возвращают OUT_OF_MEMORY без orphan index.

Для stack/Lua callback нормализуется только единственный выбранный результат.
Lua callback сначала выбирает zero/one result по count; normal callback должен
оставить zero/one value. Невалидное число результатов отклоняется прежним правилом.
Обычный fvm_stack_push копирует FVM_Value, а не внешний buffer: host string view
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
count processing; ordinary stack_push still copies a borrowed FVM_Value rather
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
