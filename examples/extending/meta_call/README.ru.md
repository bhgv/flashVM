# Вызов метаметода __call

Рабочий пример находится в `tests/meta_call.c` и собирается тестовой целью `flashvm_tests_meta_call`.

Пример создаёт два объекта, назначает один метатаблицей другого, регистрирует `__call` как C-функцию и вызывает [`fvm_meta_call`](../../../include/flashvm/meta_call.h#L20) с двумя числами. В обработчик поступают объект-владелец, метатаблица и таблица аргументов с индексами 0 и 1. Результат — 42.

Этот API пока не вызывается автоматически из AVM1 и не поддерживает AS-замыкания. Для безопасности нельзя передавать объекты между контекстами.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](../../../include/flashvm/meta_call.h#L20) |
<!-- source-links:end -->
