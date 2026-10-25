# Метаметоды C API (этап 23)

Заголовок `flashvm/metamethod.h` добавляет регистрацию нативных обработчиков
`__index`, `__newindex`, `__iterate` и `__gc` в обычной общей метатаблице.
[`fvm_table_set_metatable`](../include/flashvm/table.h#L30) присоединяет метатаблицу; [`fvm_table_get_metatable`](../include/flashvm/table.h#L34)
возвращает ссылку на ту же таблицу, без копирования.

Все обработчики получают контекст, объект `self`, массив аргументов, результат
и пользовательские данные. `argv[0]` — исходная таблица, `argv[1]` — метатаблица.
У `__index` третий аргумент — строковый ключ; у `__newindex` третий и
четвёртый аргументы — ключ и новое значение. У `__iterate` третий аргумент —
номер вызова, начиная с нуля. Возвращаемое `undefined` завершает итерацию.
Пока пользовательский итератор возвращает только значение, без отдельного ключа.

`__gc` получает третьим аргументом ссылку на ещё существующий объект.
Вызывается не более одного раза при [`fvm_meta_finalize`](../include/flashvm/metamethod.h#L82) или уничтожении
контекста. **Это ещё не сборщик мусора:** недостижимые объекты автоматически
не выявляются. Финализация примитивов и данных, доступных только из C,
пока отсутствует. Нет ещё функций ActionScript как значений, `pairs()` в
синтаксисе AS и полного набора метаметодов Lua.

Прототип AVM1 проверяется раньше `__index`, а запись существующего свойства
обходит `__newindex`. Обработчики пока регистрируются отдельно от обычных
полей метатаблицы. Это временное ограничение до реализации функций как значений.

Компилируемый пример C89 и проверка: `tests/metamethods.c`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_meta_finalize` | function | [include/flashvm/metamethod.h:82](../include/flashvm/metamethod.h#L82) |
| `fvm_table_get_metatable` | function | [include/flashvm/table.h:34](../include/flashvm/table.h#L34) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:30](../include/flashvm/table.h#L30) |
<!-- source-links:end -->
