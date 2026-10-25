# Переменные и свойства в JIT — v0.350

[English reference](JIT_REFERENCES.md)

Добавлены нативные SetVariable, DefineLocal/DefineLocal2, GetMember/SetMember,
Delete и Delete2. Все генераторы машинного кода используют общий план прямых
вызовов семантических сервисов. Диспетчер инструкций интерпретатора из машинных
блоков не вызывается.

Запись/объявление выбирают MovieClip либо глобальный объект, script-this и
подготовленную activation по тем же правилам, что интерпретатор. Используются
общие [`fvm_exec_variable_set`](../include/flashvm/execution.h#L542), [`fvm_exec_variable_declare`](../include/flashvm/execution.h#L566), [`fvm_exec_member`](../include/flashvm/execution.h#L278),
[`fvm_exec_delete_member`](../include/flashvm/execution.h#L168), [`fvm_exec_delete_reference`](../include/flashvm/execution.h#L521).

Операнды остаются в корнях нативного вызова до завершения преобразований,
getter/setter и удаления. Результаты заменяют существующие элементы стека без
выделения памяти после снятия временных корней сервиса. Ошибка возвращается через
общий эпилог машинного блока. Уже выполненные записи и побочные эффекты callback
не откатываются, как и при интерпретации.

Тест `jit_references` сравнивает реальное нативное выполнение с интерпретатором
для host и SWF4–8; счётчик входов подтверждает запуск машинного кода. Проверяются
объявления, запись, доступ к свойствам, boxing примитива, удаление и пустой стек.
Пустые операнды сравниваются в независимых контекстах: пустой Delete2 в SWF7 может
удалить глобальную привязку `undefined`, изменив условия следующего вызова.

`jit_reference_callbacks` проверяет сборку мусора внутри setter, qualified-запись,
исключения и восстановление состояния GC/AVM1/нативного вызова.
`jit_reference_allocation` последовательно запрещает выделения памяти в пути
всех семи операций; проверяются корни и отсутствие активного code handle после отказа.

Шесть целевых тестов ограниченного ASan/UBSan проходят: три новых и существующие
jit_basic/jit_callbacks/jit_allocation. Stack ASan и LeakSanitizer отключены из-за
ограничений среды и недоступного `/proc`; проверки heap/global ASan и UBSan включены.
Настройки sanitizer заданы отдельным объектом только в каталоге сборки: передача
ASAN_OPTIONS дочерним процессам здесь ненадёжна. Конфигурация и журналы не коммитятся.

Полнота инструкций, оптимизирующий JIT, реальные запуски других CPU/ОС и полное
сравнение с Flash ещё не завершены. См. [JIT.md](JIT.md),
[JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md), [ROADMAP.md](ROADMAP.md).

Linux Release: прошли все 704 AGG, 704 Cairo и 685 minimal проверок. Итоговые
прогоны потребовали восстановления прав исполнения и целевого повтора одного
AGG, двух Cairo и семи minimal ELF. Это отказы запуска; ошибки поведения повтором
не скрывались. В процессе разработки также исправлена описанная выше ошибка
общего состояния в сравнительном тесте.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_delete_member` | function | [include/flashvm/execution.h:168](../include/flashvm/execution.h#L168) |
| `fvm_exec_delete_reference` | function | [include/flashvm/execution.h:521](../include/flashvm/execution.h#L521) |
| `fvm_exec_member` | function | [include/flashvm/execution.h:278](../include/flashvm/execution.h#L278) |
| `fvm_exec_variable_declare` | function | [include/flashvm/execution.h:566](../include/flashvm/execution.h#L566) |
| `fvm_exec_variable_set` | function | [include/flashvm/execution.h:542](../include/flashvm/execution.h#L542) |
<!-- source-links:end -->
