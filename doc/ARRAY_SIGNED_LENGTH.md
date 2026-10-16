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
