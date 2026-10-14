# Method argument counts / Число аргументов метода

v0.243 adds `fvm_exec_argument_count`, a C89 value service used by CallMethod and
NewMethod. Loaded counts follow existing version-aware number conversion, then
unsigned 32-bit truncation/modulo. NaN and either infinity become zero. Calls
consume the minimum of that count and the available operand values. The supplied
argument vector, rather than the requested count, determines the callee's argc.

The actual argument count remains bounded by FVM_EXEC_MAX_METHOD_ARGUMENTS;
exceeding that resource bound reports FVM_ERROR. A huge requested count with
few available values does not allocate a huge vector. Unloaded host execution
retains its strict integral-number/sufficient-operands contract. The available
count is a value-vector length supplied by a caller, not a VM-stack pointer.

[GC:] A coercible count is retained through its valueOf callback. Conversion
errors preserve the output count; failures precede argument removal/dispatch.
Tests include SWF4–8, text, fractional/negative counts, wrapping, NaN/infinity,
short argument vectors, collection and exceptions in a prepared native callback,
and allocation failure/root restoration. CallFunction/NewObject count conversion
and full call dispatch remain separate open work.

Primary comparison: Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/activation.rs`, action_call_method/action_new_method and pop_call_args.
This does not replace independent Flash Player acceptance for these new fixtures.

Число аргументов CallMethod/NewMethod теперь проходит версионное числовое
преобразование и приведение к беззнаковым 32 битам. Если значений меньше,
передаются доступные; огромный запрошенный счётчик не вызывает огромного
выделения памяти. Реально передаваемое число ограничено документированным
пределом ресурсов. Объект-счётчик защищён корнем во время valueOf, включая GC
и исключение. Хостовый режим без SWF сохраняет прежнюю строгую проверку.
Остальные виды вызовов и все стадии роудмапа остаются открытыми.

Evidence: 450 tests pass in each Linux AGG/Cairo profile after rebuilding the
extended constructor fixture and updating the synthetic shortage expectation.
Two Cairo execute-bit launch failures pass after restoring permissions. Eight
focused heap/global ASan+UBSan checks pass; stack/leak coverage is unavailable.
All 23 unchanged original SWF oracles still match in each graphics profile.
