# MovieClip constants / Смысловые константы MovieClip — v0.273

Documented uppercase macros now identify timeline frame limits, authored
placement depth, per-frame action bytes, per-call ticks, drawing resource bounds,
coordinate bounds, twip/pixel conversion and rotation units. Equal numeric values
with different meanings retain different macro names.

GetProperty/SetProperty native dispatch uses the existing semantic property-index
header. Signed twip clamping reuses core Int32 Number bounds. Named display-service
root slots distinguish original target/index/value from resolved clip and converted
values; index coercion reads its retained snapshot. The lower index bound names
the existing negative-fraction truncation rule.

Native MovieClip constructor/accessor/method callback IDs now have documented
transport macros. Their values and method order are unchanged; existing serialized
graphs retain their identities. The API, resource bounds, geometry behavior and
wire format are unchanged. This is a readability/root-layout checkpoint, not new
display-property or platform support.

В `.h` вынесены пределы кадров, глубин размещения, блоков действий, tick-вызовов,
команд рисования и координат, единицы twips/пикселей и углов. Индексы свойств и
корней обозначены макросами; разные смыслы не объединяются из-за одинакового
числа. Стабильные ID callback сериализации сохранены без перенумерации. Остальные
числовые литералы проверяются постепенно; полная совместимость и стадии роудмапа
ещё не завершены.

Validation: Linux GCC13.3 AGG/Cairo each pass 523 tests. Three AGG execute-bit
failures pass after restoration/rerun. Seven selected ASan/UBSan display, callback,
allocator and MovieClip transport fixtures pass (heap/global only; no stack/leak
checks). All 26 accepted original SWFs match exact traces in both profiles.
