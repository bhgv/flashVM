# Временные корни GC для нативного кода

Заголовок `flashvm/gc_roots.h` предоставляет совместимый с C89 интерфейс временных корней.

## Порядок использования

1. Перед операциями с объектами, ещё не сохранёнными в таблицах VM, вызвать `fvm_gc_roots_begin(context, &scope)`.
2. Для каждого значения-объекта вызвать `fvm_gc_roots_add(scope, &value)`.
3. Пока область активна, `fvm_gc_collect()` считает эти объекты корнями и обходит их свойства, индексные элементы, прототипы и метатаблицы.
4. Закрывать области строго в обратном порядке через `fvm_gc_roots_end(scope)`.

Значения копируются в область; **строковые буферы не копируются**. Механизм защищает объекты GC, но не произвольную память C. Нельзя передавать область между потоками или использовать после уничтожения контекста. Контекст должен жить дольше всех областей.

## Ограничения

Это явная регистрация корней, а не полная автоматическая защита всех временных значений. Ещё нужно проверить все нативные вызовы и результаты, управление строками, повторный вход в финализаторы, автоматические безопасные точки, владение VM в потоках и объект ActionScript `gc`. Считать сборщик полностью законченным пока нельзя.

Рабочий тест: `tests/gc_native_roots.c`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_roots_add` | function | [include/flashvm/gc_roots.h:30](../include/flashvm/gc_roots.h#L30) |
| `fvm_gc_roots_begin` | function | [include/flashvm/gc_roots.h:27](../include/flashvm/gc_roots.h#L27) |
| `fvm_gc_roots_end` | function | [include/flashvm/gc_roots.h:33](../include/flashvm/gc_roots.h#L33) |
<!-- source-links:end -->
