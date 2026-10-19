# Слушатели Key и последовательная доставка клавиатуры

v0.590 расширяет явно устанавливаемый Key регистрацией и синхронными событиями.
[Снимок и опрос](KEYBOARD.ru.md) / [English contract](KEYBOARD_LISTENERS.md).
Автоматические key-события MovieClip/кнопок, домены безопасности, IME/ввод текста
и полное сравнение с оригинальным плеером остаются открытыми.

## Нативный API

Все функции работают на потоке владельца VM, без внутренней синхронизации.
Вход во время разрушения контекста или GC collection/release запрещён.
Регистрация не открывает устройство, не меняет клавиатурное состояние и не
вызывает обработчик. Для C API установка Key/стандартной библиотеки не нужна;
для скриптового переходника нужна.

### fvm_input_add_key_listener

`fvm_input_add_key_listener(context, listener)` удерживает уникальный объект
того же контекста по его идентичности. Оба указателя — ненулевые живые borrowed
объекты. Чужой контекст отклоняется; native userdata и capture не копируются.
Повторная регистрация успешна без выделения памяти и без изменения порядка.
Новый объект добавляется в конец порядка доставки и становится сильным корнем
GC до удаления/разрушения контекста, даже если скрипт удалил Key или последнюю
обычную ссылку на слушателя.

Неверные аргументы/владение дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); достижение
[`FVM_KEYBOARD_MAX_LISTENERS`](../include/flashvm/keyboard.h#L6) (1024) или непредвиденное перехваченное C++ исключение —
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43); отказ роста native-вектора — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44). При ошибке прежний
список сохраняется. Сохраняемый хостом после удаления объект нужно защищать
обычным корнем/pin.

### fvm_input_remove_key_listener

