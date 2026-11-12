# Retry alias-charge allocation / Повтор при нехватке памяти учёта алиасов

Copyright (c) 2026 bhgv

Fast packet accounting first sums allocation sizes for every strong occurrence.
If this upper bound exceeds the byte limit, it copies and sorts the native key
vector to charge each actual allocation once. The original owner order and its
reference multiplicity must remain unchanged.

The sort-vector malloc previously returned the same false result as a genuine
extent/byte-limit refusal. `prepare_release_packet()` and mapped-array child
transfer therefore lost the temporary failure reason. A supported object could
be routed to `difficult`, which strict native mode correctly rejects instead of
running a retained stopped-world finalizer.

The accounting helper now accepts an optional retry output. Only a native
sort-vector allocation failure sets it; hard extent/limit refusal leaves it
unchanged. Neither refusal writes the charge output or modifies keys. Both
packet-preparation paths pass their existing retry accumulator through. A prior
retry is never cleared. No roots, reference counts, packet cursors or managed
fields are consumed on allocation refusal. No exclusive path is enabled.

Fast сначала оценивает сумму по всем сильным вхождениям. При превышении лимита
он сортирует личную копию ключей и считает каждое выделение один раз. Раньше
нехватка памяти этой копии теряла причину временного отказа, и поддержанный
объект мог попасть в `difficult`. Теперь optional retry сохраняет временный
отказ; жёсткий предел остаётся жёстким. Оба пути подготовки пакета передают
этот признак без изменения владельцев, корней и исходного порядка ключей.

## Actual probes and limits

`test_packet_charge.py` derives the existing real wide-owner fixture, retaining
its production allocations, reference counts, root/epoch/physical-free checks,
and admitted guest relay. The parent contains 2300 mapped references to one
actual string child, as either a record or array. The parent and child must
physically free while two genuine guest threads remain admitted.

A wrapper lowers only this test packet's charge limit to native storage plus
one actual child allocation. Production payload sizes and counts are unchanged;
production byte limits are unchanged. This forces the real sorting branch at
small memory cost. The malloc wrapper refuses only its actual sort vector for
eight real Fast responses, then permits production malloc/sort/free to complete.
No synthetic work/progress or count substitution is used. Hard-limit refusal
also preserves output and both initially clear/set retry states.

Four positive shapes (record/array with/without sorting OOM), C/JIT and both
poll configurations give 16 profiles. Removing only the allocation retry
assignment must fail the actual leaf-refusal observer with exact status 12 in
eight profiles. Timeouts, crashes, compiler failures and sanitizer diagnostics
never count as evidence. `--positive-only` excludes intentional violations.

The ordinary production callers use `ULONG_MAX`; small alias packets normally
fit the initial upper bound and do not sort. The lowered test limit exercises
the refusal classification without claiming that an ordinary small production
packet reaches that branch. Linux x86-64 tests do not establish complete
ARM64/RV64 runtime or JIT coverage; C ThreadSanitizer does not instrument emitted
JIT instructions.

Тест уменьшает только лимит учёта выбранного пакета, а не размеры настоящих
объектов и не значения счётчиков. Штатные вызовы используют `ULONG_MAX`, поэтому
малые пакеты обычно не доходят до сортировки. Проверяются 16 положительных
сочетаний и восемь конкретных отказов при удалении новой защиты. Это проверка
классификации отказа памяти, а не заявление о частоте такой ветки в приложениях.

Validation: hosted native audit 33/33. ThreadSanitizer passed 16 new positive
profiles and all 152 concurrent wide-owner profiles, including memory/queue
pressure, competing retains, epochs and physical frees. Eight lost-retry
controls passed their exact status/message checks in the ordinary build.
Upstream snapshots remain unchanged; changed handwritten sources are at most
217 lines.
