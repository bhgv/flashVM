# Объектные оболочки Boolean и нативные методы прототипов

[fvm_standard_install](../include/flashvm/standard.h#L18) теперь устанавливает метаданные глобального класса Boolean
и отдельный конструктор вместе с прежними числовыми функциями. Boolean(value)
через [fvm_call](../include/flashvm/value.h#L138)/AVM1 CallFunction возвращает примитив; new Boolean(value) через
[fvm_construct](../include/flashvm/value.h#L158)/AVM1 NewObject — объект под управлением GC. Лишние корректные
аргументы игнорируются. Установка остаётся явной, отклоняет конфликты и публикует
глобальные значения, функции и конструкторы вместе. Повторная установка проверяет
класс, методы прототипа и захват конструктора; замена проверяемой привязки даёт
INVALID_ARGUMENT.

Имена методов, значение по умолчанию и типы результатов сверены со
[справочником Adobe AS2 Boolean](https://open-flash.github.io/mirrors/as2-language-reference/Boolean.html).
Это не подтверждает полную совместимость с Flash и все различия версий.

| Вход | Сохранённый логический примитив |
| --- | --- |
| Нет аргумента, undefined, null | false |
| false, +0, -0, NaN, пустая строка | false |
| true, ненулевые числа, бесконечности, непустые строки | true |
| Любой объект своего контекста, включая оболочку false | true |

valueOf() возвращает сохранённый примитив; toString() — "true" либо "false".
Прототип Boolean сам хранит false. Обычный объект, наследующий этот прототип,
не становится оболочкой: методы отклоняют такой receiver. Пользовательские
свойства не меняют закрытый примитив. Number сейчас напрямую извлекает из
Boolean 0/1, не выполняя переопределения valueOf/toString; общее Object
ToPrimitive ещё не готово. Boolean(оболочкаFalse) даёт true, поскольку вход — объект.

## Поиск методов и API расширений

[fvm_object_define_method_rooted](../include/flashvm/value.h#L115)(context,object,name,callback,userdata,root)
создаёт либо заменяет собственный нативный метод. root — null либо объект того же
контекста; произвольная память userdata не сканируется. Собственные данные/accessor
с тем же именем мешают регистрации. Замена callback освобождает прежний корень
захвата. [fvm_object_has_own](../include/flashvm/value.h#L70) учитывает собственные методы, но не унаследованные.

[fvm_object_call](../include/flashvm/value.h#L126) и AVM1 CallMethod ищут метод по цепочке максимум из 1024 объектов.
Receiver остаётся исходным объектом, а не прототипом, хранящим callback.
Собственный метод перекрывает унаследованный. Первое свойство-данные/accessor
блокирует вызов нативного метода и даёт UNSUPPORTED: вызов function-valued свойств
и возвращаемых getter функций ещё не реализован. Присваивание данных может
перекрыть прежний callback, не удаляя его захват; он сохраняется до замены метода
либо сборки объекта-владельца.

Перед вызовом запись callback копируется: метод безопасно заменяет сам себя.
Нативный guard запрещает повторный вход в GC; захваты удерживаются через граф
receiver/прототипов. Прежний мост именованных функций применяется только при
отсутствии метода. Нативный метод прототипа в AVM1 имеет приоритет перед
одноимённой скриптовой функцией. Превышение бюджета цепочки даёт ERROR без fallback.
Это ограниченный нативный мост, а не полный Function. [fvm_object_get](../include/flashvm/value.h#L72) пока не
выдаёт нативный метод как значение-функцию.

## GC, потоки и примеры

Циклы класс/прототип/constructor отслеживаются GC. Реестр конструктора удерживает
класс даже после удаления глобальной привязки Boolean. Пока приложение хранит
указатель оболочки, нужен pin/корень. Начиная с версии 0.3 контекстные каналы
сериализуют внутреннее состояние Boolean и встроенные методы прототипа,
включая запись в файл. См. FILE_CHANNELS.ru.md; произвольным нативным callbacks
по-прежнему нужны зарегистрированные устойчивые идентификаторы.

Рабочие контексты ActionScript создают независимую установленную стандартную
библиотеку. Изменения прототипов родителя не копируются. Тест запускает восемь
потоков с локальным Boolean, GC и строковым результатом при переопределённом
toString родителя. C89-пример examples/embedding/standard_boolean/main.c
действительно создаёт оболочку и сравнивает примитив, строку, истинность объекта
и числовое преобразование. example.as — иллюстрация; тесты отдельно исполняют
настоящие байты AVM1, без заявления о корпусе внешнего AS-компилятора.

## Оставшиеся работы

Наследование Object/Function, атрибуты свойств, первоклассные методы,
автоматическая оболочка примитивного receiver, общее ToPrimitive, преобразования
операторов, различия SWF-версий и оболочки Number/String ещё не завершены.
Полная готовность Boolean/SWF 8 не заявлена; Windows/macOS/Android не проверены.
Проверки и состояние зеркала: STANDARD_BOOLEAN_CHECKPOINT.ru.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `fvm_construct` | function | [include/flashvm/value.h:158](../include/flashvm/value.h#L158) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../include/flashvm/value.h#L115) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_has_own` | function | [include/flashvm/value.h:70](../include/flashvm/value.h#L70) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
<!-- source-links:end -->
