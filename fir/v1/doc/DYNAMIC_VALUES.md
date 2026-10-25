# Dynamic values: implemented status and proposed Limbo boundary

[Coverage](COVERAGE.md) · [FIR types](FORMAT.md) ·
[Dis target](DIS_BACKEND.md) · [Русский](#по-русски)

| Layer | Current behavior |
| --- | --- |
| FIR | T_ANY exists for dynamic as.add and channel receive; supported SWF operations retain Flash semantics |
| Numeric source adapters | Bounded numeric profiles, not a general dynamic AS or Limbo type system |
| Our direct Dis backend | Rejects any/dynamic coercion before emission; supports explicitly typed i32/bool/f64 |
| Inferno compiler Tany | Internal polymorphic compatibility type; not a Flash dynamic-value container |
| Our Limbo source compiler | No Flash-style any syntax, constructors, operations or tagged-value lowering yet |

## Proposed representation, not a shipped ABI

A Flash-compatible value should be an explicit discriminated value. For Limbo,
`ref Value` with an ADT `pick` is the natural source representation; a native C
adapter may use tag + union with explicit owner/mark/release handling. Suggested
cases are Undefined, Null, Bool, Int, Uint, Number, String and ObjectRef. Undefined
and Null must remain distinct; Number must preserve binary64 NaN/Inf/signed zero
where the selected Flash semantics require them. Source conversions remain
explicit checked operations, not blind C casts.

The existing broad Limbo syntax frontend recognizes ADT/pick syntax. That is not
semantic lowering of those ADTs through our numeric FIR compiler. Constructors,
case refinement, layout, descriptor maps, mark/release behavior and failure paths
still need implementation and acceptance before calling this usable any support.

A native union alone is insufficient for GC: pointer/scalar alternatives must
have correct active-case ownership and tracing. A conservative map must never
interpret a Number's payload as a heap pointer. ADT case layouts or explicitly
boxed owners need matching descriptors and actual collector tests.

Across DisVM and FlashVM, pass copied scalars/strings and checked object
capabilities through typed channels. A proposed ObjectRef carries its owning VM,
identity and generation; it never transfers a raw pointer into the other heap.
Retain/release, stale-handle errors and owner shutdown need defined operations.
Serialized tag/payload encoding must be versioned and independent of C union
padding, native pointer width and endianness. This paragraph proposes requirements;
it does not define or advertise an existing bridge wire format.

## По-русски

Flash-подобный any в нашем Limbo пока не реализован. FIR T_ANY и внутренний
Inferno Tany — разные вещи; прямой Dis backend отклоняет динамические операции.
Разумное представление — размеченное объединение, в Limbo ADT с pick; в C возможен
tag+union с явными правилами GC и владения. Одной union недостаточно: real-биты
не должны попадать в карту указателей, а строка/объект требуют корректного retain
и освобождения. Между кучами передаются значения и проверяемые идентификаторы
объектов, не общие указатели. Варианты, преобразования и формат обмена выше —
проектные требования, не уже существующий ABI или готовая поддержка языка.
