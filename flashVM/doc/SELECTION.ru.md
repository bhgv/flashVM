# Подмножество Selection для MovieClip

v0.592 добавляет явные Selection.getFocus/setFocus для нативных MovieClip в
хост-контексте и SWF6–8. [English](SELECTION.md). Используются общий
[корень клавиатурного выбора](KEYBOARD_FOCUS.ru.md) и существующий resolver путей.
Это ограниченная реализация, а не полная совместимость Selection оригинального Flash.

## Установка

`fvm_standard_selection_install(context)` принимает заимствованный ненулевой живой
контекст с уже установленными через [`fvm_standard_install`](../include/flashvm/standard.h#L20) стандартными объектами.
Вызывать в потоке-владельце VM, вне уничтожения и сборки/освобождения GC.
Синхронно готовит namespace с getFocus/setFocus/addListener/removeListener и
accessor-снимком _listeners и атомарно публикует
глобальный Selection. Скрипт не запускается, фокус не меняется, устройства не
открываются. Установка Key независима.

[`FVM_OK`](../include/flashvm/flashvm.h#L42) — успешная публикация или целая прежняя установка;
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) — неверный контекст/состояние, отсутствие стандартной
библиотеки либо конфликт глобального namespace/descriptor/callable;
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) — загруженный SWF ниже [`FVM_SELECTION_MIN_SWF_VERSION`](../include/flashvm/keyboard.h#L5) (6);
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) — отказ памяти; [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) — перехваченное непредвиденное
исключение C++. Любой отказ подготовки сохраняет globals и прежний фокус.
Неопубликованные временные объекты остаются обычным мусором контекста.
Повторная установка проверяет нативные callbacks, флаги доверенного входа и
затенение методов; произвольные скриптовые attributes не восстанавливает.

SWF6 ищет глобальные имена без учёта регистра; SWF7–8 и хост — с учётом.
Поэтому конфликтующее `selection` блокирует установку только в SWF6. Методы
изначально неперечисляемые. Одна установка стандартной библиотеки не включает
это подмножество. SDL-плеер устанавливает его для SWF6–8; старые ролики остаются
проигрываемыми без namespace. Если хост установил Selection заранее, затем
загрузил SWF1–5, оба метода возвращают [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) с сохранением фокуса.
Исторический Selection только для текстовых полей здесь не реализован.

## setFocus

`Selection.setFocus(newFocus)` возвращает Boolean. Нет аргументов, null либо
undefined — снять фокус и вернуть true; лишние аргументы игнорируются. Иначе
поддержаны объект нативного MovieClip того же контекста либо counted-строка пути.
Точечные/слэш-пути разрешаются от активного AVM1 target, с переходом к базовому
клипу активации, если target отсутствует. `_root`/`_level0`, counted Unicode,
версионный поиск свойств и `_lockroot` используют существующий resolver. В обычном
C-вызове без активной активации ролика путь не разрешается: передавайте объект.
Произвольного приведения объектов/чисел к строке нет.

Клип допустим, если focusEnabled преобразуется в true существующим версионным
AVM1 ToBoolean либо имеется вызываемый onDragOut, onDragOver, onPress, onRelease,
onReleaseOutside, onRollOut или onRollOver. Проверка идёт в этом порядке до первого
вызываемого члена. Наследуемые свойства, accessors и загруженный `__resolve`
используют обычный поиск. Нативный метод доступен через first-class обёртку
Function. Невызываемое значение не делает клип допустимым. tabEnabled, видимость
и _focusrect в программном выборе не участвуют. Допустимость проверяется только
при попытке выбора; позднейшая смена флага/обработчиков сама не снимает уже
существующий выбор.

