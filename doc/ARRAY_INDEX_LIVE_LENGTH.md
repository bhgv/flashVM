# Indexed writes and live Array length / Индексные записи и текущая длина Array

Implemented in v0.524 for loaded SWF4–8. This is a source comparison with
Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd, `ScriptObject::set_local`,
`length` and `set_length`; it is not an original Flash Player oracle.

Ordinary SetMember, named C API writes and indexed C table writes read current
own raw `length` data, convert it to Int32 once, and grow before own data flags
or own setters. Missing/inherited length reads as undefined; a deleted native
accessor never revives cached native length. Growth creates ordinary own data
when necessary, bypassing length setter/watch dispatch. Readonly length keeps
its data; truncation still precedes that decision and only uses old Number
storage. Signed parsed names keep their literal spelling. INT32_MAX grows to
INT32_MIN and applies nonnegative-tail deletion. Inherited indexed setters
retain the existing no-growth dispatch path.

Loaded writes no longer update the native cache after storing an index. An
own setter may replace/delete its descriptor or change length; fallback storage
preserves that result without a second conversion/growth pass. Host-only arrays
retain their previous post-store growth policy. No public ABI or graph-format
change is introduced.

[GC:] The receiver and watched input remain rooted across live length coercion,
including self-deletion and removal of global bindings. Length conversion and tail deletion share one execution quota.
Length-coercion errors/throws stop
indexed storage. OOM can leave earlier length mutation visible; retry is
supported, without promising transactional rollback.

Реализовано для загруженных SWF4–8: SetMember и C API читают собственные сырые
данные `length`, выполняют одно преобразование Int32 и увеличение до проверки
флагов и собственного сеттера индекса. Удалённый accessor не восстанавливает
длину из кэша; отсутствующая или унаследованная длина читается как undefined.
Рост создаёт обычное собственное свойство без вызова watcher/сеттера длины.
Readonly сохраняет значение длины, но удаление хвоста выполняется раньше и
только по старому Number. При переполнении INT32_MAX получается INT32_MIN.
Литеральное написание индексного имени сохраняется.

После записи повторного изменения длины нет: результат сеттера сохраняется.
Получатель и записываемое значение защищены при GC внутри valueOf. Ошибка или
исключение при преобразовании длины останавливают запись индекса; после OOM возможны частичные изменения
длины, повторная попытка допустима. Сравнение с Ruffle не заменяет Flash Player.

Tests / проверки: `array_index_live_length` checks SWF4–8, C named/table APIs,
interpreter and explicit/automatic JIT, eleven live-length representations,
readonly length/index controls, literal names and overflow. Its roots variant
checks collecting self-deleting coercion, detached incoming object graphs,
errors/throws and descriptor-removing setters that reset length. Its allocation
variant checks shared coercion/truncation quota and sweeps OOM boundaries for missing/String/Number lengths and checks
retry, counted embedded-NUL strings, primitive metadata and cycle retention.

Still open / остаются открытыми: nonnumeric assignment to the original built-in
length accessor, complete virtual/method-valued length semantics, full Array/SWF
compatibility and acceptance of all nine roadmap stages. Linux validation and
emulation do not establish native Windows/macOS/Android acceptance.

Validation / Проверено v0.524 on Linux x64 C++98: minimal 1083/1083,
AGG 1102/1102, Cairo 1103/1103 passed after restoring executable permission
on four/three/one final-run test files respectively. Seven targeted ASan/UBSan
cases passed (one permission-only rerun). An older sortOn fixture was reordered
to install its elements before coercible length, avoiding unintended setup-time
callbacks/GC; its intended sortOn callback ordering remains checked. Fresh
Unicorn instruction/ABI checks passed 26 cases for x86/ARM/Thumb/ARM64/RV32/RV64;
x64 uses native JIT. Direct DT_NEEDED audit passed 1092 Cairo ELF artifacts,
with only libc/libgcc_s/libm/libstdc++; transitive/dlopen dependencies and actual
foreign OS acceptance are not certified. Все перечисленные проверки успешны;
полная совместимость и девять этапов остаются открытыми.

Follow-up v0.525 extends the shared own length reader to native method-valued
slots; see [RU/EN contract](ARRAY_METHOD_LENGTH.md).
В v0.525 индексная запись также учитывает нативный метод в роли length.
