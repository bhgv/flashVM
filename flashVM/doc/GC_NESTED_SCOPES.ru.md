# Вложенные области временных корней GC

Нативные расширения могут открывать несколько областей временных корней. Области закрываются строго в обратном порядке (LIFO). Попытка закрыть внешнюю область при открытой внутренней возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), сохраняя обе области.

Область копирует значения [`FVM_Value`](../include/flashvm/value.h#L26). Ссылки на объекты удерживают соответствующие объекты от удаления до закрытия области. Это не передаёт владение объектами и не гарантирует безопасность указателя после последующей сборки.

Тест `tests/gc_nested_scopes.c` проверяет вложенность, неправильный порядок закрытия, сохранение объектов при сборке и освобождение после закрытия областей. **Полная интеграция корней AVM1 и нативных вызовов ещё не завершена.**

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