Нет пути, неподдержанный scalar/объект, прототип класса, недопустимый или удалённый
клип — Boolean false без публикации кандидата. Побочные эффекты поиска/getter
не откатываются. Закрытое нативное состояние проверяется
до чтения допустимости и перед публикацией: getter мог удалить клип.
[`FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS`](../src/standard/movieclip_constants.h#L66) ограничивает обход предков, resolver путей
сохраняет собственные пределы. Повреждённое состояние и настоящие ошибки
пути/getter/resolver передаются статусом VM, а не подменяются штатным false.
Отказ выделения памяти возвращает [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44).

Кандидат, receiver и аргументы защищены корнями при вложенном скрипте. Доверенные
входы Selection допускают GC внутри скриптовых getters; внешний барьер обычного
native-callback остаётся в силе. Getter может сменить фокус, свойства, удалить
клип или бросить исключение. Эти эффекты сохраняются при неудаче внешнего выбора.
При успехе внешняя попытка заменяет выбор, сделанный вложенным вызовом. Отката
и очереди уведомлений о смене фокуса нет. Успешная смена отправляет синхронные
уведомления, описанные ниже. Последующие переходы через [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L82) доставляются
выбранному клипу перед независимыми слушателями Key по общей политике хоста.

## Уведомления фокуса

Начиная с v0.593 успешная смена идентичности сначала публикует новый выбор,
затем вызывает `oldClip.onKillFocus(newClip)` и `newClip.onSetFocus(oldClip)`.
Отсутствующий клип представлен null; отсутствующий receiver не вызывается.
Повторный выбор того же клипа, отказ допустимости, C setter хоста и ленивое
снятие удалённого выбора уведомлений не отправляют. Обычный версионный поиск
поддерживает наследование и getters; receiver — соответствующий клип.
Отсутствующий/невызываемый обработчик пропускается, глобальной подстановки нет.
Возвращённое обработчиком значение игнорируется.

Оба клипа защищены корнями до публикации и до конца синхронного уведомления.
Скриптовые getters/обработчики могут вызывать GC; обычные native-барьеры сохраняются.
Вложенная смена фокуса остаётся текущей; внешний вызов заканчивает уведомление
своих первоначальных клипов. Поиск обработчика происходит при доставке, поэтому
первый callback может заменить второй. Удаление клипа не отменяет уведомление
снимка; следующий getFocus применит обычную проверку удаления. Для рекурсивных
скриптовых вызовов действуют существующие ограничения глубины VM.

Настоящая ошибка getter/обработчика, включая OOM и [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45),
передаётся вызывающему коду и прерывает дальнейшую доставку. Уже опубликованный
выбор и побочные эффекты не откатываются. OOM до публикации сохраняет старый
фокус; OOM при доставке сохраняет новый, если callback не сменил его снова.
Ошибка callback не превращается в штатный Boolean false. Очереди и отката нет.
Это явная политика подмножества; сравнение граничного порядка с оригиналом открыто.

## Слушатели Selection

С v0.594 `Selection.addListener(object)` возвращает undefined и один раз удерживает
объект того же контекста. Дубликат не меняет порядок. Отсутствующий аргумент,
null, undefined и примитивы игнорируются. Проверка аргументов VM до входа отвергает
объекты другого контекста с [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). OOM сохраняет регистрации;
предел [FVM_SELECTION_MAX_LISTENERS](../src/standard/selection_listeners.hpp#L6) (4096) возвращает [FVM_ERROR](../include/flashvm/flashvm.h#L43). Регистрация
не вызывает скрипт и не открывает устройства. `Selection.removeListener(object)`
возвращает true только при удалении идентичности, иначе false. Само удаление
не выделяет память, но вызывающий dispatcher может выделять снимок аргументов.

До публикации реальной смены выбора создаётся защищённый GC-корнями снимок
регистраций. OOM подготовки сохраняет прежний выбор. После уведомлений клипов
каждый получатель снимка получает `listener.onSetFocus(oldClip, newClip)` в порядке
регистрации; концы представлены объектом или null, receiver — сам слушатель.
Обработчик ищется один раз при доставке. Отсутствующий/невызываемый пропускается
без глобальной подстановки, настоящая ошибка прекращает доставку и передаётся
без отката уже опубликованного выбора. Вложенная смена синхронная, со своим
снимком и сохранением своего результата. Повторный выбор того же клипа,
недопустимый кандидат, C setter и ленивое снятие удалённого выбора не уведомляют.

Добавление/удаление подписки внутри callback действует на следующие переходы.
Удаление будущего получателя с последующим GC не отменяет уведомление текущего
снимка. Изменение его обработчика влияет на позднейшую доставку. Регистрации
сильные, принадлежат контексту, независимы от Key и замены глобального Selection.
Они живут до явного удаления или уничтожения контекста, не синхронизируются и не
сериализуются. Передача обычного объекта не подписывает его в другом контексте.

`Selection._listeners` возвращает новый нативный Array с текущими объектами,
без заменяемого конструктора Array. Accessor не перечисляется, не удаляется и
не имеет setter. Изменение Array не меняет подписки; удерживаемый результат
нужно защитить корнями перед собирающим вызовом. OOM сохраняет регистрации.
Это снимок для просмотра, а не изменяемый broadcaster-массив оригинального Flash.
Повторная установка проверяет все четыре метода и неподменённый accessor.

Базовые сигнатуры соответствуют справочнику Adobe AS2 по ссылке ниже. Порядок
вложенных уведомлений, снимки, пределы и замена namespace — явная политика
подмножества; сравнение с оригинальным плеером остаётся открытым.

## getFocus

`Selection.getFocus()` игнорирует аргументы. Возвращает null без выбора либо
counted-путь из нативной сцены, например `_level0.panel`. Удалённость клипа или
физического предка лениво снимает общий выбор и даёт null. Заменяемые скриптом
clip.toString/_target не вызываются. Действуют нативные правила имён, уровней,
Unicode и ограничения обхода.

Строка заимствована у контекста: использовать до следующего выделяющего вызова
либо удерживать обычными API корней значений/строк. Владение объектами не
передаётся. Ошибки проверки/построения пути, включая OOM, передаются обычными
статусами/исключениями VM. Успешная проверка удаления может снять устаревший
фокус до ошибки последующей операции.

## Ограничения и взаимодействие

Selection и C API хоста разделяют один сильный корень контекста. Хост обходит
скриптовую допустимость; getFocus всё равно описывает выбранный им клип. Выбор
живёт до сброса/замены, ленивой проверки удаления или уничтожения контекста,
включая загрузку другого ролика и замену namespace. Синхронизации нет. Состояние
локально для хоста и не сериализуется; методы namespace не имеют переносимых
serialization callback IDs, передача графа возвращает [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).

Открыты TextField/каретка/диапазоны, Button, рамки фокуса, автоматический выбор мышью/Tab, IME, security origins,
SWF ClipActions и сравнение граничных случаев/порядка с оригинальным плеером.
Точечный формат getFocus и требование вызываемого кнопочного обработчика —
описанная политика подмножества; полная эталонная приёмка ещё не выполнена.
Исходные правила: справочник Adobe AS2
([Selection](https://open-flash.github.io/mirrors/as2-language-reference/Selection.html),
[MovieClip](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html)).

## Примеры и проверки

- [Исполняемый C89-пример](../examples/embedding/selection/README.md): отказ по
  умолчанию, focusEnabled, путь, сброс и явное управление временем жизни.
- [Интерактивный SWF](../examples/player/animation_audio/README.md): выбирает корень
  путём `_root`, независимо считает события клипа/Key, увеличивает высоту синего
  квадрата при отпусканиях, доставленных выбранному корню.
- `tests/selection_listeners.cpp`: идентичность, снимок, мутации с GC, ошибки,
  замена namespace и изоляция контекстов.
- `tests/selection_listeners_allocation.cpp`: OOM регистрации/снимка/публикации.
- `tests/selection.cpp`: Unicode-пути, допустимость, неверные типы, нативные getters,
  реальные ошибки, удаление при проверке и ленивый сброс по удалённому предку.
- `tests/selection_install_allocation.cpp`: fresh-context перебор отказов установки
  и setFocus с проверкой границы публикации/уведомлений.
- `tests/jit_selection.cpp`: SWF6–8, interpreter/ручной/автоматический JIT,
  скриптовые getters с настоящим GC, версионные конфликты globals, callbacks.
- `frontend/tests/keyboard_movie.cpp`: настоящий поставляемый SWF, три цикла,
  короткие нажатия до кадра, совпадение независимых счётчиков в обоих режимах.

Тесты VM/хоста и кросс-сборки не равны приёмке физической клавиатуры/устройств.
См. [текущее состояние роудмапа](ROADMAP_STATUS.md).

Implementation / Реализация: [`fvm_selection_set_focus`](../src/standard/selection.cpp#L128), [`fvm_selection_get_focus`](../src/standard/selection.cpp#L178)
in [src/standard/selection.cpp](../src/standard/selection.cpp).


## v0.618 shared event budget / Общий бюджет событий

Registration, inspection and delivery share nested AVM1 work limits. Preparation
failure preserves valid state; later failure keeps committed input and callbacks.
[EN contract](INPUT_EVENT_BUDGET.md) / [RU](INPUT_EVENT_BUDGET.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../include/flashvm/keyboard.h#L82) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:66](../src/standard/movieclip_constants.h#L66) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_selection_get_focus` | function | [src/standard/selection.cpp:178](../src/standard/selection.cpp#L178) |
| `FVM_SELECTION_MAX_LISTENERS` | macro / define | [src/standard/selection_listeners.hpp:6](../src/standard/selection_listeners.hpp#L6) |
| `FVM_SELECTION_MIN_SWF_VERSION` | macro / define | [include/flashvm/keyboard.h:5](../include/flashvm/keyboard.h#L5) |
| `fvm_selection_set_focus` | function | [src/standard/selection.cpp:128](../src/standard/selection.cpp#L128) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
