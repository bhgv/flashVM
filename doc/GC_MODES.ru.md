# Режимы GC

- `0`: обычный порог — 1000 созданных объектов.
- `-100`: порог 1900; `100`: порог 100.
- `101` ([`FVM_GC_SUSPENDED`](../include/flashvm/gc.h#L23)): автоматические шаги не выполняются.
- `102` ([`FVM_GC_FULL`](../include/flashvm/gc.h#L26)): один принудительный проход без смены режима.

Порог не означает немедленную сборку: приложение должно вызывать [`fvm_gc_step`](../include/flashvm/gc.h#L90) в безопасных точках. Не запускайте GC во время исполнения AVM1, пока не реализована полная трассировка его стеков.

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
