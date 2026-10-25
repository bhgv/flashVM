# Исполнение AVM1

FlashVM исполняет часть AVM1: C++98 внутри VM, C89 в API встраивания. Загрузка
FWS/CWS до SWF8, GC, прототипы, метатаблицы, нативные методы и именованные
скриптовые функции доступны. Полная совместимость Flash8, таймлайн и JIT ещё
не реализованы.

[fvm_avm1_execute](../include/flashvm/avm1.h#L46) проверяет поток с завершающим End и запускает его в контексте.
[fvm_swf_execute_actions](../include/flashvm/avm1.h#L93) запускает верхние DoAction загруженного SWF, но не
реализует полный цикл кадров и событий спрайтов. [fvm_avm1_call](../include/flashvm/avm1.h#L82) вызывает функцию
с корнями GC для аргументов и получателя. Нативные функции и глобальные
переменные работают в том же контексте.

Тесты avm1*, interpreter.c, swf_execute.c и actionscript* покрывают Push,
константы, стек, переменные и локальные определения, свойства, арифметику,
переходы, вызовы и return. Неподдерживаемые инструкции возвращают UNSUPPORTED.
Переходы разрешены только на границы инструкций текущего блока. Лимит инструкций
задаётся для кадра функции; вложенные try/catch/finally используют общий лимит.

ActionTry/ActionThrow SWF7–8 поддерживают catch по имени/регистру, передачу
исключения между функциями, finally при return и замену результата finally.
См. [исключения и ограничения](EXCEPTIONS.ru.md). Прежний формат DefineFunction
и четыре регистра сохраняют ограничения; DefineFunction2 не реализован.

Один контекст нельзя одновременно исполнять из разных потоков. При ошибке
ранее изменённые глобальные переменные не откатываются. Ошибки памяти и
неподдерживаемый байткод возвращают статусы, а не AS-исключения. Данный этап
не подтверждает полную совместимость или безопасность произвольного SWF.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_call` | function | [include/flashvm/avm1.h:82](../include/flashvm/avm1.h#L82) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:46](../include/flashvm/avm1.h#L46) |
| `fvm_swf_execute_actions` | function | [include/flashvm/avm1.h:93](../include/flashvm/avm1.h#L93) |
<!-- source-links:end -->
