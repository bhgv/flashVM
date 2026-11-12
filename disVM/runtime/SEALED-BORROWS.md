# Borrow cutoff at native shutdown / Граница borrow при native shutdown

Copyright (c) 2026 bhgv

## Two lifecycle holes

Sealing closes the native world after guests have joined and concurrent queues
have drained. Existing idle collector registrations may remain until their OS
threads join. Previously `dvm_world_read_begin()` did not inspect the sealed flag,
so those registrations could announce a new raw-pointer borrow after the final
quiescence check. The controller could also seal while its own explicit borrow
was live: the seal checked other participants' borrow depths, but skipped its own.

Both decisions now use the same existing world mutex. Sealing rejects any live
borrow, including the controller's, before setting the sealed flag. Every new
borrow begin checks that flag before changing nesting depth or reader epoch.
The check covers ordinary participants and collector services. Existing matched
ends and detach operations retain their contract. No new wait, global mutator
exclusion, forced park or retained tracing fallback is introduced.

Sealing закрывает native-мир после join гостей и обработки конкурентных очередей.
Простаивающие службы могут оставаться зарегистрированными до join их OS-потоков.
Раньше они могли открыть новый raw borrow после последней проверки покоя.
Контроллер также мог закрыть мир с собственным живым borrow. Теперь обе проверки
выполняются под существующим mutex мира: sealing требует отсутствия всех borrow,
а begin проверяет закрытие до изменения глубины и эпохи. Дополнительных ожиданий,
остановки исполнителей или резервной трассировки нет.

## Public epoch ABI

Five reader-epoch declarations had been appended after the public header's
closing `extern "C"`. C++ clients therefore requested mangled symbols that the
C implementation did not export. Move the linkage closing boundary after all
public declarations. A real C++98 consumer now begins/ends a borrow, observes
borrowers, issues an epoch and checks its completion against the actual C world.
It runs while the controller is idle before each lifecycle probe.

Пять объявлений эпох находились вне `extern "C"`, поэтому клиент C++ не мог
связаться с реализацией на C. Граница linkage перенесена после всех объявлений.
Настоящий клиент C++98 вызывает все пять функций на живом idle-участнике.

## Acceptance

`disVM/tests/native_seal.py --flashvm-root flashVM` compiles production world
sources privately and tests eight combinations: reference/concurrent world,
atomic/locked poll configuration and ACTIVE/IDLE controller. An actual OS service
holds two nested borrows. The controller must refuse sealing until both ends;
a failed seal keeps the world open, and a successful one forbids later borrows
from both threads. The service detaches and joins before contexts/world are freed;
the controller keeps its live register storage throughout.

Three semantic mutation families run in all eight combinations (24 controls):
remove the sealed check, bypass it only for services, or omit the controller's
own borrow from the seal check. Each must fail with exact status 12 and its
specific invariant. Restoring the old C++ linkage boundary must compile the
consumer but fail linking all five epoch symbols. Build errors, sanitizer reports,
signals and timeouts are never accepted as lifecycle mutation evidence.

`--positive-only` skips deliberate violations for sanitizer runs. The eight
positive profiles pass with all C and C++ probe objects built under ThreadSanitizer.
The test is registered in both standalone host and hosted runtime CTest builds.
It requires a C++98 compiler for its public ABI check; it adds no production C++
runtime dependency. Testing here is Linux x86-64; these checks do not establish
complete ARM64/RV64 runtime or JIT coverage.

Проверка использует настоящие системные потоки, вложенность borrow и join.
Негативные варианты обязаны нарушить конкретный инвариант, а не упасть случайно.
Положительные варианты проходят TSan. Проверяется Linux x86-64; полная
реализация runtime/JIT для ARM64/RV64 этим не подтверждается.

Validation for this change: hosted native audit 31/31; standalone host 22/22.
ThreadSanitizer passed eight new world lifecycle profiles, eight strict
release/drain profiles and 36 module-retirement profiles. C/JIT coverage in
the latter checks does not instrument emitted JIT machine code. Upstream
snapshots remain unchanged, and every changed handwritten source is at most
413 lines.
