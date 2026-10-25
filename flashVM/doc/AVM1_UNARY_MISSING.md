# Missing unary operands / Недостающие унарные операнды — v0.300

Loaded AVM1 bytecode now supplies undefined when an audited unary operation reads
an empty operand stack. The existing versioned C execution service then performs
the operation. Previously these dispatch branches returned [FVM_ERROR](../include/flashvm/flashvm.h#L45) before
reaching that service. Host-only execution without a loaded SWF retains its
strict missing-operand error; explicit C service input pointers remain mandatory.

Affected operations: Not, StringLength/MBStringLength, CharToAscii/MBCharToAscii,
AsciiToChar/MBAsciiToChar, ToInteger, TypeOf, ToNumber, ToString, Increment and
Decrement. Existing TargetPath, Enumerate/Enumerate2 and RandomNumber missing-value
handling now uses the same interpreter-only preparation helper without changing
those policies. Other Action stack underflow policies remain to be audited.

| Empty-stack operation | SWF4–6 result | SWF7–8 result |
| --- | --- | --- |
| Not | true | true |
| ToInteger | 0 | 0 |
| ToNumber | 0 | NaN |
| Increment / Decrement | 1 / -1 | NaN / NaN |
| TypeOf | "undefined" | "undefined" |
| ToString | empty string | "undefined" |
| StringLength / MBStringLength | 0 | 9 |
| CharToAscii / MBCharToAscii | 0 | 117 (first unit of "undefined") |
| AsciiToChar / MBAsciiToChar | empty string | empty string |

[GC:] The operand vector is already linked to the interpreter's active root
frame. The helper initializes undefined before insertion and invokes no script.
Allocation exceptions use the existing interpreter containment scope. Services
retain their own coercion roots and conversion/exception order. No bytecode or
stack parameter is added to the public C execution API.

Primary comparison source: pinned Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/runtime.rs (Avm1::pop) and core/src/avm1/activation.rs (unary actions).
The former returns undefined on stack underflow; actions pass that value to their
normal versioned conversion. This is a bounded source comparison and regression
fixture, not independent Flash Player certification or complete Action coverage.

avm1_unary_missing executes raw Action bytes in host and loaded SWF4–8 modes,
checking the result table above. avm1_unary_missing_allocation injects failures
through execution preparation and operand insertion, verifies root/execution/depth
restoration, and collects between retries for SWF4–8.

Загруженные SWF теперь передают undefined в общие C-обработчики, когда выбранной
унарной операции не хватает значения на стеке. Раньше интерпретатор преждевременно
возвращал ошибку. Преобразование undefined зависит от версии SWF; результаты
приведены в таблице. Строгий host-режим и проверка указателей публичного C API
сохранены. Тесты проверяют исходные байты Action-кодов, версии SWF4–8, отказы
памяти и восстановление корней/глубины исполнения. Остальные случаи недостатка
операндов ещё требуют отдельного аудита; этап 2 остаётся открытым.

Validation: Linux AGG and Cairo each passed 583 tests after updating two existing
fixtures that still expected the superseded loaded-unary underflow error. Those
first assertion failures are retained in validation logs; their raw Action bytes
are unchanged. Both new heap/global ASan+UBSan fixtures passed (no stack/leak
coverage). All 26 accepted original SWFs matched the oracle in each profile.
Dispatch references remain 94/100 in the bounded pinned catalog.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
