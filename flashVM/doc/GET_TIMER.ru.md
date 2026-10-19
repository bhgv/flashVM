# getTimer — ограниченные часы контекста (v0.620)

[English](GET_TIMER.md). [fvm_standard_install](../include/flashvm/standard.h#L20) добавляет `getTimer()` для host и
SWF4–8. Глобальная функция вызывает [fvm_exec_get_time](../include/flashvm/execution.h#L168) — тот же сервис, что GetTime
AVM1 и машинный код JIT. Дополнительные аргументы игнорируются без ToString/valueOf.
Извлечённая Function и обычный глобальный вызов используют одни часы.

## Часы и ограничения

Начало отсчёта — создание контекста, а не загрузка или запуск фильма. Если первый
отсчёт недоступен, первый успешный вызов устанавливает ноль. Допустимый отсчёт
платформы конечный и неотрицательный. Прошедшие миллисекунды округляются вниз и
берутся по модулю 2^32; значения выше 2^31 остаются положительными. При откате
часов сохраняется предыдущее полное прошедшее время. [fvm_advance](../include/flashvm/flashvm.h#L74) на эти часы не
влияет. Это существующий контракт execution-сервиса; совпадение начала отсчёта и
знакового переполнения с оригинальным Flash Player ещё не подтверждено.

Ожидание, планировщик, сеть и устройства не создаются. Контексты имеют независимые
начала отсчёта; вызовы выполняются на потоке владельца VM. `setInterval`,
`clearInterval` и `setTimeout` этим изменением не реализованы.

## Вызовы, ошибки и владение

Сначала вызвать [fvm_standard_install](../include/flashvm/standard.h#L20), затем [fvm_call](../include/flashvm/value.h#L138) с именем `getTimer`, нулём
аргументов и ненулевым выходным указателем. Конфликтующие собственные имена
сохраняются по активным правилам регистра SWF; установка возвращает
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Повторная установка проверяет идентичности регистраций.
В загруженных версиях ниже SWF4 callback возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
Глобальный вызов и [fvm_function_invoke](../include/flashvm/function.h#L65) извлечённой Function возвращают Number
при успехе, [FVM_ERROR](../include/flashvm/flashvm.h#L43) при недоступных/некорректных часах и undefined при ошибке.
Внутренний callback отвергает фазы уничтожения, сборки и освобождения до чтения
часов. Сам callback не выделяет память и не вызывает преобразования пользователя;
обвязка вызова может выделять память. Нативные барьеры остаются активными.
Публичная [fvm_exec_get_time](../include/flashvm/execution.h#L168) при ошибке сохраняет переданный результат.

[GC:] Извлечённую Function нужно сохранять в корне или закреплять [fvm_gc_pin](../include/flashvm/gc.h#L48)
через сборку, затем освобождать [fvm_gc_unpin](../include/flashvm/gc.h#L54). Числовой результат не владеет
управляемой памятью. ID нового callback — 163, старые 150–162 не меняются.
После сериализации и уничтожения отправителя импортированная Function читает
часы получателя. Начало отсчёта отправителя и платформенные handles не переносятся.

Реализация: [get_timer_call](../src/standard/numeric.cpp#L700); стабильный ID: [FVM_TIME_GET_TIMER_CALLBACK](../src/standard/time_constants.h#L5).

## Проверки

[tests/global_get_timer.cpp](../tests/global_get_timer.cpp): управляемые часы, host/SWF4–8, глобальный вызов и
Function, дроби, откат, границы 2^31/2^32, ошибки/восстановление, отсутствие
преобразований, фазовые ограничения, конфликт имён и перенос графа.
[tests/jit_get_timer.cpp](../tests/jit_get_timer.cpp): GetTime, глобальный вызов и alias в интерпретаторе,
ручном и автоматическом JIT для SWF4–8 с проверкой реальных native entries.
[tests/global_get_timer_allocation.cpp](../tests/global_get_timer_allocation.cpp): отдельные проверки отказов выделения.
[Пример C89](../examples/embedding/get_timer/README.ru.md).

[Справочник Adobe AS2](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html)
указывает Player 4+ и миллисекунды от запуска SWF. Это источник назначения и
доступности, но не подтверждение наших правил времени или непроверенных платформ.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_get_time` | function | [include/flashvm/execution.h:168](../include/flashvm/execution.h#L168) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `FVM_TIME_GET_TIMER_CALLBACK` | macro / define | [src/standard/time_constants.h:5](../src/standard/time_constants.h#L5) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
