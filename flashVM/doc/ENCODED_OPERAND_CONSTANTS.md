# Encoded widths and framing / Ширины полей и заголовки

v0.248 replaces remaining numeric framing constants in AVM1 integer readers,
double payload decoding, relative branches and the top-level SWF action scanner
with documented uppercase macros. `src/avm1/operand_constants.h` describes byte/
word/double widths and signed branch encoding. `src/swf/record_constants.h`
describes the movie prefix, packed RECT width, frame fields and short/long tags.
Existing tag IDs/masks remain in `format_constants.h`.

Distinct AVM1/SWF meanings retain their own namespaces even where values match.
Ordinary array byte indices, traversal increments and rounding subtraction remain
arithmetic as required by project guidance. This is a source-readability change;
no action value, accepted file layout, public ABI or runtime policy is changed.
No third-party source or independent raw-byte/oracle fixture was edited.

Оставшиеся числа формата в чтении операндов, double, переходах и сканировании
SWF заменены смысловыми макросами в отдельных заголовках с комментариями.
Константы AVM1 и SWF разделены по назначению. Формат данных и поведение не
меняются; индексы обхода остаются обычной арифметикой. Полная приёмка стадий,
JIT и других ОС остаётся открытой.

Evidence: 454 tests pass per Linux AGG/Cairo profile; five AGG execute-bit launch
failures pass after permission recovery. Seven focused heap/global ASan+UBSan
checks and 23 unchanged original SWF traces per graphics profile pass. Stack/leak
instrumentation and other OS execution remain unavailable in this environment.
