# Знаковая длина Array / Signed Array length — v0.489

RU: В загруженных SWF4–8 `new Array(number)` и `Array(number)` используют
Int32 clamp: число внутри [-2147483648, 2147483647] усекается к нулю,
NaN, бесконечности и числа вне этого диапазона дают -2147483648.
Проверка диапазона предшествует усечению: 2147483647.5 также даёт минимум.
Отрицательный ноль нормализуется в целочисленный ноль. Единственный аргумент
другого типа остаётся элементом массива; это изменение касается числовой ветки.

EN: Native storage now retains negative integer length directly, rather than
casting it to an unsigned allocation/traversal extent. The length getter exposes
that number. Loaded positive indexed writes can grow a negative length; readonly
length stays protected while indexed own data can still be stored. Nonnegative
length assignment can restore ordinary storage. Existing unloaded constructor
validation and raw unsigned length-assignment behavior are retained.

Graph encoding validates integral native length in [-2147483648, 4294967295].
Decoding accepts the same extended range, including without installed builtins;
cycles, native accessors and signed length survive collection and restoration.
The wire layout and public C89 ABI did not change. Pre-v0.489 readers reject
negative native Array state. An unloaded destination can read negative length
and recover through a nonnegative assignment; this does not promise legacy
loaded semantics for unloaded mutating methods.

`array_signed_constructor` covers C construction/function calls and interpreted,
explicit-JIT and automatic-JIT new/function paths in host and SWF4–8, signed
boundaries, fractional values, NaN/infinities, GC and cyclic graph restoration.
It fails against the pre-change library. `array_signed_constructor_allocation`
checks new/new[] failure boundaries, unpublished constructor results, successful
retry and transactional decoder rollback. `array_signed_storage` checks raw
length visibility, empty traversal methods, positive index growth, readonly
length, host restoration/recovery and malformed private encoder payloads.

The older large-sort fixture now sets a raw unsigned length explicitly, keeping
its distinct high-unsigned mutation restriction covered. Native negative end
operations and splice are still explicitly unsupported at this checkpoint;
arbitrary/virtual length assignment and signed-overflow behavior remain open.
All nine roadmap stages remain open.

Сравнение выполнено по исходникам Ruffle, не по новому запуску Flash Player:
[Array constructor/function](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs),
[Clamp](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/clamp.rs).
The inspected clamp uses ordered range comparisons before the integer cast.
Original-player and platform acceptance remain open.


## v0.490 — negative native Array end operations / Native Array

Загруженные native Array с отрицательной Int32-длиной поддерживают
push/pop/shift/unshift. Push пишет знаковые ключи, увеличивая native length
перед сохранением данных. Pop удаляет ключ length-1 и уменьшает длину;
shift возвращает raw index 0 и уменьшает длину без перемещения индексов.
Unshift пишет с индекса 0, затем задаёт captured length + argc: промежуточно
увеличившаяся длина может привести к удалению только что записанных индексов.
Readonly длина и readonly/nondelete данные сохраняют отдельные правила.

A common signed raw length setter truncates native nonnegative tails before
applying readonly protection to the length data itself. Raw signed element
stores grow native length before checking the destination's readonly flag.
Version flags clear only on successful writable replacements; protected data
keeps them. Getter/setter/watch callbacks are bypassed, including hidden virtual
properties. High unsigned raw lengths are interpreted through Int32 and can
normalize to signed storage. Borrowed-object end-operation rules are retained.

New coverage: array_native_negative_ends (C call, direct invocation, interpreter,
explicit/automatic native JIT, flags, GC and restored graphs),
array_native_negative_edges (zero crossing, argc 0–4, readonly/nondelete slots,
raw unsigned input, budget errors/retry and Int32-underflow rejection),
array_native_negative_virtual (retained descriptor data and callback bypass),
array_native_negative_allocation (new/new[] boundaries, counted NUL strings,
collection and retry after partial mutation). The direct regression fails
against the v0.489 library. Existing array_negative_ends now covers native
normalization instead of the removed blanket rejection.

Int32-minimum pop/native shift still return UNSUPPORTED before mutation; signed
addition overflow, negative native splice, arbitrary/virtual length assignment
and complete platform/player compatibility remain open. Failed operations may
retain completed earlier mutations. Public ABI and wire layout are unchanged.
All nine roadmap stages remain open; only the native host JIT runs these cases.
