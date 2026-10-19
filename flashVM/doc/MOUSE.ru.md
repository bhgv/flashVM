# Слушатели первичного указателя Mouse

v0.595 добавляет явное подмножество Mouse для SWF6–8 и C89-интерфейс хоста.
Хост передаёт первичное касание, перо или мышь; ядро не открывает устройство,
окно и не запрашивает разрешений. Физическая мышь не требуется. [English](MOUSE.md).


SDL-мост плеера v0.596 передаёт первичный палец в эти события с сохранением
порядка клавиш/указателя; см. [ввод плеера](PLAYER_POINTER.ru.md).

## Установка

`fvm_standard_mouse_install(context)` принимает живой заимствованный контекст с
установленными стандартными объектами. Вызов в потоке-владельце, вне GC и
уничтожения. Атомарно готовятся Mouse.addListener/removeListener и _listeners.
Key/Selection не устанавливаются, координаты не меняются, скрипт не выполняется.

[FVM_OK](../include/flashvm/flashvm.h#L42) означает успешную публикацию или целую прежнюю установку.
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) — неверный контекст/состояние, отсутствие стандартных
объектов или конфликт глобального descriptor/callable/идентичности методов.
Загруженный SWF ниже [FVM_MOUSE_MIN_SWF_VERSION](../include/flashvm/mouse.h#L5) (6) даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46);
хост-контекст поддерживается. [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) и пойманный неожиданный [FVM_ERROR](../include/flashvm/flashvm.h#L43)
сохраняют прежний global. Промежуточные объекты остаются мусором контекста.
В SWF6 имена глобалов нечувствительны к регистру, в SWF7–8/host чувствительны.
Обычная установка стандартных объектов не включает Mouse автоматически.
Методы/getter также отвергают SWF1–5 после установки до загрузки ролика.

## Нативный контракт

- `fvm_input_add_mouse_listener(context, listener)` удерживает ненулевой объект
  того же контекста один раз. Дубликат даёт [FVM_OK](../include/flashvm/flashvm.h#L42) без изменения порядка.
  Предел [FVM_MOUSE_MAX_LISTENERS](../include/flashvm/mouse.h#L6) (1024) даёт [FVM_ERROR](../include/flashvm/flashvm.h#L43), отказ памяти —
  [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) без регистрации. Обработчики не вызываются.
- `fvm_input_remove_mouse_listener(context, listener, removed)` требует объект
  того же контекста и ненулевой int output. [FVM_OK](../include/flashvm/flashvm.h#L42) записывает 1 при удалении,
  0 при отсутствии. [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) сохраняет output. Само удаление
  не выделяет память и не запускает скрипт.
- `fvm_input_dispatch_mouse(context, type, world_x, world_y)` синхронно доставляет
  [FVM_MOUSE_EVENT_DOWN](../include/flashvm/mouse.h#L14), [FVM_MOUSE_EVENT_MOVE](../include/flashvm/mouse.h#L13) или [FVM_MOUSE_EVENT_UP](../include/flashvm/mouse.h#L15). Координаты —
  конечные stage/world пиксели до обратного преобразования MovieClip. Неверный
  тип, NaN/infinity, неверное состояние и вложенная доставка Mouse дают
  [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) без изменения позиции/уведомлений. Хост выбирает первичный
  контакт; VM не выводит удержание, не подавляет дубликаты и не распознаёт жесты.
- `fvm_input_get_pointer(context, x, y)` копирует позицию без выделения памяти
  или обращения к устройству; outputs по прежнему контракту ненулевые и разные.
  MovieClip._xmouse/_ymouse и AVM1 читают это состояние. [`fvm_input_set_pointer`](../include/flashvm/input.h#L72)
  остаётся операцией публикации без уведомлений.

Все вызовы — в потоке VM, вне сборки/освобождения GC и уничтожения.
C-регистрация/доставка независимы от установки скриптового namespace. Владение
managed-объектами не передаётся хосту; снятие подписки или уничтожение контекста
снимает корень. Заимствованные код/userdata должны жить до окончания регистраций
и активных доставок. Уничтожать контекст из callback запрещено.

## Доставка

Сначала создаётся защищённый GC-корнями снимок получателей. OOM подготовки
сохраняет позицию и drag-состояние. Затем публикуются обе координаты, обновляется
существующее нативное перетаскивание, получатели по порядку получают
onMouseDown/onMouseMove/onMouseUp без аргументов. Обработчик видит новую позицию;
выбранный фокус или результат hit test не требуются.

Поиск метода обычный: версионный регистр, наследование, accessors, script/native,
один поиск на доставку с правильным receiver. Нет/невызываемый обработчик
пропускается без глобальной подстановки, возвращённое значение игнорируется.
Скрипт может вызывать GC; native-callback сохраняет обычный барьер, кроме
внутреннего доверенного входа с собственными корнями. Снимок удерживает будущего
получателя после удаления подписки и GC. Изменение подписок действует на
следующее событие, замена метода — на его последующую доставку. Вложенная
доставка Mouse отвергается; прочие операции следуют своим контрактам.

Настоящая ошибка drag/getter/обработчика прерывает дальнейшую доставку и
передаётся исходным статусом/исключением VM. После публикации OOM и настоящие
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46)/FVM_INVALID_ARGUMENT сохраняют новые координаты; побочные эффекты
не откатываются. Если callback сменил координаты, они остаются текущими: внешний
вызов не перепубликует старый снимок. Временные корни и guard восстанавливаются
при успехе, ошибке и C++ unwind.

Старый gesture [`fvm_input_dispatch`](../include/flashvm/input.h#L82) и его C callback независимы: они не вызывают
Mouse автоматически; Mouse не создаёт gesture callback. Хост выбирает нужный
маршрут, избегая двойной доставки.

## Скриптовые методы и просмотр

`Mouse.addListener(object)` возвращает undefined; нет аргумента/null/undefined/
примитив игнорируются. Ошибки принадлежности контексту, памяти и предела
передаются. `Mouse.removeListener(object)` возвращает true только при удалении,
иначе false для отсутствующего/недопустимого аргумента. Внешний dispatcher
может выделять снимок аргументов, хотя нативное удаление памяти не требует.

`Mouse._listeners` — неперечисляемый, неудаляемый getter без setter; возвращает
новый нативный Array с текущими объектами. Подмена конструктора Array не
запускает скрипт. Изменение Array не меняет подписки. Удерживаемый результат
защищается обычными корнями managed-значений перед GC/выделениями, когда это
требуется его контрактом. OOM getter сохраняет регистрации. Это снимок для
просмотра, а не изменяемый broadcaster-массив оригинального плеера.

Сильные регистрации принадлежат контексту, независимы от Key/Selection и замены
глобального Mouse. Синхронизации и сериализации подписок нет; перенос объекта
не подписывает его в получателе. Методы namespace не имеют переносимого callback codec.

## Проверки и границы

[C89-пример](../examples/embedding/mouse_listeners/README.md) передаёт один контакт
без OS-устройства. `tests/mouse_listeners.cpp`: идентичность, мутации с GC,
защита от вложенной доставки/уничтожения, drag до callback, ошибки, неверные
координаты, пропуск обработчиков и изоляция контекстов. `tests/mouse_allocation.cpp`:
OOM установки, до/после публикации. `tests/jit_mouse.cpp`: SWF6–8, настоящие
скриптовые регистрация/обработчики, interpreter/manual/automatic JIT, GC и
настоящее AVM1-исключение обработчика.

Открыты hide/show курсора, колесо, прямая рассылка MovieClip/Button/ClipActions,
автоматический выбор фокуса/цели, сырые мультитач/жесты, security origins,
граничный порядок оригинала и нативная/device/endurance приёмка. Базовые
сигнатуры соответствуют [справочнику Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html).
Снимки, пределы и явный mapping хоста — политика подмножества, а не полная
Flash-совместимость. Ни один этап не закрывается; см. [состояние](ROADMAP_STATUS.md).

## Колесо прокрутки

С v0.605 [fvm_input_dispatch_mouse_wheel](../include/flashvm/mouse.h#L32)(context, delta, world_x, world_y) принимает
явное событие колеса. `context` — живой контекст вызывающего потока вне GC,
освобождения и разрушения. `delta` — конечный Number в **строках прокрутки,
нормализованных хостом**: плюс вверх, минус вниз. Дробная часть и явно переданный
ноль сохраняются. `world_x`, `world_y` — конечные пиксели мира/сцены на момент
события, не координаты экрана или виджета. Указатели и значения заимствуются
только на время синхронного вызова; отдельное устройство VM не открывает.

Сначала копируются и укореняются те же регистрации Mouse, что у down/move/up.
После успешной подготовки публикуются координаты, обновляется активный drag,
затем по порядку регистрации вызывается необязательный onMouseWheel(delta).
Передаётся ровно один числовой аргумент. Автоматических scrollTarget, hit test,
изменения фокуса/кнопки и прокрутки содержимого нет. Отсутствующий или невызываемый
метод игнорируется без обращения к глобальной функции. Для прямой C-доставки
установка пространства Mouse не обязательна; SWF6–8 использует тот же список.

[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) означает неверный контекст/состояние, NaN/бесконечность в
дельте/координатах или вложенную доставку Mouse. У колеса и down/move/up общая
защита: нельзя рекурсивно вызвать ни свой, ни другой вид события. Ошибочный ввод
и OOM снимка сохраняют координаты/drag, обработчиков не вызывают. После публикации
ошибка drag/getter/метода или исключение VM прекращает дальнейшее уведомление и
сохраняет совершённые изменения. OOM поиска метода тоже может случиться после
публикации. Неожиданное нативное исключение ограничивается [FVM_ERROR](../include/flashvm/flashvm.h#L43). Временные
корни и защита снимаются на любом выходе. Удаление регистрации и GC из раннего
обработчика не уничтожают ожидающих получателей снимка. Изменения подписок
влияют на следующие события; метод ожидающего получателя ищется при его вызове.
Не разрушайте контекст внутри обработчика; доставка блокирует такое разрушение.

Хост сам переводит щелчки, строки, пиксели и высокоточные отсчёты в строки.
Нельзя молча выдавать пиксельную дельту за строки. VM не округляет дробную часть,
не копит остаток, не синтезирует MouseMove и не добавляет аргумент объекта-цели.
[Полный C89 пример](../examples/embedding/mouse_wheel/README.md).

[Копия справочника Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html)
описывает знаковую дельту строк и необязательную цель; историческая поддержка
колеса была привязана к Windows. Здесь явный переносимый контракт хоста только
для delta. Автоматическая цель, точный порядок событий старого проигрывателя,
физические колёса/тачпады и все ОС этой реализацией не приняты.

## Запрос видимости курсора

С v0.608 контекст хранит желаемую видимость, изначально 1. VM не владеет нативным
курсором. Сенсорный/headless хост может хранить запрос, не создавая физическую
мышь или курсор. Положение указателя, кнопка/фокус, очередь событий, подписки
и исполнение скрипта не зависят от видимости.

[fvm_input_get_cursor_visible](../include/flashvm/mouse.h#L37)(context, visible_output) требует живой контекст
потока владельца вне GC/освобождения/разрушения и непустой выходной int-указатель.
[FVM_OK](../include/flashvm/flashvm.h#L42) записывает 0/1; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) сохраняет выход. Это запрос VM,
не проверка существования курсора ОС или успешного применения к экрану.

[fvm_input_set_cursor_visible](../include/flashvm/mouse.h#L42)(context, visible, previous_output) принимает только
visible=0 или 1, previous_output обязателен. [FVM_OK](../include/flashvm/flashvm.h#L42) меняет запрос и записывает
прежнее 0/1, в том числе при повторном hide/show. Неверные контекст, состояние,
значение или выход дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) без изменения запроса и выхода.
Обе функции не выделяют память, не вызывают скрипт/хост/ОС, не дают OOM или
ошибку устройства. Их можно вызвать из обработчика Mouse на том же потоке;
рекурсивной доставки событий нет. Скаляр не требует корня GC. Значение живёт
весь срок контекста и сохраняется при загрузке фильма; хост может явно включить
видимость при смене фильма.

Оконный хост читает запрос после скрипта/ввода и применяет к собственному окну
средствами SDK, отдельно сообщает ошибки SDK. Это не подтверждение физического
вывода. Глобальная функция курсора ОС/SDK требует явного владельца при нескольких
окнах/контекстах. На этой точке Mouse.hide/show и привязка SDK ещё не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:18](../include/flashvm/mouse.h#L18) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:25](../include/flashvm/mouse.h#L25) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:32](../include/flashvm/mouse.h#L32) |
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:37](../include/flashvm/mouse.h#L37) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:20](../include/flashvm/mouse.h#L20) |
| `fvm_input_set_cursor_visible` | function | [include/flashvm/mouse.h:42](../include/flashvm/mouse.h#L42) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOUSE_EVENT_DOWN` | enum constant | [include/flashvm/mouse.h:14](../include/flashvm/mouse.h#L14) |
| `FVM_MOUSE_EVENT_MOVE` | enum constant | [include/flashvm/mouse.h:13](../include/flashvm/mouse.h#L13) |
| `FVM_MOUSE_EVENT_UP` | enum constant | [include/flashvm/mouse.h:15](../include/flashvm/mouse.h#L15) |
| `FVM_MOUSE_MAX_LISTENERS` | macro / define | [include/flashvm/mouse.h:6](../include/flashvm/mouse.h#L6) |
| `FVM_MOUSE_MIN_SWF_VERSION` | macro / define | [include/flashvm/mouse.h:5](../include/flashvm/mouse.h#L5) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:45](../include/flashvm/mouse.h#L45) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
