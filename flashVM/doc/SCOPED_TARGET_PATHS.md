# Scoped target paths / Поиск путей в областях видимости

v0.256 adds the C89 [`fvm_exec_scope_target`](../include/flashvm/execution.h#L378) value service. Callers supply a managed
activation/With chain, selected fallback target, optional explicit this object and
counted path. Each scope retries the complete object path after a partial miss;
then target/global are tried. Missing paths return undefined, errors preserve out,
and output may alias input. No bytecode, operand stack or raw local map is needed.

All retries share the display root derived from the selected target. Absolute and
_root paths therefore do not accidentally start at an activation object. The
private resolver now accepts explicit display-root/this overrides; ordinary
SetTarget and explicit-base deletion callers retain their previous default policy.
SWF5+ exact `this` path components use the supplied receiver independently from
target selection. This does not claim full historical this-cell semantics.

The interpreter uses this service for Delete2 when a slash occurs before its last
dot/colon separator. It resolves the complete prefix, then invokes shared member
deletion without reading the final getter. A slash in the last member is literal;
pathless slash names remain simple bindings. Non-slash Delete2 keeps its previous
object-chain behavior. General GetVariable/SetVariable grammar and case/version
corner cases remain open; this is not complete path compatibility.

[GC:] Original scope, target, path, shared display root, explicit this and current
scope node remain rooted through getter calls. A getter may remove prior graph
edges and collect; retries retain the current node before following its parent.
The resolver independently retains its current object and explicit this. All root
slots use documented uppercase macros. Allocation exceptions restore root frames.

Общая C-служба повторяет полный путь для каждой управляемой области, затем для
выбранного клипа и глобального объекта. Абсолютные пути используют общий корень
сцены; `this` передаётся отдельно от текущей цели. Интерпретатор подключён к службе
для составных Delete2 со слешами. Последнее свойство удаляется без вызова getter.
Проверяются SWF4–8, неполные внутренние пути, внешние области, GC и исключения в
getters, самоудаление дескрипторов и отказ каждого выделения памяти до успеха.
Общая грамматика GetVariable/SetVariable и все стадии роудмапа остаются открытыми.

Evidence: 472/472 Linux AGG and 472/472 Cairo checks. Two AGG execute-permission
launch failures passed after permission restoration. Eleven selected heap/global
ASan + UBSan checks passed; host stack/leak coverage remains disabled. All 25
unchanged reference SWFs matched in each profile. Upstream target_path is SWF15,
so it remains outside the SWF<=8 acceptance corpus without changing its header.
Проверено по 472 теста двух профилей, 11 выбранных проверок памяти и 25 исходных
эталонов в каждом профиле. Тест target_path версии SWF15 исключён по версии.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_scope_target` | function | [include/flashvm/execution.h:378](../include/flashvm/execution.h#L378) |
<!-- source-links:end -->
