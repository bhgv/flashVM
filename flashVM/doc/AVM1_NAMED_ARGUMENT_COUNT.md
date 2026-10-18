# Named call counts / Число аргументов именованных вызовов

v0.244 connects CallFunction and loaded NewObject to the same value-based
[`fvm_exec_argument_count`](../include/flashvm/execution.h#L482) service already used by CallMethod/NewMethod. Loaded
SWF4–8 calls convert text/object/numeric counts, truncate/wrap to unsigned 32 bits,
and consume at most available values. The actual vector remains resource bounded.

Unloaded CallFunction keeps its strict count contract through the service.
Unloaded NewObject preserves its distinct previous literal-entry bound and strict
number/integer/available-operand checks. Constructor dispatch and function-name
conversion have not changed in this checkpoint. Full name coercion, missing and
noncallable binding policy remain open; no complete AVM1 conformance is claimed.

The new fixture executes both named actions for each SWF4–8, with a text count
larger than the available arguments and a fractional count truncating to zero.
It checks the actual argc seen by an attached native script body, scalar call
results and constructor object results. Existing count-service GC, exception and
allocation fixtures continue to cover the shared conversion path.

CallFunction и NewObject в загруженных SWF теперь используют ту же C-службу
счётчика, что и вызовы методов. При нехватке аргументов передаются доступные,
дробная часть отбрасывается; число реально передаваемых значений ограничено.
Для хостового NewObject сохранён прежний отдельный предел. Преобразование имён
и остальные пробелы диспетчеризации остаются открытыми, как и все стадии роудмапа.

Evidence: Linux AGG/Cairo each pass 451 tests; seven focused heap/global
ASan+UBSan checks pass (stack/leak instrumentation unavailable). All 23 unchanged
original SWF traces remain exact in each graphics profile. Non-Linux execution
and machine-code JIT remain unverified/unimplemented respectively.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_argument_count` | function | [include/flashvm/execution.h:482](../include/flashvm/execution.h#L482) |
<!-- source-links:end -->
