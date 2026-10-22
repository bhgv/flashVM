# Файловые каналы и метатаблицы значений — версия 0.4, ABI 2

Маске `<"file", путь, режим>` соответствует `channel.create("file", path[, mode])` в подключаемом API ActionScript. В C89 используется `fvm_channel_open_file(path, mode, &channel)`. Путь — непустая строка UTF-8. Режим задаётся точно как `r`, `w` или `rw`; пропуск/NULL означает `rw`. Пустой или неизвестный режим и неверные типы аргументов отвергаются.

| Режим | Чтение | Запись | Открытие |
| --- | --- | --- | --- |
| `r` | Да | Ошибка | Существующий файл без изменений |
| `w` | Ошибка | Да | Создание или очистка |
| `rw` | Да | Да | Сохранение или создание; дописывание |

Позиции чтения и записи независимы. Чтение начинается с нулевого байта. После успешной отправки данные сбрасываются через stdio. Один общий удерживаемый дескриптор защищён мьютексом канала; разные дескрипторы и процессы этим мьютексом не синхронизируются. Для нескольких отправителей используйте общий дескриптор. Адаптер использует stdio и позиции `long`: файлы за пределами `LONG_MAX` на платформах с 32-битным `long` не заявлены. Сброс stdio не гарантирует атомарность при аварии или сохранность уровня fsync.

