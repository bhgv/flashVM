# URL-функции AS1/AS2

v0.619 добавляет `escape(value)` и `unescape(value)` через [`fvm_standard_install`](../include/flashvm/standard.h#L22).
Поддержаны контекст хоста и SWF5–8. Вызов из SWF4 даёт [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48), в том
числе после установки до загрузки. Конфликтующие собственные привязки, включая нативные методы и регистровые
алиасы SWF5/6, сохраняются с отказом начальной установки. Функции не обращаются к сети или кодировке ОС.
[English](URL_FUNCTIONS.md).

Правила пунктуации взяты из
[исходного справочника Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html):
кодируются даже точка и @. Это отличается от ECMAScript/AS3 escape. Unicode-кодирование
также сопоставлено с [реализацией AVM1](https://github.com/ruffle-rs/ruffle/blob/147cacd6c081fa33e78b3d57efdde8e9415225a2/core/src/avm1/globals.rs),
но она не заменяет эталонный проигрыватель. Новых зависимостей/заимствованного кода нет.

Implementation: [`fvm_builtin_escape`](../src/standard/url_encoding.cpp#L178) / [`fvm_builtin_unescape`](../src/standard/url_encoding.cpp#L186); public invocation uses [`fvm_call`](../include/flashvm/value.h#L204) or [`fvm_function_invoke`](../include/flashvm/function.h#L79).

## Поддержанный профиль

| Вызов | Результат |
| --- | --- |
| `escape("a@b.c")` | `a%40b%2Ec` |
| `escape("@*_+-./%")` | `%40%2A%5F%2B%2D%2E%2F%25` |
| `unescape("%2b+%2520")` | `+ %20`, один проход |
| Нет аргумента | undefined без преобразования |
| Явный undefined | Пустая строка в SWF5–6; строка `undefined` в SWF7–8 и контексте хоста |
| Дополнительные аргументы | Игнорируются |
| Некорректный escape или декодированные байты | [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48), без частичного результата |

Escape сохраняет только ASCII-буквы/цифры, остальные байты переводит в `%HH` с
прописными hex-цифрами. Строки имеют счётную длину UTF-8/WTF-8; NUL становится `%00`.
Парные суррогаты публичного UTF-16 конструктора представлены четырьмя UTF-8 байтами;
одиночные сохраняют трёхбайтовый WTF-8. Unescape принимает оба регистра hex, заменяет
буквальный плюс пробелом, а `%2B` оставляет плюсом. Перед публикацией результат
проверяется как счётный WTF-8.

Некорректные `%`, `%0`, `%GG`, `%uXXXX`, overlong, оборванные и выходящие за диапазон
UTF-8-последовательности пока не поддержаны. Историческое восстановление повреждённых
строк, замена неверных байтов, System.useCodepage/локальные кодировки и полный
эталонный аудит версий остаются открытыми. Это не HTML/XML/общий URI-компонентный
кодек и не реализация LoadVars/сети.

## Преобразование, владение и ошибки

Используются обычные правила AVM1 выбранной версии. Getters и `toString` объекта
могут выполнять скрипт, менять globals, собирать память или возбуждать исключение.
Ошибка передаётся без повтора и отката этих эффектов. Прямой вызов глобальной
функции теперь учитывает доверенный вход runtime и удерживает копию capture/аргументов
даже при замене собственной регистрации. Обычные нативные callbacks сохраняют
запрет GC; внешний нативный барьер никогда не снимается. Публичного механизма
доверия произвольным callbacks хоста не добавлено.

Результат принадлежит контексту. При traced-string GC его нужно удерживать в
достижимом хранилище VM перед последующей сборкой; локального C-значения недостаточно.
При ошибке частичная строка не публикуется. Эффекты и выделения завершённого
преобразования могут остаться; отказ принятия буфера освобождает неопубликованные
байты. Память: [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46); некорректная фаза жизни: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47);
настоящие исключения скрипта передаются без изменения.

Преобразование и обработка строки используют общий бюджет. Escape учитывает каждый
входной байт; unescape — входные байты и каждую проверяемую выходную scalar/одиночную
surrogate-единицу. Исчерпание/обнаруженная остановка worker дают [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Это не
время и не предел выделения памяти. Ноль отключает числовой предел, но не проверку
остановки. Вызывать в потоке владельца, вне GC/освобождения контекста.

## Графы и проверки

Старые callback ID 150–156 и 158–160 сохранены; новые используют 161/162. Извлечённая
Function переносится графом, переживает уничтожение отправителя и восстанавливает
доверенный вход для собирающего память преобразования. Старые получатели отклоняют
новые ID как неподдержанные; формат конверта не изменён.

Тесты: `url_encoding`, `jit_url_encoding`, `url_encoding_graph`,
`url_encoding_allocation`, `global_call_trusted_capture`: Unicode/NUL/суррогаты,
пунктуация, undefined/отсутствующий аргумент по версиям, отказ повреждённого ввода,
lifecycle/бюджет, OOM, настоящий SWF5–8 interpreter/manual/automatic JIT, GC/исключения,
сохранение старых ID, teardown отправителя и захват при самозамене глобальной функции.
[Пример C89](../examples/embedding/url_encoding/README.ru.md).
Это ограниченные проверки Linux, не полная SWF/межплатформенная приёмка.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_builtin_escape` | function | [src/standard/url_encoding.cpp:178](../src/standard/url_encoding.cpp#L178) |
| `fvm_builtin_unescape` | function | [src/standard/url_encoding.cpp:186](../src/standard/url_encoding.cpp#L186) |
| `fvm_call` | function | [include/flashvm/value.h:204](../include/flashvm/value.h#L204) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:79](../include/flashvm/function.h#L79) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../include/flashvm/standard.h#L22) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
