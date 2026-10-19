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
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:20](../include/flashvm/mouse.h#L20) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOUSE_EVENT_DOWN` | enum constant | [include/flashvm/mouse.h:14](../include/flashvm/mouse.h#L14) |
| `FVM_MOUSE_EVENT_MOVE` | enum constant | [include/flashvm/mouse.h:13](../include/flashvm/mouse.h#L13) |
| `FVM_MOUSE_EVENT_UP` | enum constant | [include/flashvm/mouse.h:15](../include/flashvm/mouse.h#L15) |
| `FVM_MOUSE_MAX_LISTENERS` | macro / define | [include/flashvm/mouse.h:6](../include/flashvm/mouse.h#L6) |
| `FVM_MOUSE_MIN_SWF_VERSION` | macro / define | [include/flashvm/mouse.h:5](../include/flashvm/mouse.h#L5) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:29](../include/flashvm/mouse.h#L29) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
