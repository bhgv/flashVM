# Наблюдатели прокрутки TextField

Range notifications от text/current format следуют [edit-транзакции](TEXT_EDIT_EVENTS.ru.md); программные edits не синтезируют onChanged.


Размеры и режимы раскладки теперь поддерживают [события диапазонов](TEXT_GEOMETRY_EVENTS.ru.md). Text/style events расширены в TEXT_EDIT_EVENTS.ru.md; ввод остаётся открытым.


Расширяется [bounded bridge прокрутки](TEXT_SCROLL.ru.md).
[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
описывает порядок field-first, регистрацию listeners и аргумент source field.
Явный offset setter уведомляет об effective изменении scroll/hscroll; размеры и
режимы раскладки расширены в TEXT_GEOMETRY_EVENTS.ru.md. Ввод,
reflow вне поддержанных native setters, HTML/mixed styled fields, полная reference
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

[FVM_TextFieldScrollPlan](../src/standard/text_listeners.hpp#L28) удерживает source и копию порядка регистраций до публикации
обоих offsets. Сначала выполняются conversion и свежее измерение. Ошибка подготовки
allocation/work сохраняет offsets. После публикации вызывается field onScroller,
затем snapshot listeners; аргумент — source field. Missing/noncallable handler
игнорируется. Первая ошибка lookup/callback/work останавливает доставку и передаётся;
commit и эффекты callback сохраняются. Вложенные записи не перезаписываются.
Изменения регистраций влияют на следующие события; handler разрешается перед
вызовом каждого recipient. Unchanged/clamped-same позиция не уведомляет.
Query/path/capture сохраняют no-script контракт. Native accessor barrier
отклоняет collection внутри callback; обычный GC доступен после снятия roots.

## Передача и старые методы

Реестр — обычное сильное ребро графа, включая циклы self-listener. Binary capture
и channel transfer сохраняют регистрации, source identity и offsets без повторной
доставки событий. План текущей доставки на стеке не сериализуется. Поле без
регистраций не получает private registry edge. Публичный C ABI struct и wire bit
нативного Field не расширяются. Новые callback selectors 207/208 требуют нового
reader.

При restore обновляется только retired rootless native callback 205 в методах
TextField addListener/removeListener со сбросом receiver method cache. Отдельно
сохранённый старый Function alias остаётся неподдержанным. Custom methods, roots
и userdata следуют обычному контракту capture; raw host userdata не становится
переносимым благодаря этому изменению.

## Ошибки, исполнение и оставшийся объём

Лимит 4096 записей отклоняет новую уникальную регистрацию с [FVM_ERROR](../include/flashvm/flashvm.h#L43). Проверка
реестра и staging регистрации используют общий work budget. Подготовка recipients
предшествует публикации; lookup и доставка расходуют тот же бюджет события.
Ошибка allocation до публикации сохраняет offsets. Ошибка lookup, VM exception,
C++ exception хоста или work после публикации сохраняет offsets и уже выполненные
эффекты. Доставка останавливается без автоматического retry/replay. Повторная
запись той же effective position не доставляет пропущенный хвост. Эффекты conversion
также сохраняются при последующей ошибке подготовки.

Callback выполняется синхронно на потоке владельца context. Вложенный setter
сохраняет своё итоговое состояние. Порядок recipients копируется, но handler
разрешается перед каждым вызовом: замена следующего handler видна текущей доставке.
Native accessor GC barrier сохраняется: collection внутри callback отклоняется;
между операциями GC обходит регистрации и собирает недостижимые циклы. Worker-тест
использует независимые contexts, а не конкурентный доступ к одной VM. Нативные
scroll query, path export и graph capture не исполняют observers.

Остаются открыты reflow вне поддержанных native setters, onChanged и ввод, Selection/focus, HTML/mixed-style и интеграция текста
SWF display list. Проверки используют аналитическую раскладку и исполняемые AVM1
fixtures, а не binary oracle оригинального Flash Player. Приёмка устройств и
других платформ отдельная; ни одна стадия плана не закрывается.

## Проверки

Наблюдатели явной прокрутки TextField v0.1104–v0.1117: addListener/removeListener хранят ограниченный список identities в managed graph; эффективная запись offsets публикует состояние до field-first onScroller и rooted snapshot listeners. Проверены source argument, duplicate/removal, живая замена handler, вложенные записи и отсутствие события при неизменной позиции. Binary/channel перенос сохраняет регистрации, GC собирает недостижимые циклы; обновляются только retired rootless method slots, а сохранённые Function aliases и host overrides сохраняют тело. SWF4/5 не доставляют эти события; реальные AVM1 setters SWF6–8 совпадают в interpreter/manual/native JIT с проверкой native entry counts. Четыре независимых OS-worker проверяют 128 эпох. Перебор allocation failures и work limits различает rollback до публикации и частичную доставку после commit; ошибки VM/getter/C++ callback сохраняют выполненные эффекты. Прошли 163 выбранные native text/font/graph/tutorial проверки, 17 ASan/UBSan и отдельные no-throw callback/no-fault allocation запуски; LeakSanitizer отключён. SDK-free C89 ABI/admission проходит, три font tutorial явно пропускаются. Переименованный detached text-компонент конфигурируется с bundled static fonts; скопированный observer checker проходит на native executable без отдельной detached-сборки. C89 tutorial проверяет точный порядок callback и итоговые offsets. События всей геометрии, onChanged/ввод, HTML/mixed styles, текст SWF display list, реальные устройства/другие платформы и все девять стадий открыты. [Контракт](TEXT_SCROLL_EVENTS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:28](../src/standard/text_listeners.hpp#L28) |
<!-- source-links:end -->
