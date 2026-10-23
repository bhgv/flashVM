# Наблюдатели прокрутки TextField

Расширяется [bounded bridge прокрутки](TEXT_SCROLL.ru.md).
[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
описывает порядок field-first, регистрацию listeners и аргумент source field.
Здесь триггер — эффективное изменение scroll/hscroll явным offset setter. Ввод,
автоматические события всей геометрии, HTML/mixed styled fields, полная reference
семантика и физические платформы остаются открытыми. Все девять стадий открыты.

## Регистрация и владение графом

TextField.addListener принимает same-context object identity; повтор возвращает
true без изменения порядка. Отсутствующий/non-object аргумент даёт false без
coercion. removeListener сообщает, была ли удалена identity. Foreign-context
значения недопустимы. Поддержаны SWF6–8 и unloaded host context. Работа выполняется
на потоке владельца; staging не вызывает script. Ошибка выделения/work сохраняет
живую регистрацию. Лимит — 4096 listeners.

Регистрация хранится в private nonenumerable/nondeletable managed edge к компактному
нативному Array. Это деталь реализации, а не публичный изменяемый список. Обычные
tracing и graph serialization удерживают listeners и циклы; в native Field нет
скрытых raw listener pointers. Receiver независимо владеет fonts/fields/listeners.
Новые native callback IDs стабильны. Host callbacks и captured roots сохраняют
свою identity.

## Транзакция явного уведомления

[FVM_TextFieldScrollPlan](../src/standard/text_listeners.hpp#L25) удерживает source и копию порядка регистраций до публикации
обоих offsets. Сначала выполняются conversion и свежее измерение. Ошибка подготовки
allocation/work сохраняет offsets. После публикации вызывается field onScroller,
затем snapshot listeners; аргумент — source field. Missing/noncallable handler
игнорируется. Первая ошибка lookup/callback/work останавливает доставку и передаётся;
commit и эффекты callback сохраняются. Вложенные записи не перезаписываются.
Изменения регистраций влияют на следующие события; handler разрешается перед
вызовом каждого recipient. Unchanged/clamped-same позиция не уведомляет.
Query/path/capture сохраняют no-script контракт. Native accessor barrier
отклоняет collection внутри callback; обычный GC доступен после снятия roots.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:25](../src/standard/text_listeners.hpp#L25) |
<!-- source-links:end -->
