# Named execution roots / Именованные корни исполнения — v0.299

execution_roots.h documents the remaining shared-service temporary root layouts
with uppercase role macros. The handler sources use these names instead of raw
slot indices and array lengths. All macros are private implementation details;
public C89 signatures, callback argument order and serialized identifiers remain
unchanged. Callback arities use separate names from GC frame sizes, even when
the numeric values match.

| Service | Retained roles |
| --- | --- |
| Target selection | Original input, converted counted path |
| Numeric operations | Left and right operands through coercion/metamethods |
| Dynamic comparison/addition | Left/right operands, left/right concatenation text |
| Recursive loose equality | Its own independent left/right frame |
| Global register write | Original/copied input string |
| Frame control | Explicit clip, optional frame/label |
| Call frame | Converted/original request, explicit scope receiver |
| Trace | Original operand during string conversion |
| URL | Callback URL/target/flags prefix, original URL/target |
| Generated body entry | Capture, prepared this, suspended caller this |

[GC:] Each frame count matches its declared roles. Call-frame service previously
allocated three slots but used only two; the unused slot is removed. Native-body
entry explicitly assigns each object role before copying the array into roots.
Recursive loose equality has separate layout names from the outer four-value
comparison frame. Its existing defensive conversion recursion bound is named too.
Root-chain linkage, callback barriers, exception unwinding and conversion order
are preserved. No new Action or JIT machine-code backend is implemented here.

Индексы и размеры временных корней общих обработчиков теперь записаны именованными
макросами с комментариями о назначении каждого значения. Число аргументов callback
отделено от размера GC-кадра. Вызов кадра больше не выделяет неиспользуемый третий
слот. Вход в сгенерированное тело явно показывает capture, текущий и сохранённый
this; рекурсивное равенство имеет собственные имена корней. Публичный C-интерфейс,
порядок преобразований и формат сериализации не меняются.

Validation uses the existing direct-C services, collecting/throwing coercion,
allocation-failure, global-register and generated-body fixtures, followed by full
Linux AGG/Cairo regression suites and the accepted original-SWF oracle corpus.
Sanitizer coverage is limited to heap/global ASan and UBSan; stack and leak
instrumentation are excluded. Full compatibility and all roadmap stages remain
open; this change improves auditability of shared execution and GC ownership.

Result: Linux AGG/Cairo each passed all 581 tests; twelve AGG and nine Cairo
executables needed launch-permission repair and passed on retry. The six targeted
sanitizer fixtures passed. All 26 accepted original SWFs matched the pinned
oracle in both profiles. No stage-completion claim follows from these counts.
