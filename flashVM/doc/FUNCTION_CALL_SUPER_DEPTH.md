# CallFunction superclass depth / Глубина super при CallFunction — v0.510

Loaded SWF4–8 resolved function-value calls use explicit superclass depth zero,
including qualified CallFunction names. The selected this receiver is preserved;
property storage no longer supplies method lookup depth. The C global function
fallback uses the same rule after lookup, including getters/resolvers which
remove bindings. Native registration priority, construction, named CallMethod
and unloaded host inference are unchanged. Public direct [fvm_function_invoke](../include/flashvm/function.h#L79)
retains its existing embedding contract.

[GC:] Qualified lookup owners remain in caller root scopes independently of
callee metadata. The invocation scope retains the Function and restores prior
constructor/function/owner/depth state on completion, throw, error and allocation
failure. No public ABI or graph wire format changes.

В загруженных SWF4–8 CallFunction, в том числе квалифицированный, вызывает
полученное значение Function с глубиной super 0. Привязка this сохраняется;
владелец свойства удерживается отдельными корнями и не задаёт глубину метода.
Приоритет нативных регистраций, конструирование и именованный CallMethod не
меняются. Прежний контекст восстанавливается при успехе и ошибках.

Tests: function_call_super_depth reproduces the v0.509 failure and covers SWF4–8,
regular functions, DefineFunction2 (SWF7–8), own/inherited data or getter-returned
functions, short/missing chains, counted resolved names, C resolved/qualified and
interpreter/explicit/automatic JIT paths. function_call_super_roots deletes the
getter and detaches prototypes before collection, checking lookup-owner/capture
roots, explicit outer-depth restoration, throws/errors. function_call_super_allocation
sweeps new/new[] failure/retry. function_call_super_transport restores qualified
pure AVM1 functions into another context and checks public superclass reads/GC.
The prior qualified-call fixture now expects the depth-zero ancestor result (900)
rather than the defining-owner result (123); constructor/property assertions remain.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
activation.rs action_call_function calls CallableValue::call_with_default_this;
callable_value.rs invokes val.call for both bound/unbound values, and function.rs
Executable::call uses depth 0. This is source comparison, not a Flash Player oracle.
The same audit shows original anonymous CallMethod supplies Undefined this; its
value binding is still open (v0.509 preserves the embedding object's this).
Full this/closure semantics, SWF compatibility and all roadmap stages remain open.

Validation / Проверка: full Linux x64 C++98 minimal 1046, AGG 1065 and Cairo
1066 tests passed after execute-permission repairs on 5/5/7 generated binaries
and reruns. Six targeted ASan/UBSan tests and a strict C89 depth-header probe
passed. Foreign-OS acceptance and complete SWF compatibility remain open.

Follow-up v0.512: anonymous script this/preloads/super are corrected to Undefined;
see [current contract](ANONYMOUS_UNDEFINED_THIS.md). Earlier object/facade
expectations above describe the historical implementation and are superseded.
Уточнение v0.512: ожидания объекта this/super в старых проверках исправлены;
актуальная семантика описана в связанном документе.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../include/flashvm/function.h#L79) |
<!-- source-links:end -->
