# Бюджет объявления интерфейсов — v0.383

Загруженный [fvm_exec_implements](../include/flashvm/execution.h#L222) для SWF7–8 теперь использует общий бюджет AVM1.
До работы оплачиваются вход, каждое захватываемое значение интерфейса и каждое
проверяемое значение при подготовке snapshot. Полная обработка N аргументов
расходует 2N+1 единиц дополнительно к байткоду вызывающей стороны. Вложенный
вызов сохраняет счётчик; внешний прямой вызов сбрасывает его. Исчерпание бюджета,
насыщение счётчика и остановка worker дают [FVM_ERROR](../include/flashvm/flashvm.h#L43). Нулевой instruction_limit
остаётся неограниченным. Host-учёт и его constructor-reference модель не меняются.

[GC:] Аргументы удерживаются корнями при подготовке списка. Полный список
публикуется swap только после всех выделений и оплаченной обработки. OOM и
ошибка бюджета сохраняют прежние рёбра и флаг первой установки. Повторное
объявление сохраняет первый список; аргументы по-прежнему проверяются и
обрабатываются перед решением о публикации. Частичные корни/snapshots
освобождаются, глубина исполнения восстанавливается.

Валидированные loaded-запросы с нулевым count, примитивным constructor и SWF4–6
возвращают успех до выделения root/snapshot и входа в work scope. Недопустимые
значения, чужой owner, отсутствующий массив аргументов и публичный предел
проверяются до этой ветви. Отсутствие выделений относится к прямому C-сервису;
адаптер байткода может выделять собственный буфер операндов. Валидация входа и
инициализация root storage не получают новый поэлементный учёт; публичный и
decoder-предел 65 536 аргументов сохраняется.

Новые getters, boxing и coercions не добавляются. Форматы frozen/legacy edges,
первая пустая установка, правила membership, ABI и сериализация не меняются.
Полная Flash-сверка версий/player и смешанных movie открыта.

Проверки: implements_work — SWF4–8, все 65 536 ссылок, low/unlimited/saturated
бюджеты, отсутствие частичной установки, первая запись, GC и восстановление scope;
implements_work_allocation — все выделения, noops при нулевом allowance,
валидация чужих значений до пропуска операции, GC/retry;
jit_implements_work — ошибка до публикации и успешный повтор в интерпретаторе,
явном и автоматическом JIT, включая игнорируемые объявления SWF4–6.

Сопоставление: [ImplementsOp закреплённого Ruffle](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
Добавлен учёт ресурсов runtime; нового прогона оригинального Flash Player нет.
Все девять стадий остаются открытыми.

Validation v0.383: all 799 AGG / 799 Cairo / 780 minimal Linux Release checks
have passing evidence after five ELF execution-permission repairs and targeted
reruns (two AGG, three Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: three new declaration/work cases, existing
snapshot allocation/transport, membership data/large-graph and collecting
membership-callback regressions. Stack ASan and LeakSanitizer remain disabled.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_implements` | function | [include/flashvm/execution.h:222](../include/flashvm/execution.h#L222) |
<!-- source-links:end -->
