# Signed Array property indices / Знаковые индексы Array — v0.494

Loaded SWF4–8 native Arrays interpret decimal property names as wrapping Int32
for length growth. Leading ASCII whitespace, one optional sign and leading zeroes
are accepted; every remaining character must be an ASCII decimal digit. Arbitrarily
long names wrap modulo 2^32. Trailing whitespace, decimal points, non-ASCII
whitespace and embedded NUL do not form an index. The property spelling remains
literal: `01` and `+1` do not become the stored element `1`.

Growth follows the watcher and precedes own readonly checks and own setters.
Inherited virtual dispatch consumes the write without growing length. Incrementing
index 2147483647 wraps length to -2147483648 and deletes the old nonnegative
range before attempting length storage. Readonly length blocks replacement but
still allows that deletion; DONT_DELETE slots survive. Work limits and worker
stop polling apply to deletion, with errors propagated before the new slot is
written. Deletions completed before an error remain visible; this is not an
atomic operation. Actual growth replaces the old numeric metatable.

В SWF4–8 имена разбираются со знаком и переполнением Int32, без изменения
буквального ключа. Watcher выполняется до роста длины; собственный setter и
readonly проверяются после него. Переполнение положительного индекса удаляет
старые элементы с учётом DONT_DELETE и лимита операций, даже при readonly
length. Удаление до ошибки может частично изменить массив. Незагруженный
host API сохраняет прежнюю политику Uint32.

Tests: `array_named_index` covers SWF4–8, C API, interpreter and explicit/automatic
JIT, aliases, wrapping, invalid names and protected overflow. `array_named_index_effects`
covers watchers, own/inherited virtuals, counted NUL, non-ASCII whitespace,
budget interruption/retry and native-root cleanup. Existing `array_legacy_index_length`
continues to cover readonly slots, setters and graph transfer.

Reference: Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object/script_object.rs` (`parse_array_index`, `set_local`) and
`wstr/src/parse.rs` (`Wrapping<i32>` parsing). This is source comparison, not
an original Flash Player runtime oracle. Arbitrary nonnumeric/virtual length and
inherited-data versus deeper virtual-property precedence remain open. No roadmap
stage or foreign-platform runtime acceptance is closed by this change.

Validation: Linux Release minimal 990/990 and AGG 1009/1009 after restoring
execute permissions on affected generated binaries; four targeted restricted
ASan/UBSan tests pass. Final counted-key graph roundtrips pass in all three
profiles. Unicorn checks pass for all six JIT targets, including branch/error,
far-branch and applicable high-address/Thumb variants. These emulator checks
do not establish Windows/macOS/Android runtime acceptance.
