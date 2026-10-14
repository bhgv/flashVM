# GetVariable service / Общий GetVariable — v0.268

`fvm_exec_get_variable` accepts explicit managed scope, target and this inputs,
a local-scope indicator and a value name. Loaded names undergo string coercion;
already-string names retain their complete count. Unloaded host names must be
strings. Qualified/slash and plain phases reuse the existing common services.
The interpreter now accepts loaded numeric/object names instead of rejecting
every non-String operand before coercion.

Loaded SWF4 GetVariable hides concrete MovieClip results as undefined, matching
the action-specific display-object rule in the primary source. The class
prototype remains an ordinary object. SWF5–8 and unloaded host reads retain
MovieClip values. This filter is specific to GetVariable: raw path/lookup services,
CallFunction and NewObject retain their own policies. Other display-object kinds,
full this-cell behavior and legacy Enumerate semantics remain open.

[GC:] Scope, target, this, original name, converted name and intermediate result
have documented root slots. Name conversion can collect while an otherwise
unreachable explicit target is retained by the service. Coercion/getter errors,
bad host names and allocation failures preserve output, including aliases.

`execution_get_variable` covers C89, unloaded/SWF4–8, numeric and counted-NUL
names, concrete MovieClips, name-conversion GC with a target-only property,
exceptions and aliases. `avm1_get_variable_names` covers numeric-name conversion
and MovieClip filtering through actual actions. `execution_get_variable_allocation`
checks conversion/lookup allocation boundaries and root/depth restoration.

Полный обработчик GetVariable вынесен в отдельный файл и C-интерфейс без p-кода
и стека. В загруженном SWF имя приводится к строке; в SWF4 результат MovieClip
скрывается как undefined. Низкоуровневый поиск и другие операции не получают этот
фильтр. Корни окружения и промежуточных значений сохраняются при callback и GC.
Остальные виды display-объектов, Enumerate и полная совместимость ещё открыты.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_get_variable`, and
[`value.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs)
`as_swf4_variable`. Source-based fixtures do not certify full Player equivalence.

The generated target-scope fixture formerly used SWF4 GetVariable(this) as a
MovieClip member receiver. Its SWF4 branch now writes through explicit qualified
clip paths, retaining the same target/value/restoration assertions. SWF5–8 keep
the original this-based member sequence; dedicated new fixtures assert the SWF4
undefined result. No independent SWF bytes or oracle traces were changed.

Validation: Linux GCC13.3 AGG/Cairo each pass 513 tests, with successful execute-
permission reruns for two AGG/three Cairo binaries. Eight selected ASan/UBSan
fixtures pass (heap/global coverage; no stack/leak checks). All 26 previously
accepted original SWFs match exact stdout with empty stderr in both profiles.
An additional unchanged swf4_vars probe is recorded as failing, not accepted:
SWF4, 472 bytes, SHA256 e39a55e70b2a719ac2b03cfb78b58e55f1ed76220ac39cdf3754d54714020afe.
Its upstream test.toml also declares known_failure=true. It exposes remaining
_global/parent/system-property behavior and is excluded from the passing corpus.
The get_variable_in_scope candidate has a SWF15 header and supplies no SWF<=8 evidence.
