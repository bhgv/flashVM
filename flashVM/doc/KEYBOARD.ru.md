# Состояние клавиатуры хоста и опрос Key в AS1/AS2

В v0.589 добавлено явно устанавливаемое подмножество объекта Flash `Key` для
опроса. Оно использует существующий диспетчер native-методов/AVM1 и не открывает
системные устройства. [English contract](KEYBOARD.md).

## Установка

Сначала вызывается [`fvm_standard_install`](../include/flashvm/standard.h#L18), затем [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L85).
Последняя публикует глобальный объект `Key` с методами `getCode`, `getAscii`,
`isDown`, `isToggled` и константами BACKSPACE, TAB, ENTER, SHIFT, CONTROL, CAPSLOCK,
ESCAPE, SPACE, PGUP, PGDN, END, HOME, LEFT, UP, RIGHT, DOWN, INSERT, DELETEKEY.
Значения соответствуют виртуальным кодам `FVM_KEY_*`, а не системным scancode.
В v0.590 добавлены [слушатели и доставка](KEYBOARD_LISTENERS.ru.md).
Конструктор, `isAccessible` и семантика доменов безопасности отсутствуют.

[`FVM_OK`](../include/flashvm/flashvm.h#L42) возвращается после полной подготовки объекта. Конфликт с собственным
глобальным значением/accessor/native/script/constructor-регистрацией даёт
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), сохраняя старую привязку. Повторная установка успешна,
если native-методы остались исходными и не затенены. Она не восстанавливает
изменённые скриптом константы/атрибуты. Замена или затенение метода отклоняется.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) сохраняет глобальные значения; временный неопубликованный
объект может остаться обычным мусором до GC. Прочие C++ исключения превращаются
в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) и не пересекают ABI. Нужен живой контекст с установленной стандартной
библиотекой; повторный вход из GC/разрушения отклоняется. Основная функция
[`fvm_standard_install`](../include/flashvm/standard.h#L18) не подключает это подмножество автоматически.

## API снимка

[`FVM_KeyboardState`](../include/flashvm/keyboard.h#L50) — обычная память вызывающего кода. Обнулите её целиком,
затем задайте `struct_size = sizeof(FVM_KeyboardState)`.

| Поле | Контракт |
| --- | --- |
| `struct_size` | Точный текущий размер структуры; иные размеры отклоняются. |
| `down` | [`FVM_KEYBOARD_CODE_COUNT`](../include/flashvm/keyboard.h#L8) независимых байтов 0/1; нулевая ячейка всегда ноль. |
| `last_code` | Последний код нажатия, 0..255; ноль означает отсутствие известного кода. Отпускание может сохранить его. |
| `last_ascii` | Английский ASCII последнего перехода, 0..[`FVM_KEYBOARD_ASCII_MAX`](../include/flashvm/keyboard.h#L9); ноль при отсутствии. Не Unicode-ввод текста. |
| `caps_lock`, `num_lock` | Строго 0/1, состояние переключателя независимо от удержания клавиши. |

`fvm_input_set_keyboard(context, state)` проверяет все поля до копирования.
Нулевые указатели, неверный размер, ячейки, коды или переключатели дают
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), сохраняя старое состояние целиком. После успеха память
хоста можно переиспользовать: указатель не сохраняется. Функция не выводит события
из разницы снимков, не меняет указатель/очередь жестов, не выделяет память и
не выполняет getter или callback.

`fvm_input_get_keyboard(context, state)` требует заранее заданный точный
`struct_size` и копирует текущее состояние. При ошибке сохраняет все байты
выходной структуры. Новый контекст имеет нулевой ввод и правильный размер.

`fvm_input_reset_keyboard(context)` обнуляет удержания, последние коды и
переключатели, сохраняя правильный размер. Неверный контекст даёт
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Используйте при потере фокуса; после возвращения публикуйте
действительное состояние системных переключателей. Вымышленные отпускания не
доставляются. Set/get/reset не выделяют память, возвращают только [`FVM_OK`](../include/flashvm/flashvm.h#L42) или
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).

Все три функции вызываются исключительно на потоке владельца VM, без внутренней
синхронизации. Вход во время GC collection/release или разрушения отклоняется.
Управляемых значений/корней GC в снимке нет. Системный поток должен сначала
передать ввод в цикл владельца. Целостность валидации не означает межпоточную
атомарность структуры.

## Методы скрипта

| Метод | Реализованное поведение |
| --- | --- |
| `Key.getCode()` | Number из `last_code` текущего контекста, изначально/после сброса ноль. Лишние аргументы игнорируются. |
| `Key.getAscii()` | Number из `last_ascii`, изначально/после сброса ноль. Лишние аргументы игнорируются. |
| `Key.isDown(code)` | Boolean удержания выбранной виртуальной клавиши; несколько клавиш независимы. |
| `Key.isToggled(code)` | Boolean Caps Lock либо Num Lock по числовому [`FVM_KEY_NUMLOCK`](../include/flashvm/keyboard.h#L44); для прочих кодов false. |

Запросы преобразуют первый аргумент через [`fvm_value_to_number`](../include/flashvm/standard.h#L23); ошибки и
исключения преобразования распространяются. Нет аргумента, NaN, бесконечность,
отрицательное значение, число меньше 1 или >=256 дают false. Допустимая дробная
часть отбрасывается к нулю. Преобразование может выполнить скрипт; состояние
читается после него, поэтому опубликованное внутри преобразования обновление
видно. Лишние аргументы игнорируются. Сравнение числовых крайних случаев с
оригинальным плеером ещё открыто: это контракт текущего подмножества.

Методы используют вызывающий контекст, в том числе через второе имя объекта.
[GC:] Глобальное значение удерживает Key; при его замене хост должен сам
защитить сохранённый объект корнем/pin. Клавиатурное состояние не сериализуется;
сериализация самого host-local native-объекта сейчас возвращает [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).
В другом контексте явно установите объект и опубликуйте отдельный снимок.
Указатели и системное состояние в граф не переносятся.

## SDL-плеер

Плеер явно устанавливает Key и перед каждым movie tick публикует полный снимок.
[Переходник](../frontend/common/keyboard_sdl.h) переводит буквы/цифры, стрелки,
навигацию, Shift/Control/Alt, переключатели, F1..F12, цифровой блок и английскую
пунктуацию. Левый/правый модификаторы используют общую ячейку; маска SDL сохраняет
удержание второго при отпускании первого. Регистр букв: Shift XOR Caps Lock;
Shift меняет ASCII цифр/пунктуации по английской раскладке. Неизвестные символы не
меняют удержание/последний код нажатия. Неизвестный ASCII равен нулю.
IME, составной текст, точный перевод физических scancode для всех раскладок и
комбинации управляющих символов здесь не реализованы.

Потеря фокуса/фон сбрасывают клавиши, коды и переключатели. Escape/Android Back
сохраняют прежнее закрытие плеера. Очень короткое нажатие между movie ticks может
не попасть в опрос удержания; счётчики HostInput сохраняют такие нажатия.
Поля HostInput для указателя/кнопок/счётчиков остаются доступны. Доставка Key listeners теперь описана [здесь](KEYBOARD_LISTENERS.ru.md).
Автоматические key-события кнопок/клипов ещё не доставляются.

## Пример и проверки

[Пример C89 с построчными комментариями](../examples/embedding/keyboard_polling/README.md):
создание владельцев, одновременные удержания, опрос, сброс и обработка ошибок.
`tests/input_keyboard.c` проверяет изоляцию, валидацию, конфликт установки, GC и
отказ сериализации; `tests/jit_keyboard.cpp` — реальные AVM1-вызовы для SWF5..8
в interpreter/manual-JIT/automatic-JIT. Перебор отказов выделения памяти проверяет
транзакционность установки и отсутствие выделений в операциях снимка.
`frontend/tests/keyboard.c` проверяет реальный код SDL-переходника и сброс фокуса/
фона без открытия устройств. Это не аппаратная приёмка клавиатуры на каждой ОС
и не полное сравнение с оригинальным Flash.

Основа интерфейса: документ Adobe
[ActionScript 2.0 Language Reference](https://homepage.cs.uiowa.edu/~slonnegr/flash/ActionScript2Reference.pdf),
глава Key, копия исходного руководства издателя на сайте University of Iowa.
Ограничения безопасности и key-события кнопок/клипов остаются работой роудмапа.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:63](../include/flashvm/keyboard.h#L63) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:65](../include/flashvm/keyboard.h#L65) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:61](../include/flashvm/keyboard.h#L61) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_KEY_NUMLOCK` | macro / define | [include/flashvm/keyboard.h:44](../include/flashvm/keyboard.h#L44) |
| `FVM_KEYBOARD_ASCII_MAX` | macro / define | [include/flashvm/keyboard.h:9](../include/flashvm/keyboard.h#L9) |
| `FVM_KEYBOARD_CODE_COUNT` | macro / define | [include/flashvm/keyboard.h:8](../include/flashvm/keyboard.h#L8) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../include/flashvm/keyboard.h#L50) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:85](../include/flashvm/keyboard.h#L85) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_value_to_number` | function | [include/flashvm/standard.h:23](../include/flashvm/standard.h#L23) |
<!-- source-links:end -->
