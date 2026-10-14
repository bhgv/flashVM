# Shared execution checkpoints / Общие контрольные точки исполнения

v0.228: `fvm_exec_checkpoint` is a C89 execution service used by the interpreter
and available to generated script bodies. It accounts semantic instruction units,
updates a debugger mapping token and optionally polls cooperative worker stop
requests and scheduled GC. No Action code, bytecode buffer, operand stack or
machine-code format is part of the signature. `FVM_EXEC_POLL` requests polling;
flags 0 only account/update. Zero units permit polling without additional work.

Общий C-сервис учитывает смысловые единицы исполнения, обновляет отладочную
позицию и по флагу проверяет остановку worker и необходимость GC. Интерпретатор
использует его на прежних границах: одна единица на Action, poll на первом и
далее каждом 64-м шаге. Нативное тело вызывает тот же сервис на своих границах.
Единицы — стоимость семантики программы, а не число инструкций процессора.

Top-level entry resets the counter; nested generated/interpreted calls share it.
Overflow returns FVM_ERROR without wrapping. Exceeding a nonzero configured limit
returns FVM_ERROR after accounting the rejected work. Invalid flags or use outside
an active script/execution entry return FVM_INVALID_ARGUMENT. Generated code must
propagate failure immediately and report costs corresponding to interpreter work.
This establishes the bridge, not correctness of a backend's chosen accounting.

Нативный и вложенный интерпретируемый вызовы делят один бюджет. Переполнение
счётчика не оборачивает его. Ошибки лимита/остановки необходимо возвращать из
сгенерированного тела; игнорирующий их код не соответствует контракту JIT.
Вызов из host вне активного исполнения запрещён. Генераторов машинного кода
для шести архитектур здесь ещё нет, и стадия JIT не закрыта.

[GC:] Polling is allowed only with all live values rooted in activation/register
slots or explicit GC scopes. The collector preserves existing suspended/native
barrier rules, and an enclosing collection is not recursively entered. Debugger
locations and this/target/depth are restored by the shared call path on return,
budget failure, cancellation and exceptions. No conservative CPU-stack scan.

[GC:] Caller обязан опубликовать корни до poll. Проверены сохранение нового
объекта в регистровом слоте, принудительно запланированный GC и восстановление
после ошибки. Обычные нативные callbacks сохраняют запрет повторного GC; вызов
poll не обходит этот барьер.

Evidence: Linux default AGG profile passed all 430 tests/examples.
`execution_checkpoint.cpp` checks nested budget accumulation, repeated recovery,
source mapping/restoration, GC register roots, invalid flags/inactive calls and
unsigned-counter overflow. `execution_checkpoint_worker.c` exercises the public
ABI in C90, self-requested cooperative stop and cleanup. The six selected
checkpoint/budget/GC/native-body tests also pass minimal heap/global ASan + UBSan
with the documented stack/leak restrictions. Existing worker/interpreter tests
retain their behavior. No other target-platform runtime certification.

## Restored original SWF probes / Восстановленные исходные SWF

Twenty-two unchanged SWF<=8 files and their outputs were retrieved from Ruffle
commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`, under
`tests/tests/swfs/avm1/`. SHA identities are retained in the separate validation
artifact. All use explicit runner policy `--acknowledge-quit --timeline 1`;
the SWFs and output oracles are unchanged. This fixes the earlier actions-only
host-policy comparison: original AS2 v8 now passes byte-exactly (1841 bytes,
zero exit, empty stderr); define_function2_preload_order also passes completely.

| Status | Original probes |
| --- | --- |
| Byte-exact, zero exit, empty stderr (15) | add_swf4, add_swf5, as2_super_and_this_v8, biturshift_swf8, define_function2_preload_order, equals_swf4, equals_swf5, equals2_swf5/6/7, lessthan_swf4/5, lessthan2_swf5/6/7 |
| Trace mismatch (3) | divide_swf4 (number formatting precision), strictequals_swf6 (NaN equality), object_prototypes (membership/case/deletion subset) |
| Execution incomplete (4) | function_suppress_and_preload, this_swf5, this_swf6, this_swf7 |

These are source/oracle comparisons using the FlashVM runner, not an independent
Flash Player execution here. Four additionally retrieved files use SWF15/17 and
are excluded from this SWF<=8 acceptance set. The failed probes remain open work,
not removed or treated as passing. All nine roadmap stages remain open.

Из 22 исходных SWF<=8 полностью совпали 15. Остальные семь остаются в реестре
как конкретные ошибки/расхождения. Нельзя переносить общий статус «совместимо»
на основании этих проб. Платформы Windows/macOS/Android пока не проверены
реальным запуском; общая приёмка пункта 9 остаётся открытой.
