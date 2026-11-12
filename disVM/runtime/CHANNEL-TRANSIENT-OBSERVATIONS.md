# [GC:] Actual channel reservation and buffer-change refusals

Copyright (c) 2026 bhgv

The channel fixture adds four shapes beside the existing quiet-claim cases:
unbuffered and mapped-buffered channels with an actual match reservation, and
ordinary/mapped buffered channels with an actual temporarily changed buffer.
Each runs C/JIT with both polls, while the original three-resident close protocol
continues to verify owner-mailbox delivery and fixed execution-thread identity.

A real `DVM_ChannelMatch` stack ledger is linked/unlinked under the queue mutex
with a different live native peer's context identity. The original claim observes
the reservation through the real busy query; no busy result is substituted and
no payload mover runs. The ledger is removed before returning to Fast, so a later
snapshot is empty. Source reference count stays one throughout.

The buffer-change case retains the actual array, moves its field owner to the
diagnostic under the queue guard, and presents `H` in the channel field while the
claim still expects the original array. The real native claim must refuse.
The original owner returns to the field before the buffered packet rolls back.
No count, successful claim, collector result or allocation is fabricated.

RU: Добавлены настоящие резервации сопоставления и смена буфера под мьютексом.
Стековая запись match использует личность другого живого native-соседа. Исходный
захват видит busy; до поздней проверки Fast резервация уже снята. Перемещение
данных не выполняется и ответ busy не подменяется. Смена поля буфера сохраняет
настоящую ссылку массива, получает настоящий отказ старого снимка и возвращает
владельца в поле до отката пакета. Счётчик источника всё время равен одному.

Four private host omissions remove busy-reason capture or the expected-buffer
identity comparison, one per new shape. They must fail exact semantic predicates,
not build errors, process timeouts, signals or sanitizer warnings. The default
matrix now contains 156 positive applications and 32 omission controls;
`--channel-only` selects 32 applications and eleven channel controls.

These are protocol fault tests under existing lifetime/admission protection,
not a benchmark or a claim that a payload mover executed in the synthetic overlap.
General stopped tracing, unknown callbacks and stopped shutdown remain. Linux
x86-64 execution is covered; ARM64/RV64 execution is not established here.
