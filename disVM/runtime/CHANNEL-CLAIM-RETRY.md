# [GC:] Preserve channel claim refusal under its queue mutex

Copyright (c) 2026 bhgv

## EN: Keep the actual observation

Fast formerly received only a boolean quiet-channel claim result, then sampled
buffer reservations, references and waiter close readiness again. A short-lived
retain could defeat the actual atomic claim and disappear before those samples.
The callback could classify a supported channel as a hard retained refusal.
The same later-sample problem applied when a transfer or hosted waiter vanished.

The host now offers `dvm_runtime_channel_claim_observed`. Under the existing
channel queue mutex it preserves a changed buffer, active transfer, hosted waits
or a lost quiet claim in an optional initialized retry flag. That observation
is paired with the original claim, not reconstructed afterward. Fast retains
the flag through later samples and through buffered-packet rollback. Failed
claims preserve channel, buffer, descriptor and queue ownership. Successful claims
still transfer the mover descriptor and consume the original last reference.

The original boolean `dvm_runtime_channel_claim_quiet` remains a wrapper for
existing graph finalization and callers, including its already-claimed count-zero
mode. No world stop or additional execution mutex is introduced. The existing
queue mutex only protects this metadata operation; no foreign Prog runs here.

Hosted-waiter classification is shared with `dvm_runtime_channel_post_close`.
That path still posts once to each owning resident's mailbox and wakes the owner;
it neither executes another resident nor replaces its live registers. Foreign
native waits and unsupported transport `aux` stay hard refusals rather than
being silently admitted as shared finalizers.

## RU: Причина сохраняется там, где наблюдалась

Прежний bool терял причину отказа: временная ссылка, передача или ожидание могли
исчезнуть до следующего чтения состояния. Теперь причина сохраняется под тем же
мьютексом очереди, что защищает настоящий захват. Fast не затирает её поздними
снимками или откатом пакета буфера. Отказ не передаёт владение каналом, буфером,
дескриптором или очередями. Успех сохраняет прежний протокол.

Старый bool-интерфейс остаётся совместимой обёрткой, включая уже захваченные
нулевые счётчики графов. Hosted-закрытие по-прежнему идёт через почтовый ящик
постоянного владельца Prog. Чужой Prog не исполняется сборщиком, регистры не
переносятся. Транспорт и чужие native-ожидания остаются жёсткими отказами.

## Actual native regression / Настоящая проверка

Four `cnewc` shapes cover buffered/unbuffered channels and ordinary/mapped
`movtmp` payloads. Each runs interpreted/JIT with both polls. A genuine extra
reference and root defeat the original quiet claim; the diagnostic verifies its
captured retry and removes that owner before Fast's later sample. Buffered cases
roll back their real child-root packet before retry. Physical frees, allocation
generations, root/reference withdrawal and final Type creator ownership are checked.

Each channel application also starts a third permanently bound Limbo resident
blocked on a real receive. A publisher hook retains that actual channel rather
than fabricating a waiter. Process state is sampled and the channel reference
retained under the actual queue mutex; the fixture mutex cannot protect against
concurrent scheduler wakeups. Root publication follows after unlocking.
The parent probes the real refusal, posts the real
close message, and remains under shared admission while the owning resident
handles `channel hangup` and withdraws its registration. A later close sample
must be empty. Two other native peers remain admitted throughout. All three
residents finish on their original OS threads and join normally.

A brief queue-guarded unsupported `aux` tag separately checks the hard-refusal
boundary without invoking a transport callback. It is restored before the actual
quiet claim. No successful claim or collector response is fabricated.

Seven new semantic controls omit queue-locked capture for each channel shape,
overwrite the captured buffered/unbuffered retry, or incorrectly retry unsupported
transport. Each requires its specific semantic failure; compiler errors, signals,
sanitizer reports and process timeouts never pass a control. Private host
composition relinks use the same CMake flags and leave production objects intact.

The default fixture now contains 140 positive C/JIT/poll applications and 28
semantic omissions. `--channel-only` selects 16 applications and seven controls;
`--positive-only` retains its existing meaning. Older 124 owner cases remain.

After correcting the diagnostic state-read race, final validation passed all 98
selected runtime CTest checks. TSan passed five complete 140-profile repetitions
(700 VM runs), 44 graph-claim, 108 module-cycle and 36 Duke applications: 888
positive executions without warnings. All 28 semantic omission controls passed.
Changed handwritten sources are at most 433 lines; upstream remains unchanged.
This is scoped validation, not a claim that the entire repository suite passes.

General stopped tracing, unsupported native finalizers, hard graph limits and
stopped shutdown remain. Execution is Linux x86-64; C TSan does not instrument
emitted JIT instructions. ARM64/RV64 execution is not established here.
