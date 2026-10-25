# Транзакции событий геометрии TextField

Изменения нижней видимой строки уведомляют даже при неизменных offsets/maxima; поиск observers учитывается: [контракт](TEXT_VISIBLE_EVENTS.ru.md).


Text assignment, replaceText и current TextFormat расширяют транзакцию: [контракт edits](TEXT_EDIT_EVENTS.ru.md). Future style остаётся молчаливым; ввод открыт.


Расширяются [наблюдатели прокрутки](TEXT_SCROLL_EVENTS.ru.md) для host-created
одностильного TextField. [Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
описывает уведомления при изменении scroll properties. Точная bounded-геометрия
и синхронная транзакция ниже — политика реализации, не binary oracle оригинального
Flash Player. Все девять стадий плана остаются открытыми.

## Триггеры и допуск

Записи _width, _height, multiline, wordWrap и autoSize сравнивают effective
scroll, hscroll, maxscroll, maxhscroll и bottomScroll до и после candidate layout. Изменение
любого элемента создаёт одно событие, даже если меняется только максимум.
Неизменный tuple, parent _x/_y и textColor не уведомляют. Изменение только line spacing или clip rectangle здесь не является триггером;
bottomScroll учитывается по TEXT_VISIBLE_EVENTS.ru.md.
Явные scroll/hscroll setters сохраняют прежний контракт.

[GeometryNotification](../src/standard/text.cpp#L640) обнаруживает потенциальный handler по native property/
method/accessor descriptors в prototype chain, учитывая folding SWF6 и version
visibility. Private registration edge или loaded __resolve descriptor также
включают observed path. Обнаружение не вызывает getter, resolver или callback.
Без потенциальных observers сохраняется lazy measurement/admission. Даже пустой
ранее созданный registry или noncallable descriptor включает измерение. SWF4/5
сохраняют молчаливую геометрию; SWF6–8 и unloaded host contexts поддерживают
новые события. Loaded resolver следует обычному optional method dispatch.

## Подготовка, публикация, доставка

Сначала выполняется numeric conversion, включая пользовательский код; затем
подготовка использует свежую source geometry. Candidate независимо владеет копией
style и измеренной layout, пока прежние размеры/layout/offsets остаются активными.
Observed edit должен допустить source и candidate layout; ошибки font/layout,
allocation/work передаются без публикации этой записи. Успешно построенный source
cache и явные эффекты conversion могут сохраниться. Без observers сохраняется
lazy-поведение: регистрация handler может перенести отложенную layout error
на geometry setter.

При изменении tuple snapshot recipients готовится до публикации. Успешный commit
без allocation публикует размеры, режимы раскладки, autoSize anchor/origin,
измеренный cache и clamped offsets вместе. Сначала вызывается field onScroller,
затем сохранённые listener identities в порядке регистрации; аргумент — настоящий
source field. Handler разрешается перед каждым вызовом. Внутри callback уже видны
новая геометрия и диапазоны.

Первая ошибка getter/resolver/callback/work останавливает доставку без rollback
геометрии и выполненных эффектов. C++ exception хоста следует обычному mapping
статусов. Автоматического retry/replay нет. Вложенные setters сохраняют итоговое
состояние: внешний callback не перезаписывает его при возврате. Удаление recipient
влияет на следующие события, но текущий snapshot остаётся rooted. Обнаружение,
подготовка и доставка используют общий execution budget.

## Владение, GC и передача

Публичный C ABI layout и native callback selectors не расширяются. Нативный
[fvm_textfield_scroll_info](../include/flashvm/standard_text.h#L80), path/capture остаются no-script и не синтезируют события.
Layouts независимо принадлежат владельцам и освобождаются при ошибке. Без
observers cache может оставаться lazy; измеренный candidate cache сохраняется
после успешной публикации геометрии.

Callback синхронный, на потоке владельца context, с прежним native GC barrier.
Collection внутри callback отклоняется; roots/execution depth снимаются при
успехе, VM/C++ exceptions, allocation/work error. Между вызовами GC обходит
обычный listener graph. Binary/channel переносит состояние и регистрации, но
не текущий план на стеке и не историю уведомлений. Receiver независимо владеет
fonts/layout после уничтожения sender. Raw host userdata сохраняет обычный
контракт lifetime/capture. Thread-тесты используют разные contexts, а не
конкурентный доступ к одной VM.

## Оставшийся объём

Text assignment, replaceText и current TextFormat теперь следуют TEXT_EDIT_EVENTS.ru.md;
future formatting само по себе не уведомляет. Полная bottomScroll/reference rounding
семантика, onChanged/ввод, Selection/focus, HTML/mixed styles, автоматическая
перепривязка font sources и текст SWF display list открыты. Отклоняемые multiline/
automatic dimension операции сохраняют прежние ограничения. Устройства и другие
платформы проверяются отдельно; все девять стадий остаются открытыми.

## Проверки

Наблюдатели размеров/раскладки TextField v0.1118–v0.1130: _width/_height, multiline, wordWrap и autoSize сравнивают effective offsets и maxima через независимо измеренный candidate. Потенциальные descriptors, inherited handlers, folding/visibility SWF6 и loaded __resolve обнаруживаются без script. Без потенциальных observers сохраняется lazy admission; observed layout errors возникают до публикации. Commit публикует измеренную геометрию и clamped positions до field-first rooted snapshot delivery; вложенные записи сохраняют итоговое состояние. События от text/style/fonts и изменения только bottomScroll вне этого триггера. Прошли 174 выбранные native text/font/graph/tutorial проверки, десять focused ASan/UBSan и отдельные no-throw callback/no-fault allocation запуски; LeakSanitizer отключён. Обычная сборка перебирает allocation/work failures до/после публикации и ошибки VM/getter/C++. Реальные AVM1 SetMember SWF6–8 совпадают в interpreter/manual/native JIT с native entry counts; dynamic resolver вызывается один раз после публикации. Четыре независимых OS-worker проверяют 128 geometry/GC эпох. Binary/channel receivers сохраняют fonts/listeners после уничтожения sender. SDK-free C89 ABI/admission проходит, три font tutorial явно пропускаются. Переименованный detached-компонент конфигурируется с bundled static fonts; его скопированный C89 checker проходит на native executable без отдельной detached-сборки. Точный callback trace C89: 1 2 1 2 1 1 1, итог scroll=1/hscroll=0. Ввод/onChanged, события text/style, HTML/mixed styles, текст SWF display list, устройства/другие платформы и все девять стадий остаются открытыми. [Контракт](TEXT_GEOMETRY_EVENTS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:80](../include/flashvm/standard_text.h#L80) |
<!-- source-links:end -->