`send`/`receive` и `trySend`/`tryReceive` передают типизированные сериализованные записи. На конце файла неблокирующее чтение возвращает [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47) (`tryReceive()` — undefined), блокирующее — [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Оно не ждёт бесконечно дописывания другим процессом. Нарушение режима и операции после закрытия возвращают ошибку. `available` проверяет наличие оставшихся байтов, но не корректность следующей записи. Ёмкость очереди и количество сообщений для файла не поддерживаются.

Для обычных файлов есть [`fvm_channel_file_read`](../include/flashvm/channel.h#L34) и [`fvm_channel_file_write`](../include/flashvm/channel.h#L36), работающие с необработанными байтами, включая NUL. Они используют те же позиции. Не смешивайте байты и сообщения, если байты не представляют целые записи формата. В мосте ActionScript пока доступны типизированные сообщения.

## Формат сохранения графа

[`fvm_value_serialize`](../include/flashvm/serialization.h#L18) выделяет буфер; его надо ровно один раз освободить через [`fvm_serialized_free`](../include/flashvm/serialization.h#L24). [`fvm_value_deserialize`](../include/flashvm/serialization.h#L21) восстанавливает значение в контексте получателя. Каналы графов в памяти и файловые каналы используют один формат. Снимок не содержит указателей исходной VM и остаётся доступен после её уничтожения.

Формат 1: заголовок 16 байтов (`FVMG`, три нулевых байта, версия 1, 32-битная длина тела и CRC32 тела в big-endian). Числа записываются как IEEE-754 binary64 в big-endian. Объекты ссылаются на номера узлов графа. Это собственный формат FlashVM, не SWF, AMF или формат Lua. Несовместимое представление double отвергается. CRC обнаруживает случайное повреждение и не является аутентификацией.

Сохраняются именованные и индексированные свойства, прототипы, метатаблицы, циклы, общие ссылки, нативные методы/аксессоры и захваты зарегистрированных метаметодов. Метаданные примитивов также передаются. Получатель получает независимую копию; исходные объекты остаются у отправителя. Циклы прототипов недопустимы. Ограничения: 4096 объектов, 65536 записей свойств/методов и 64 МиБ на запись. Индексы имеют 32 бита без знака. Суммарный объём строк, текста и шрифтов проверяется до копирования снимка. Некорректные длины, типы, ссылки, повторяющиеся ключи, описания callbacks, CRC и обрезанные записи отвергаются. Неудачное восстановление откатывает объекты, строки, счётчик выделений и шрифты без вызова пользовательских финализаторов.

| Нативный объект | Поддержка сейчас |
| --- | --- |
| Обычные объекты/таблицы | Свойства, индексы, прототипы, метатаблицы |
| Обёртка Boolean и метаданные класса | Внутреннее Boolean, методы прототипа, граф класса |
| Math | Свойства, методы и состояние генератора исходного контекста; восстановление меняет генератор контекста получателя |
| TextFormat и метаданные текстовых классов | Свойства и методы; самостоятельному TextFormat для измерения нужен зарегистрированный соответствующий/стандартный шрифт получателя |
| Созданный хостом TextField | Текст, текущий/будущий стиль, размеры, переносы/многострочность, методы/аксессоры, байты используемых шрифтов и индексы faces; кэши пересчитываются |
| Живые потоки/каналы, произвольный userdata, стековые callable | Отвергаются до появления соответствующих кодеков |
| Пространства расширений gc/thread/channel/metatable | Кодека пространства пока нет; отвергаются |
| MovieClip и другие ещё не реализованные классы Flash | Реализации/кодека пока нет |

Перед восстановлением TextField вызовите [`fvm_standard_text_install`](../include/flashvm/standard_text.h#L21) в получателе. Встроенные в запись шрифты регистрируются транзакционно. Если имя уже существует, байты и индекс должны полностью совпадать; конфликт отвергает восстановление. Текущий и будущий шрифты записываются отдельно без дедупликации. Индексы faces в формате — знаковые 32-битные. Ограничения реализованного подмножества TextField остаются в силе.

Пользовательский нативный callback должен иметь одинаковое устойчивое имя в обоих контекстах, зарегистрированное через [`fvm_serialization_register_callback`](../include/flashvm/serialization.h#L30). Адрес кода не записывается. Userdata может быть NULL либо в точности указателем на отслеживаемый корень callback; корень копируется и привязка восстанавливается. Произвольная нативная память и callbacks входа в сборщик отвергаются. Встроенные callbacks и селекторы TextField имеют зарезервированные внутренние ID, которые нельзя заменить регистрацией. Отсутствие пользовательского callback вызывает ошибку восстановления. Именованные определения AVM1-функций и исполняемый байткод этим механизмом не переносятся.

Примитивное C-чтение возвращает [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), не снимая граф объекта/метаданных: затем можно использовать чтение с контекстом. Повреждённое или обрезанное обрамление файла не сдвигает позицию. Контекстное чтение снимает полностью обрамлённую запись даже при ошибке её CRC/кодека, как снимается неудачно восстановленный пакет очереди. Ошибка частичной записи блокирует дальнейшие операции с этим дескриптором; хвост файла может потребовать восстановления извне.

## Метатаблицы всех существующих типов значений

[`fvm_value_set_metatable`](../include/flashvm/metamethod.h#L36) / [`fvm_value_get_metatable`](../include/flashvm/metamethod.h#L38) работают с undefined, null, Boolean, числами, строками и объектами. У примитива метатаблица принадлежит значению и сохраняется при присваивании/копировании. Замена через [`fvm_value_number`](../include/flashvm/value.h#L42), [`fvm_value_string`](../include/flashvm/value.h#L46) и другие инициализаторы очищает её. У объекта она принадлежит общей идентичности, включая поддержанные встроенные объекты. Метатаблица должна принадлежать тому же контексту; NULL снимает привязку.

В ActionScript: `value = metatable.attach(value, table)` и `metatable.get(value)`. Второй аргумент null/undefined снимает привязку. Пространство устанавливается через [`fvm_extensions_install`](../include/flashvm/extensions.h#L17). GC отслеживает метаданные через глобальные переменные, свойства/индексы, стеки/регистры/локальные переменные AVM1, нативные корни и явные C root scopes. Локальная переменная C автоматически корнем не становится: используйте root scope, глобальную ссылку или закреплённый объект.

[`fvm_value_meta_invoke`](../include/flashvm/metamethod.h#L42) вызывает зарегистрированный нативный метаметод с аргументами `[значение-владелец, определяющая метатаблица, аргументы операции...]`. Receiver равен NULL для примитива и объекту для объекта. Отсутствующий метод даёт [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47). Поиск цепочки ограничен; результат callback проверяется на принадлежность контексту.

Допустимые имена: `__index`, `__newindex`, `__iterate`, `__gc`, `__call`, `__add`, `__sub`, `__mul`, `__div`, `__mod`, `__pow`, `__unm`, `__eq`, `__lt`, `__le`, `__concat`, `__tostring`. Обобщённый доступ к свойствам и AVM1 GetMember/SetMember вызывают `__index`/`__newindex` примитивов. AVM1 Add/Subtract/Multiply/Divide и Equals/Less сначала ищут обработчик слева, затем справа; аргументы операции сохраняют исходный порядок. Остальные обработчики доступны через явный вызов, но ещё не подключены ко всем опкодам и преобразованиям. Существующие API индексации, итерации, вызова и финализации объектов сохраняются. `__gc` относится к объектам со стабильной идентичностью, а не к отдельным копиям скаляров.

ABI 2 добавляет `FVM_Value.metatable`: пересоберите нативные клиенты и инициализируйте значения штатными helpers перед заполнением полей. Примитивные C API каналов/worker не могут удерживать указатели контекста и отвергают такие метаданные. Используйте контекстные каналы графов. Передача графов через нативный аргумент/результат worker ещё не реализована.

Исполняемый пример C89: `examples/embedding/file_channels/main.c`. Пример API ActionScript: `examples/actionscript/file_channels.as`. Файл `.as` — документация; тесты исполняют байткод AVM1 и C-вызовы моста. Буквальная маска `<...>`, стрелки и прочий расширенный синтаксис требуют соответствующего компилятора.

Тесты: `tests/file_channels.c`, `tests/value_metatables.c`, `tests/serialization_validation.c`, `tests/serialization_textfield.c` и существующие наборы для графов, GC, прототипов и потоков. Фактические результаты: `FILE_CHANNELS_CHECKPOINT.ru.md`. Этот этап не подтверждает готовность всех Flash-классов, произвольных замыканий или исполнение на всех платформах.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_file_read` | function | [include/flashvm/channel.h:34](../include/flashvm/channel.h#L34) |
| `fvm_channel_file_write` | function | [include/flashvm/channel.h:36](../include/flashvm/channel.h#L36) |
| `fvm_channel_open_file` | function | [include/flashvm/channel.h:28](../include/flashvm/channel.h#L28) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:17](../include/flashvm/extensions.h#L17) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_serialization_register_callback` | function | [include/flashvm/serialization.h:30](../include/flashvm/serialization.h#L30) |
| `fvm_serialized_free` | function | [include/flashvm/serialization.h:24](../include/flashvm/serialization.h#L24) |
| `fvm_standard_text_install` | function | [include/flashvm/standard_text.h:21](../include/flashvm/standard_text.h#L21) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `fvm_value_deserialize` | function | [include/flashvm/serialization.h:21](../include/flashvm/serialization.h#L21) |
| `fvm_value_get_metatable` | function | [include/flashvm/metamethod.h:38](../include/flashvm/metamethod.h#L38) |
| `fvm_value_meta_invoke` | function | [include/flashvm/metamethod.h:42](../include/flashvm/metamethod.h#L42) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../include/flashvm/value.h#L42) |
| `fvm_value_serialize` | function | [include/flashvm/serialization.h:18](../include/flashvm/serialization.h#L18) |
| `fvm_value_set_metatable` | function | [include/flashvm/metamethod.h:36](../include/flashvm/metamethod.h#L36) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../include/flashvm/value.h#L46) |
<!-- source-links:end -->
