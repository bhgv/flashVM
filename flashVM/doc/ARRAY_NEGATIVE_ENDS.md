# Borrowed Array end operations with negative length — v0.374

Loaded SWF4–8 borrowed push/pop/shift/unshift now accept own lengths wrapping to
negative Int32. Push writes signed own keys starting at captured length, including
crossing zero, then stores the signed sum as length and returns that sum. Unshift
has no source range to move, deletes/writes indices starting at zero, returns the
signed sum and leaves borrowed length unchanged. Pop reads/deletes the raw own
key at length minus one without resizing. Shift returns raw own index zero;
there is no movement, deletion or borrowed length update for negative length.

Own data reads ignore prototypes and do not invoke getters. Existing raw-store
and delete rules retain readonly/nondeletable flags and bypass setters/watches.
Insertion work is charged. Native invocation protects values awaiting return;
allocation failures restore roots/depth but may retain partial mutations. Native
Array negative storage, signed overflow (including pop at Int32 minimum) and
complete Flash compatibility remain open. Host mode is unchanged.

Comparison source: [pinned Ruffle Array implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
`array_negative_ends` covers all five loaded versions, signed keys/zero crossing,
zero arguments, preserved borrowed lengths, readonly/nondeletable data, raw
accessors, Int32 minimum, work limits and native rejection.
`array_negative_ends_allocation` injects each allocation boundary for all four
methods, collects after failure/success and retries partial writes.

Validation v0.374: all 771 AGG / 771 Cairo / 752 minimal Linux Release checks
have passing evidence after ELF execution-permission repairs and failed-test
reruns. Five restricted heap/global ASan+UBSan checks pass (stack ASan and LSan
disabled): both new end-operation tests, legacy push/unshift and negative splice.


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
