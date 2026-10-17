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
profiles. Unicorn checks pass for x86, ARM, ARM64, RISC-V and RISC-V64, including
branch/error, far-branch and applicable high-address/Thumb variants. The x64
backend is exercised by native Linux JIT tests, not this Unicorn checker. These emulator checks
do not establish Windows/macOS/Android runtime acceptance.

## v0.517 whitespace correction / Уточнение пробелов

Leading whitespace means exactly SPACE, TAB, LF, FF and CR. Vertical tab
(U+000B) is a literal non-digit and prevents index classification, including
after an otherwise accepted prefix. Previously it incorrectly triggered native
length growth and could truncate existing elements on Int32 overflow. Literal
property storage and accessors still execute normally; inherited setters keep
their existing no-growth dispatch. Unloaded host canonical-index policy is unchanged.

Пропускаются только SPACE, TAB, LF, FF и CR. Вертикальная табуляция U+000B
остаётся частью имени и исключает его из индексов, в том числе после пробелов.
Ранее она могла ошибочно менять length и удалять элементы при переполнении.
Обычное свойство/сеттер сохраняет работу; наследуемый сеттер не растит length.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/object/script_object.rs parse_array_index uses u8::is_ascii_whitespace.
The Rust contract explicitly excludes vertical tab:
https://doc.rust-lang.org/std/primitive.u8.html#method.is_ascii_whitespace
This source comparison is not an original Flash Player oracle.
Сравнение исходников не заменяет проверку оригинальным Flash Player.

array_index_whitespace checks every ASCII control plus SPACE/DEL, Unicode
whitespace, counted NUL, signs, Int32 wrapping and literal key identity. It
uses C API, interpreter and explicit/automatic JIT across host and SWF4–8;
roundtrips preserve noncanonical keys and length through GC.
array_index_whitespace_effects checks own/inherited setters, readonly length,
protected elements, overflow truncation, detached receiver roots, GC, throws
and errors. array_named_index retains the former vertical-tab input as a
negative regression. General nonnumeric length assignment remains open.
Проверки охватывают C API, интерпретатор, JIT, перенос графа, GC и ошибки;
полная семантика Array и ненумерическая length остаются открытыми.

Validation / Проверено v0.517 on Linux x64 C++98: minimal 1064/1064,
AGG 1083/1083 and Cairo 1084/1084 passed, including successful reruns after
restoring executable permissions for 22/29/29 binaries. Five targeted
ASan/UBSan cases passed. Direct DT_NEEDED audit passed for 1073 Cairo ELF
artifacts, finding only libc, libgcc_s, libm and libstdc++; it does not inspect
transitive or dynamically loaded dependencies. Windows/macOS/Android acceptance
remains open / реальная приёмка остальных ОС остаётся открытой.
