# GC modes

- `0`: normal allocation threshold, 1000 objects.
- `-100`: threshold 1900; `100`: threshold 100.
- `101` ([`FVM_GC_SUSPENDED`](../include/flashvm/gc.h#L23)): skip automatic steps.
- `102` ([`FVM_GC_FULL`](../include/flashvm/gc.h#L26)): one forced pass, preserving prior mode.

A threshold does not schedule collection on its own: the host must invoke [`fvm_gc_step`](../include/flashvm/gc.h#L90) at safe points. Do not collect inside AVM1 execution until evaluation stack roots are traced.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_GC_FULL` | macro / define | [include/flashvm/gc.h:26](../include/flashvm/gc.h#L26) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_GC_SUSPENDED` | macro / define | [include/flashvm/gc.h:23](../include/flashvm/gc.h#L23) |
<!-- source-links:end -->
