# Бюджет обхода интерфейсов — v0.381

В загруженном [fvm_exec_instance_of](../include/flashvm/execution.h#L308) снят отказ после 4096 посещённых узлов
интерфейсов. Линейный поиск в visited-vector заменён hash map по идентичности
объектов. Сравнивается полный указатель, поэтому коллизии hash и усечение
его адресных битов на LLP64 не дают ложного совпадения. Для выбранных портов
size_t должен вмещать указатель; это проверяется при компиляции. Типичный поиск
посещённого узла имеет постоянную стоимость; гарантии худшего случая нет.

Учёт начинается перед приведениями loaded-вызова и использует общий счётчик
AVM1, включая вложенные скрипты и нативные операции. Внешний прямой вызов
сбрасывает счётчик; вложенный сохраняет его. До обработки оплачиваются каждый
обычный прототип, извлечение pending-узла, включая повторный, и ребро интерфейса.
Исчерпание бюджета и насыщение unsigned-счётчика возвращают [FVM_ERROR](../include/flashvm/flashvm.h#L45) без
изменения выхода. Нулевой instruction_limit означает отсутствие лимита.
Проверка остановки worker сохраняет прежнюю политику Array: раз в 64 единицы.
Прежнее имя Array scope теперь является alias общего механизма; стоимость
операций Array намеренно не меняется.

[GC:] Прежние корни аргументов и прототипа сохраняют граф при boxing и настоящем
GC в callbacks. После преобразований обход не вызывает скрипт или сборку;
ключи и pending-указатели заимствуют ссылки из удерживаемого графа. При OOM
частичные hash/stack освобождаются; глубина исполнения восстанавливается при
успехе, исчерпании бюджета, ошибке callback и исключении выделения.

Ограничение обычной глубины прототипов сохраняется. Интерфейсные рёбра не
обходят обычное наследование своих узлов. Frozen prototype edges и прежние
constructor edges сохраняют трактовку. Незагруженный host сохраняет строгий
class API. ABI/wire не меняются. Другие сканирования свойств и нативные helper
этим изменением не получают новый учёт работы.

Проверки: instance_of_large_graph — SWF4–8, true/false на циклических графах
из 8192 интерфейсных узлов, GC, alias, finite/unlimited/shared бюджеты и
насыщение счётчика; instance_of_large_graph_allocation — все выделения stack/hash,
сохранение выхода, корни/глубина, GC/retry; jit_large_interfaces — 4097 узлов,
ошибка общего бюджета и повтор через интерпретатор, явный и автоматический JIT
в SWF4–8. Прежние callback/boxing/interface/Array регрессии остаются обязательными.
Все девять стадий остаются открытыми.

Validation v0.381: all 792 AGG / 792 Cairo / 773 minimal Linux Release checks
have passing evidence; AGG/Cairo passed initially, minimal required one ELF
execution-permission repair and targeted rerun. Eight restricted heap/global
ASan+UBSan checks pass: three new graph/native checks, existing membership
callbacks/allocation/data and large-sort/negative-end Array regressions.
Stack ASan and LeakSanitizer remain disabled. The new native polling and revised
membership constants headers pass a strict C89 syntax probe. No new original
external SWF oracle or foreign-OS acceptance is claimed. All nine stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_exec_instance_of` | function | [include/flashvm/execution.h:308](../include/flashvm/execution.h#L308) |
<!-- source-links:end -->