`fvm_input_remove_key_listener(context, listener, removed)` удаляет по идентичности.
Нужны живой контекст, ненулевой объект того же контекста и ненулевой
выходной `int *removed` в памяти вызывающего. Неверные аргументы дают
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), сохраняя список и выход. Успех — [`FVM_OK`](../include/flashvm/flashvm.h#L42); `*removed`
равен 1 при удалении или 0 при отсутствии. Память не выделяется; порядок
оставшихся сохраняется. Состояние клавиш не сбрасывается, обработчики не вызываются.

Удаление во время доставки влияет на следующий снимок получателей; объект из
текущего снимка всё равно получает текущее событие. Удаление с повторным добавлением
перемещает его в конец следующего порядка. Финализация не запускается принудительно.

### fvm_input_dispatch_keyboard

`fvm_input_dispatch_keyboard(context, state, pressed)` принимает borrowed полный
снимок состояния после перехода и строго 0/1 в `pressed`. Все поля проверяются
как в [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L61). Согласованность состояния с физическим переходом,
последние коды и переключатели задаёт хост; функция не выводит их из фазы и не
опрашивает ОС. Повтор клавиши можно явно передать ещё одним нажатием. Указатель
на снимок не сохраняется.

С v0.591 сначала во временный rooted-вектор копируется явно выбранный клип,
затем список регистраций. [Выбор, два канала и удаление](KEYBOARD_FOCUS.ru.md#доставка-и-время-жизни).
Только после успешной подготовки публикуется клавиатурное состояние. Затем
в порядке регистрации вызывается текущий `onKeyDown` либо `onKeyUp` каждого
получателя: без позиционных аргументов, с объектом-слушателем в `this`.
Изменения подписок влияют на следующее событие; замены методов/getters учитываются
при доставке конкретному получателю. Отсутствующий или невызываемый обработчик
пропускается. Наследование, accessors и загруженный `__resolve` используют
существующую диспетчеризацию методов. Отсутствующий обработчик объекта не
подменяется одноимённой глобальной native-функцией.

Ошибка настоящего getter/обработчика, включая возвращённый вызываемым native-кодом
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), распространяется и останавливает остальных получателей.
Опубликованное состояние, уже выполненные callbacks и изменения подписок не
откатываются. Возвращённое обработчиком значение игнорируется, код ошибки/исключение —
нет. Не повторяйте ошибочное событие вслепую: первые получатели могут выполниться
дважды. Pending-исключение остаётся в обычном exception API.

Неверные указатели/размер/поля/фаза и вложенная клавиатурная рассылка дают
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до изменения состояния. OOM подготовки даёт
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), сохраняя прежнее состояние. OOM последующего поиска метода
или исполнения распространяется уже после публикации. Непредвиденные C++
исключения превращаются в [`FVM_ERROR`](../include/flashvm/flashvm.h#L43), не пересекают ABI.

[GC:] Контекст маркирует все регистрации. Снимок получателей удерживает удалённые
из списка объекты до конца текущего события; owner/receiver, аргументы и результат
getter защищаются существующим rooted-диспетчером. Скрипт сохраняет обычные правила
GC interpreter/JIT. Обычные native callbacks сохраняют native-барьер сборщика.
Разрушение контекста во время рассылки отклоняется; runtime и presentation-объекты
из обработчика разрушать запрещено. Scope guard снимает флаг доставки и временные
корни при любом выходе. Set/reset состояния и изменения подписок внутри обработчика
разрешены; последующие обработчики видят это явное изменение состояния.

## Скриптовый интерфейс

Как прежде, вызовите [`fvm_standard_install`](../include/flashvm/standard.h#L18), затем [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L88).
Установка теперь атомарно готовит также методы регистрации и getter списка;
повторная установка проверяет исходные незатенённые callbacks. Регистрации не
очищаются и не дублируются. Это явная установка подмножества; полная политика
доступности по версиям оригинального Flash и безопасности не реализована.

| Член | Текущий контракт |
| --- | --- |
| `Key.addListener(object)` | Уникальная регистрация, результат Undefined. Отсутствующий/необъектный аргумент игнорируется без преобразования. Ошибки native-регистрации распространяются. |
| `Key.removeListener(object)` | Boolean удаления; для отсутствующего/необъектного аргумента false. |
| `Key._listeners` | Read-only свойство: новый Array-снимок в порядке регистраций. |

Array содержит исходные ссылки и использует сохранённый builtin Array prototype,
не вызывая заменённый скриптом конструктор Array или prototype-getter.
Сохраняемый native-кодом через GC массив нужно защищать корнем. Изменение
массива не меняет подписки; следующее чтение создаёт новый снимок. Постоянная
идентичность массива и соответствие мутациям оригинального плеера не заявляются.
Запись в getter игнорируется, удаление блокируется исходными атрибутами.
Ошибки выделения памяти getter обрабатываются обычным механизмом VM.

Регистрации принадлежат контексту, а не конкретному Key/ролику. Reload и замена
Key их не удаляют: устаревшие подписки нужно снять явно. Физический ввод и
регистрации не передаются с сериализованным графом; импортированные слушатели
нужно зарегистрировать заново. Сериализация native-объекта Key пока unsupported.

## Очередь SDL

Оконный переходник копирует поддерживаемые переходы в native FIFO из
`FVM_DESKTOP_PENDING_EVENTS` (128) полных снимков. Нажатия, отпускания и повторы
сохраняются даже между movie ticks. Неподдерживаемые SDL-символы события не
добавляют. Сбор SDL-событий не исполняет VM-код.

`fvm_desktop_window_pump_input` работает на потоке владельца до следующего
кадра; затем плеер публикует окончательный HostInput/снимок опроса и продвигает
ролик. Pump обрабатывает только входную порцию; повторный вход запрещён.
Потеря фокуса/фон отменяют накопленные переходы и сбрасывают удержания/коды/
переключатели без вымышленных отпусканий. Escape/Android Back по-прежнему
закрывают тест. FIFO не содержит VM-указателей/корней и находится вне бюджета VM.

Переполнение фиксирует [`FVM_ERROR`](../include/flashvm/flashvm.h#L43); плеер сообщает ошибку ввода вместо тихой
потери событий. Pump удаляет текущее событие до dispatch: ошибка потребляет его,
сохраняя последующие для хоста, решившего продолжить. Обычный плеер сообщает
ошибку и закрывает текущий ролик. Отмена при потере фокуса не скрывает ранее
зафиксированное переполнение. Callback не должен разрушать или повторно входить
в жизненный цикл presentation-хоста.

## Примеры и проверки

- [Пример C89](../examples/embedding/keyboard_listeners/README.md): setup, сильная регистрация, нажатие/отпускание, освобождение владельцев.
- [Ролик 02](../examples/player/animation_audio/README.md): принадлежащий ролику observer регистрируется один раз при всех циклах timeline. Нажатия увеличивают зелёный маркер по высоте; отпускания сдвигают вниз. Короткий тап обновляет оба счётчика, даже если опрос удержания его не заметил.
- Core-тесты: remove/add/настоящий GC, дубликаты, receiver, наследуемый getter с одним чтением, отсутствие global fallback, невызываемые члены, ошибки вызываемого кода, исключения, запрет вложенной рассылки.
- Отказы памяти различают подготовку и уже опубликованную доставку, проверяют отсутствие выделений у дубликата/удаления.
- Настоящая регистрация скриптом и тела callbacks проверяются в SWF6..8 через interpreter/manual-JIT/automatic-JIT.
- Готовый SWF проходит три цикла в interpreter/JIT с одной регистрацией и правильными обоими счётчиками. Тест реального кода SDL проверяет целый тап между кадрами и явное переполнение без устройств.

Это ограниченные программные проверки, а не аппаратная приёмка клавиатуры
каждой ОС или сертификация мутаций по оригинальному плееру. Основной справочник —
документ Adobe [AS2 Language Reference](https://homepage.cs.uiowa.edu/~slonnegr/flash/ActionScript2Reference.pdf), глава Key.


[Explicit host keyboard focus / Явный выбор клипа](KEYBOARD_FOCUS.md) ([RU](KEYBOARD_FOCUS.ru.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L75), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L78).


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
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:70](../include/flashvm/keyboard.h#L70) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../include/flashvm/keyboard.h#L82) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:78](../include/flashvm/keyboard.h#L78) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:72](../include/flashvm/keyboard.h#L72) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:61](../include/flashvm/keyboard.h#L61) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:75](../include/flashvm/keyboard.h#L75) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_KEYBOARD_MAX_LISTENERS` | macro / define | [include/flashvm/keyboard.h:6](../include/flashvm/keyboard.h#L6) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:88](../include/flashvm/keyboard.h#L88) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
