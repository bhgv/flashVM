# Borrowed negative-length splice — v0.373

Loaded SWF 4–8 borrowed `Array.splice` accepts an own length wrapping to a
negative Int32. A nonnegative start selects that signed length; removal is empty,
insertions use signed own keys and the final raw length is captured length plus
insertion count. Crossing zero is supported. Start, own length and delete count
are converted once in order. Later conversion mutations do not change bounds.
Negative starts without signed addition overflow, negative delete counts and an
explicit undefined delete count produce undefined. Missing delete count removes
zero entries. Unrelated tail properties remain intact. Raw stores use existing
property flags and bypass getters, setters and watches; insertion work is budgeted.
The removed array is temporarily rooted. Allocation/budget failure need not undo
previous insertions, consistently with existing generic splice.

Native Array negative storage and signed-addition overflow remain unsupported;
this does not close stage 1. Comparison source:
[Ruffle array implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Tests: `array_legacy_splice_negative`, `array_legacy_splice_length` cover signed
keys, zero crossing, captured bounds despite conversion side effects, omitted/
undefined/negative deletion, retained tails, readonly flags, budget exhaustion,
native negative rejection and root/depth restoration.
`array_legacy_splice_negative_allocation` injects every allocation boundary until
success, collects after failure, retries partial mutations and verifies retained strings.

Validation v0.373: 769 AGG / 769 Cairo / 750 minimal Release checks pass after
private-header rebuild and ELF permission repairs/reruns; five restricted
heap/global ASan+UBSan splice checks pass (stack ASan/LSan disabled).


## v0.491 — native negative-length splice / Native Array

Загруженные native Array с отрицательной Int32-длиной теперь поддерживают
splice по тем же captured bounds, что и заимствованные объекты. Вставки пишутся
в знаковые ключи, native length растёт перед проверкой readonly данных, итоговая
длина задаётся с учётом readonly/nondelete правил native Array. Удалённый массив
пуст и удерживается корнем до завершения операции. Getter/setter/watch не
вызываются для raw данных, в том числе для скрытых версионными битами descriptor.

Negative prepared bounds now force property-aware splice even if the current
native resource length is a high unsigned Number, or delete-count coercion has
changed it to a positive length. This fixes a fast-path mismatch: the old fast
path could ignore captured negative_length and insert at index zero instead of
a signed key. The common signed raw setter observes current length for tail
deletion, while insertion positions and final target length use captured bounds.
A callback that changes length to 2 after capturing -3 therefore permits final
shrink to -1 to delete current indices 0 and 1, except nondeletable entries.

Tests: array_native_negative_splice covers SWF4–8, insertion counts 0–4,
signed-constructor/high-unsigned raw storage, readonly length, C/direct invoke/
interpreter/explicit/automatic native JIT, collection and graph restoration.
array_native_negative_splice_coercion checks conversion side effects, negative/
undefined/omitted arguments, nondelete tails, signed-addition rejection and
budget errors/retry. array_native_negative_splice_allocation checks each new/
new[] boundary until success, temporary root cleanup, counted NUL strings and
retry after partial mutation. array_native_negative_virtual now includes splice
for callback bypass, readonly/nondelete descriptor data and version flags.
The native splice regression fails against the v0.490 library; the previous
borrowed fixture now checks successful native operation and delete conversion.

No public ABI or wire-layout change. Signed overflow, ordinary negative-index growth, arbitrary/virtual native
length assignment, original-player comparison, foreign-platform execution and
all nine roadmap stages remain open. Mutation is not transactional after an
allocation/budget error; completed earlier writes may remain.
