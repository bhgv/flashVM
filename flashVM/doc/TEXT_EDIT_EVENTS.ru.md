# События прокрутки при изменениях текста и формата

[Транзакции геометрии](TEXT_GEOMETRY_EVENTS.ru.md) расширены на присваивание text,
replaceText и текущий setTextFormat host-created одностильного TextField.
[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
различает программные scroll notifications и пользовательский onChanged.
Ограниченные триггеры и admission ниже не являются binary oracle оригинального
Flash Player. Все девять стадий плана остаются открытыми.

## Триггеры и версии

Транзакция сравнивает effective scroll, hscroll, maxscroll и maxhscroll. Изменение
tuple создаёт одно field-first onScroller, затем rooted snapshot listeners с
аргументом настоящего source field. При неизменном tuple текст/формат публикуется
без события. Изменения только bottomScroll, intrinsic metrics, autoSize размеров
или origin сами по себе не являются триггером. Программные edits не синтезируют
onChanged.

Поддержаны SWF6–8 и unloaded host contexts. Loaded SWF4/5 сохраняют прежнее
молчаливое поведение. Присваивание идентичного counted text в loaded SWF сохраняет
current format без применения future style и без уведомления. Unloaded same-text
assignment применяет future style и может изменить диапазоны. setNewTextFormat
меняет только future formatting, не измеряет current geometry и не уведомляет.

## Публикация текста и стиля

[GeometryNotification](../src/standard/text.cpp#L642) независимо измеряет proposed
counted content и uniform style. Source geometry читается после обычной проверки/
conversion аргументов; явные эффекты conversion сохраняются. Potential own/
inherited/folded handlers, registration edges и loaded __resolve следуют admission
из TEXT_GEOMETRY_EVENTS.ru.md. Getter/resolver/callback не вызывается на подготовке.

Observed edit должен допустить old и proposed layout. Ошибка allocation, work,
font/layout или registry preparation сохраняет прежние content/current style/
размеры/offsets этой записи. Успешно построенный source cache и прежние явные
conversion effects могут остаться. Без observers admission остаётся lazy.
Регистрация потенциального handler может перенести отложенную layout error на
edit, включая ошибку старого source layout.

Text assignment использует counted UTF-8 и прежнюю политику future style.
replaceText сохраняет ограничения UTF-16 boundaries/count: неверные диапазоны
и граница внутри surrogate pair отклоняются. Для непустого source сохраняется
current uniform style; вставка в пустое поле использует future style. Joined bytes
и roots recipients готовятся до adoption нового owned replacement storage.
Ошибка allocation/adoption не публикует частичную замену. Старый aliased counted
string остаётся действительным snapshot. Конец диапазона не подрезается молча
под более короткий текст следующего вызова.

Current setTextFormat объединяет поддержанные uniform properties в candidate:
font selection, size, kerning, leading и поддержанные paragraph/layout policies.
Прежние font/metric constraints проверяются до notification preparation.
Color/underline-only изменения молчаливы при одинаковых диапазонах. Mixed style
spans и неподдержанные formats сохраняют прежние ошибки.

Все fallible операции идут до публикации. Content/current style публикуются
вместе с candidate dimensions, autoSize anchor/origin, owned measured layout и
clamped offsets до dispatch. Handler видит новый текст и метрики. Публичный C ABI
struct, native callback ID и wire bit Field не расширяются.

## Ошибки доставки, reentry и владение

Сохраняются field-first порядок, snapshot identities и живой lookup handler.
Первая ошибка getter/resolver/callback/work передаётся и останавливает хвост,
сохраняя опубликованное состояние и выполненные эффекты. Автоматического
retry/replay нет. Вложенная запись text/style/geometry остаётся итоговой; внешний
edit не публикует свой старый candidate повторно после возврата callback.
Удалённый listener остаётся в текущем rooted snapshot; следующие события
используют изменённые регистрации.

Подготовка и dispatch используют общий execution budget. Native GC barrier
отклоняет collection внутри callback; roots/nesting снимаются при успехе,
allocation/work errors и VM/C++ exceptions. Между операциями owner GC обходит
обычный listener graph и собирает недостижимые циклы. Query/path export и binary/
channel capture остаются no-script, события не повторяются. Receiver независимо
владеет fonts, text/layouts и listeners после уничтожения sender. Raw host userdata
сохраняет прежние lifetime/capture ограничения. Worker-проверки используют разные
VM, а не одновременный доступ к одному context.

## Оставшийся объём

Полная bottomScroll/reference rounding семантика, onChanged/ввод, Selection/focus,
HTML/mixed styles, автоматическая перепривязка font sources и текст SWF display
list остаются открытыми. Автоматический reflow вне поддержанных native setters
не покрывается. Реальные устройства и другие платформы требуют отдельной приёмки.
Серия не закрывает стадии audio, video, timeline, graphics, JIT, concurrency, GC
или debugger.
## Проверки

Наблюдатели прокрутки при content/current-format edits v0.1131–v0.1145: counted text assignment, replaceText и setTextFormat сравнивают proposed content/style geometry до публикации. Изменения effective offsets/maxima уведомляют field-first и rooted snapshot listeners после общего commit текста/стиля/layout/autoSize geometry и clamped offsets. Unchanged tuple, future formatting и decoration-only изменения молчаливы; loaded identical counted text сохраняет current formatting. replaceText сохраняет строгие UTF-16 boundaries и owned/aliased snapshot семантику. Прошли 185 выбранных native text/font/graph/tutorial проверок, десять focused ASan/UBSan и отдельные no-throw callback/no-fault allocation запуски; LeakSanitizer отключён. Обычная сборка перебирает allocation/work failures для трёх операций, различает rollback и committed partial delivery, проверяет ошибки VM/getter/C++. Reentrant edits сохраняют nested content/style и roots удалённых recipients. Двадцать семь SWF6–8 edit/execution комбинаций исполняют настоящие AVM1 SetMember/CallMethod в interpreter/manual/native JIT с native entry counts. Четыре независимых OS-worker выполняют 128 text/layout/GC эпох; binary/channel receivers сохраняют listener/font ownership после уничтожения sender. SWF4/5 остаются молчаливыми, программный onChanged не синтезируется, autoSize center anchors сохраняются при range-neutral edits. SDK-free C89 ABI/admission проходит; три font tutorial явно пропускаются. Переименованный detached-компонент конфигурируется с bundled static fonts; его скопированный C89 checker проходит на native executable без отдельной detached-сборки. C89 trace: 1 2 1 2 1 1 1 1 1 1, итог scroll=1/hscroll=0, maxima=1/40. Полная bottomScroll/reference rounding семантика, ввод/onChanged, HTML/mixed styles, automatic external reflow/font-source rebinding, текст SWF display list, устройства/другие платформы и все девять стадий остаются открытыми. [Контракт](TEXT_EDIT_EVENTS.ru.md).
