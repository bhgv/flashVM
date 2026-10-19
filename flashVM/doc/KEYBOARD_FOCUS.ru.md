# Явный выбор получателя клавиатуры хостом

В v0.591 C89-хост может выбрать нативный MovieClip для получения `onKeyDown` /
`onKeyUp`. Используется тот же вызов методов с защитой GC и правильным `this`,
что и у [слушателей Key](KEYBOARD_LISTENERS.ru.md). [English](KEYBOARD_FOCUS.md).

Это политика встраивания: API не устанавливает `Selection`, не реализует правила
доступности фокуса Flash, выбор мышью/Tab, уведомления о смене фокуса и SWF
ClipActions. SDL-плеер сам не выбирает клип. Хост может направлять ввод выбранному
ролику, сохраняя управление выбором в своём интерфейсе. Низкоуровневый выбор
игнорирует `focusEnabled`, `tabEnabled`, видимость и `_focusrect`; выдавать его за
совместимую реализацию `Selection.setFocus` нельзя.

## fvm_input_set_keyboard_focus

`fvm_input_set_keyboard_focus(context, clip)` выбирает один нативный MovieClip.
`context` — заимствованный ненулевой живой контекст. `clip` — null для сброса либо
заимствованный живой объект именно этого контекста. Отдельно созданный корневой
клип допустим. Прототип класса, обычный объект и клип, удалённый сам или вместе с
нативным физическим предком, отклоняются. Проверяется закрытое состояние сцены,
а не заменяемые скриптом свойства `_parent`.

Успех — [`FVM_OK`](../include/flashvm/flashvm.h#L42): прежний выбор атомарно заменяется новым. Контекст удерживает
сильный корень GC на клип и достижимую сцену; дополнительный pin на время выбора
не нужен. У прежнего клипа снимается только этот корень, остальные ссылки, pins
и регистрации слушателя сохраняются. Скрипт не исполняется, состояние клавиатуры
не меняется, уведомлений нет. Сброс через null не выделяет память. Проверка
ненулевого клипа может выделять временные нативные строки поиска. Обход
ограничен [`FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS`](../src/standard/movieclip_constants.h#L68): 256 посещённых узлов, включая
сам выбранный клип.

[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) — неверный контекст, чужой объект, тип/прототип, удалённый
клип или повреждённое закрытое состояние. [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) — отказ памяти при
проверке. [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) — чрезмерная/циклическая цепочка предков или перехваченное
непредвиденное исключение C++. При ошибке прежний выбор сохраняется. Владение
userdata не передаётся, обращения к устройствам ОС нет.

## fvm_input_get_keyboard_focus

`fvm_input_get_keyboard_focus(context, out)` принимает живой заимствованный
контекст и ненулевой принадлежащий вызывающему `FVM_Object **out`. При [`FVM_OK`](../include/flashvm/flashvm.h#L42)
записывает заимствованный указатель на клип либо null. Новый pin не создаётся:
для хранения указателя после сброса/смены фокуса нужен обычный корень GC.

Если клип или его нативный предок удалены, запрос либо следующая доставка
клавиатуры лениво снимают выбор и возвращают null. До такой проверки, явного
сброса, замены или уничтожения контекст может удерживать удалённый граф. Простое
скрытие или смена родителя живого клипа не сбрасывают выбор. Скриптовые getters
и уведомления не вызываются.

[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) — неверный контекст/выход либо повреждённое состояние;
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) — отказ памяти при проверке; [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) — чрезмерная цепочка
предков либо непредвиденное перехваченное исключение C++. Ошибка сохраняет и
выход, и выбор. Запрос пустого выбора не выделяет память.

## Доставка и время жизни

[`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90) проверяет снимок и удалённость выбранного клипа,
затем подготавливает защищённый корнями список: выбранный клип первым, далее
слушатели Key в порядке регистрации. Только после успешной подготовки публикует
новое состояние клавиатуры. Каждый получает ноль аргументов и собственный объект
в `this`; коды доступны через Key или [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L71). Если выбранный
клип также явно зарегистрирован слушателем Key, он получает оба вызова: каналы
независимы, устранения повторов между ними нет. Выбор сам по себе не добавляет
клип в Key._listeners и не расходует лимит регистраций.

Порядок «клип, затем слушатели» — политика хоста FlashVM; порядок оригинального
плеера пока не подтверждён. Смена выбора внутри callback влияет на следующее
событие. Текущий снимок защищает объекты даже после смены фокуса, удаления
последующих слушателей и GC. Методы ищутся непосредственно перед соответствующим
вызовом, поэтому ранний callback может заменить поздний обработчик. Отсутствующий
или невызываемый обработчик пропускается; настоящая ошибка getter/обработчика
останавливает доставку и передаётся вызывающему. Опубликованное состояние и
побочные эффекты сохраняются. Ошибка подготовки сохраняет состояние клавиатуры;
обнаружение удаления уже могло снять устаревший выбор. Вложенная доставка запрещена.

Выбор принадлежит контексту. API вызывается в потоке-владельце VM, без
синхронизации. Уничтожение и сборка/освобождение GC запрещают оба вызова, обычный
callback клавиатуры допускает их. Уничтожение контекста снимает корень и остаётся
запрещённым во время доставки. Выбор, физическое состояние и регистрации не
сериализуются; [`fvm_load_memory`](../include/flashvm/flashvm.h#L72), замена Key и [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L73) их
автоматически не снимают. Хост должен явно сбросить выбор при выгрузке ролика
или передаче ввода другому интерфейсу. Потеря фокуса ОС отдельно сбрасывает
физическое состояние клавиатуры.

## Пример и проверки

[Исполняемый C89-пример](../examples/embedding/keyboard_listeners/README.md)
сравнивает слушателя Key и явный выбор клипа, без окна и устройств. После
установки стандартных объектов он вызывает [`fvm_movieclip_create`](../include/flashvm/movieclip.h#L36) и сразу
выбирает возвращённый заимствованный клип, защищая его до выделений памяти при
назначении обработчиков. При любой ошибке освобождение завершает контекст.

`tests/keyboard_focus.cpp` проверяет типы/владение/прототип, корни GC, смену фокуса
из callback со сборкой, сохранность снимка слушателей, два независимых канала,
ошибки обработчика, удаление предка и ленивый сброс. В
`tests/jit_keyboard_listeners.cpp` реальные скриптовые callbacks выполняются на
выбранном клипе для SWF6–8 в интерпретаторе, ручном и автоматическом JIT.
`tests/keyboard_dispatch_allocation.cpp` проверяет сохранение выхода при отказах
памяти, ошибку подготовки и сброс null без выделения памяти. Это тесты VM/хоста,
а не приёмка физической клавиатуры или полной совместимости фокуса Flash.

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L101).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../include/flashvm/keyboard.h#L71) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:36](../include/flashvm/movieclip.h#L36) |
| `FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:68](../src/standard/movieclip_constants.h#L68) |
| `FVM_Object` | struct | [src/core/runtime.hpp:122](../src/core/runtime.hpp#L122) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
<!-- source-links:end -->
