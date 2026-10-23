# Видимые строки и bounded-поиск observers

[Транзакции text/edit](TEXT_EDIT_EVENTS.ru.md) расширены на нижнюю видимую строку;
поиск descriptors учитывается до публикации геометрии.
[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#bottomScroll)
описывает read-only bottomScroll и scroll notifications. Аналитическая геометрия
и транзакция ниже — bounded-политика реализации, не binary oracle Flash Player.
Все девять стадий остаются открытыми.

## Триггер пяти свойств

Observed native edit теперь сравнивает scroll, hscroll, maxscroll, maxhscroll и
bottomScroll. Изменение tuple создаёт одно синхронное field-first onScroller,
затем rooted snapshot регистраций. Изменение только bottomScroll тоже учитывается:
например, три помещающиеся строки заменяются одной при неизменных scroll=1,
hscroll=0, maxscroll=1 и maxhscroll=0.

Это относится к поддержанным dimension/layout-policy writes, counted text
assignment, replaceText и current setTextFormat. bottomScroll не становится
доступным для записи. Неизменный five-property tuple, color/decoration-only edits
и future formatting молчаливы. Размер, layout metric или autoSize origin сами
по себе не являются дополнительным триггером. Loaded identical counted text
сохраняет early return/current-format поведение. SWF4/5 остаются молчаливыми;
SWF6–8 и unloaded host contexts поддерживают событие. Программные edits не
синтезируют onChanged.

## Учёт работы без вызова descriptors

[fvm_textfield_observer_present](../src/standard/text_listeners.cpp#L73) — private C++98 helper потока владельца, не новый
публичный C ABI. Он проверяет receiver/lifecycle/version; при status failure
Boolean output остаётся false. До measurement проверяются private registration
edge или потенциальные own/inherited onScroller descriptors и loaded __resolve
без вызова getter, resolver или callback.

Сначала выбирается exact spelling. Folded fallback loaded SWF6 использует прежнее
UTF-16 сравнение и первое совпадающее stored spelling; host/SWF7–8 точные.
Attribute visibility использует тот же charged selection. Exact table lookup,
prototype visit и каждая посещённая запись folded fallback расходуют общий native
execution work. Проверяются context ownership и прежний prototype-depth bound.
Циклы, foreign graph segments, work exhaustion и periodic worker cancellation
не публикуют edit.

Поиск делит budget существующего [FVM_TextFieldScrollPlan](../src/standard/text_listeners.hpp#L28) с candidate preparation
и dispatch, без сброса перед callbacks. Fixed lookup keys могут выделять память;
действует прежний mapping allocation статуса публичного вызова. Считаются посещённые
таблицы/записи, а не каждый байт имени или каждая операция font engine. Noncallable
descriptor и пустой ранее созданный registry также допускают measurement. Без
потенциальных observers сохраняется lazy font/layout admission, но сам поиск
расходует work. Обнаружение потенциального descriptor не гарантирует callable
handler при последующем lookup.

## Публикация и доставка

Old и candidate layouts должны пройти прежний observed-edit admission. Ошибка
подготовки сохраняет старые text/current style/geometry/offsets этой записи;
выполненные conversion effects и построенный source cache могут остаться. После
подготовки content/style, layout, autoSize geometry и clamped offsets публикуются
до lookup handler. Callback видит новый bottomScroll и остальные properties.
Query/path export и graph capture остаются no-script, без синтеза/replay событий.

Recipient identities/order копируются и удерживаются roots; handler разрешается
перед каждым вызовом. Удалённый recipient может получить текущее событие. Вложенный
edit сохраняет итоговые text/style/geometry. Первая ошибка getter/resolver/callback/
work останавливает доставку без rollback публикации и выполненных эффектов.
Автоматического retry/replay нет. Прежний callback GC barrier сохраняется; между
операциями owner GC обходит обычный listener graph.

Публичный struct, wire bit Field, callback selector и graph format не расширяются.
Binary/channel receiver независимо владеет fonts/text/listeners после уничтожения
sender; stack-local notification не переносится. Raw host userdata сохраняет
прежние lifetime/capture ограничения. Worker-тесты используют независимые VM,
а не одновременный доступ к shared context.

## Оставшийся объём

Полное original-player rounding/edit поведение, onChanged/ввод, HTML/mixed styles,
automatic external reflow/font-source rebinding, Selection/focus и текст SWF display
list открыты. Обработка байтов descriptor names и работа font engine не получают
нового metering в этой серии. Устройства и другие платформы требуют отдельной
приёмки. Стадии audio/video/timeline/graphics/JIT/concurrency/GC/debugger открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_textfield_observer_present` | function | [src/standard/text_listeners.cpp:73](../src/standard/text_listeners.cpp#L73) |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:28](../src/standard/text_listeners.hpp#L28) |
<!-- source-links:end -->
