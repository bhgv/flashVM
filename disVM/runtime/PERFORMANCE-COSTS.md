# Журнал измеренных замедлений DisVM

Здесь сохраняем пути, которые прибавили время, чтобы отдельно разбирать их после
завершения протоколов многопоточности. Статус всех записей ниже: **открыто**.
Метки `[PERF ID]` стоят рядом с соответствующими функциями; поиск:
`rg '\[PERF ' disVM/runtime`. Новые подтверждённые замедления добавляем сюда со
своим ID, условиями сравнения, исходными результатами и командой воспроизведения.

Это журнал затрат, а не автоматическое профилирование во время работы IDE.
Причины ниже — кандидаты для разбора, если нет отдельного измерения компонента.
Защиты нельзя снимать ради результата замера: оптимизация должна сохранять
проверенные владение, корни, порядок публикации и очистку ошибок.

## Измеренные сравнения

Все замеры локальные, Linux/x64, один и тот же образ программы внутри пары.
Для сравнений VM время включает запуск процесса; после прогрева бралась медиана трёх запусков.
Это не время одной инструкции, не скорость IDE и не доказательство для других
машин. У старых записей индивидуальные отсчёты не сохранены; JSON содержит медианы.
SMALL-097 дополнительно сохраняет три отсчёта каждой стороны защищённого парного
сравнения; сравнение полного флага с default по-прежнему хранит только медианы. Историческое сравнение MOVP осталось только
в округлённом виде. Проценты между строками **не складывать**: нагрузки различны.

| ID / версия | Нагрузка и база сравнения | C: база → защита | JIT: база → защита | Результаты |
|---|---|---|---|---|
| MOVP-088 / v0.1786 | 20000 обменов String; менялось только старое/новое тело MOVP в одинаковых объектах runtime | ≈0,131 → 0,135 с; ≈+3% | ≈0,118 → 0,130 с; ≈+10% | [Исторический протокол](SHARED-POINTER.md#cost-and-reproduction); округлённые значения |
| COPIES-089 / v0.1787 | 20000 итераций word/raw-record; default против полного `--guarded-copies` того выпуска | 0,228893 → 0,262788 с; +14,81% | 0,157588 → 0,218018 с; +38,35% | [JSON](performance/copies-runtime089.json), [контракт](SHARED-BYTES.md) |
| MAPPED-091 / v0.1789 | 20000 присваиваний записей со String; default против полного защищённого профиля | 0,140093 → 0,168638 с; +20,38% | 0,104591 → 0,141706 с; +35,49% | [JSON](performance/mapped-runtime091.json), [контракт](SHARED-MAPPED.md) |
| HEAD-092 / v0.1790 | 20000 итераций чтения головы списка; default против полного защищённого профиля | 0,105578 → 0,143605 с; +36,02% | 0,098950 → 0,152425 с; +54,04% | [JSON](performance/head-runtime092.json), [контракт](SHARED-HEAD.md) |
| LISTP-093 / v0.1791 | 20000 итераций: два HEADP и один TAIL; default против полного защищённого профиля | 0,123102 → 0,155452 с; +26,28% | 0,118057 → 0,160638 с; +36,07% | [JSON](performance/list-pointer-runtime093.json), [контракт](SHARED-LIST-POINTER.md) |
| LISTB-094 / v0.1792 | 20000 итераций пяти HEADB/W/L/F/M; default против полного защищённого профиля | 0,130036 → 0,241494 с; +85,71% | 0,091800 → 0,224481 с; +144,53% | [JSON](performance/list-bytes-runtime094.json), [контракт](SHARED-LIST-BYTES.md) |
| INT-095 / v0.1793 | 20000 итераций BYTE/WORD/LONG конвертаций; default против полного защищённого профиля | 0,174223 → 0,191245 с; +9,77% | 0,126488 → 0,167298 с; +32,26% | [JSON](performance/integer-runtime095.json), [контракт](SHARED-INTEGER.md) |
| ARITH-096 / v0.1794 | 20000 итераций 15 явных ADD/SUB/AND/OR/XOR B/W/L плюс проверки/цикл; default против полного защищённого профиля | 2,514193 → 3,296930 с; +31,13% | 1,647545 → 3,721847 с; +125,90% | [JSON](performance/arithmetic-runtime096.json), [контракт](SHARED-ARITHMETIC.md) |
| SMALL-097 / v0.1795 | 20000 итераций того же семейства с компактной защитой; полный guarded против default | 2,776730 → 2,885887 с; +3,93% | 1,905204 → 3,281297 с; +72,23% | [JSON](performance/arithmetic-small-runtime097.json), [контракт](SMALL-RANGES.md) |
| MULDIV-098 / v0.1796 | 20000 итераций 24 явных целочисленных операций, включая MUL/DIV/MOD, плюс цикл/проверки; полный guarded против default | 4,038585 → 4,380781 с; +8,47% | 2,460639 → 4,385723 с; +78,24% | [JSON](performance/product-division-runtime098.json), [контракт](PRODUCT-DIVISION.md) |

| SHIFT-099 / v0.1797 | 20000 итераций шести явных SHL/SHR плюс цикл/проверки; полный guarded против default | 0,634960 → 0,715761 с; +12,73% | 0,443583 → 0,755794 с; +70,38% | [JSON](performance/shifts-runtime099.json), [контракт](SHARED-SHIFTS.md) |

| BRANCH-101 / v0.1799 | 20000 итераций 18 явных условных переходов, массивы вне цикла; полный guarded против default | 1.895569 → 1.989876 с; +4.98% | 1.272985 → 1.985966 с; +56.01% | [JSON](performance/branches-runtime101.json), [контракт](SHARED-BRANCHES.md) |

| ARRAYLEN-102 / v0.1800 | 20000 динамических LENA, массивы вне цикла; полный guarded против default | 0.162826 → 0.175156 с; +7.57% | 0.130670 → 0.174871 с; +33.83% | [JSON](performance/array-length-runtime102.json), [контракт](ARRAY-LENGTH.md) |
| LISTLEN-103 / v0.1801 | 20000 длин заранее созданных списков31/32, checksum630000; полный guarded против default | 0,165363 → 0,246310 с; +48,95% | 0,135551 → 0,247596 с; +82,66% | [JSON](performance/list-length-runtime103.json), [контракт](LIST-LENGTH.md) |
| STRINGLEN-104 / v0.1802 | 20000 длин заранее созданных ASCII/Rune-строк31/32, checksum630000; полный guarded против default | 0,165301 → 0,183149 с; +10,80% | 0,135974 → 0,189321 с; +39,23% | [JSON](performance/string-length-runtime104.json), [контракт](STRING-LENGTH.md) |
| STRINGCMP-105 / v0.1803 | 20000 итераций шести сравнений заранее созданных ASCII/Rune-строк; полный guarded против default | 0,439698 → 0,548159 с; +24,67% | 0,310993 → 0,541777 с; +74,21% | [JSON](performance/string-branch-runtime105.json), [контракт](STRING-BRANCHES.md) |
| REALBRANCH-106 / v0.1804 | 20000 итераций шести REAL-сравнений; полный guarded против default | 0,389042 → 0,438889 с; +12,81% | 0,224471 → 0,451894 с; +101,32% | [JSON](performance/real-branch-runtime106.json), [контракт](REAL-BRANCHES.md) |

| REALARITH-108 / v0.1807 | 20000 итераций четырёх REAL-операций и четырёх накоплений, checksum305000; полный guarded против default | 0,360478 → 0,490139 с; +35,97% | 0,239290 → 0,573447 с; +139,65% | [JSON](performance/real-arithmetic-runtime108.json), [контракт](REAL-ARITHMETIC.md) |

| REALCONVERT-110 / v0.1809 | 20000 итераций четырёх FP-конвертаций, два REAL-сложения и цикл/проверки; checksum140000; полный guarded против default | 0,357928 → 0,472416 с; +31,99% | 0,236250 → 0,482701 с; +104,32% | [JSON](performance/real-conversion-runtime110.json), [контракт](REAL-CONVERSION.md) |

У восемнадцати последних VM-записей флаг включает и другие защищённые операции. Поэтому
долю MOVMP, HEADMP, отдельных блокировок или allocator по этим числам выделить
нельзя. Защищённые копирования пока включаются явно; MOVP и SELF имеют свой
проверенный путь независимо от этого флага. NativeWorld всё ещё необходим.

## Пути и очередь разбора

| ID | Отмеченные функции/пути | Что известно из реализации | Что ещё измерить и проверить |
|---|---|---|---|
| MOVP-088 | [shared_pointer.c](shared_pointer.c): `dvm_runtime_movp_copy`; [cell_access.c](../host/cell_access.c); x64 pointer tail | Захват ячеек, retain/mark, публикация и снятие защиты; begin/end не выделяют память | Разделить время guard/RC/mark/C-tail; проверить пользу доказанной локальности |
| COPIES-089 | [shared_bytes.c](shared_bytes.c): `dvm_runtime_bytes_copy`; [range_access.c](../host/range_access.c); [bytes_fast.c](bytes_fast.c) | Выбор объединения stripes, их захват/снятие и переход в C; больших копий может касаться весь домен | Разнести малые/большие копии, коллизии и стоимость C-tail; не приписывать всё memmove |
| MAPPED-091 | [shared_mapped.c](shared_mapped.c): `dvm_runtime_mapped_copy`; [root_batch.c](root_batch.c) | Для непустой карты ссылок — пять native-блоков подготовки, снимок, обход ссылок, roots и retirement | Счётчики/время allocator, snapshot, retain/mark, root mutex и destructor; проверить безопасное повторное использование workspace при вложенных callbacks и ошибках |
| HEAD-092 | [shared_head.c](shared_head.c): `dvm_runtime_guarded_headmp`; [list_hold.c](list_hold.c): `dvm_runtime_list_hold` | Два блока временного корня списка плюс до пяти блоков mapped-копирования; проверка размера/карты и дополнительное удержание списка | Отдельно стоимость hold/root/проверок и payload-копирования; проверить workspace/reuse и локальные варианты без ослабления жизни исходного списка |
| LISTP-093 | [shared_list_pointer.c](shared_list_pointer.c): `dvm_runtime_list_pointer_copy`; [list_hold.c](list_hold.c); [shared_pointer.c](shared_pointer.c) | Два native-блока корня списка на проекцию, защита исходного узла и внутреннего поля, проверка extent/карты, retain/mark и cleanup | Разделить hold/root и MOVP; сравнить HEADP и TAIL отдельно, проверить reuse/locality без потери времени жизни контейнера |

Число блоков выше — подтверждённый путь аллокаций, **не измеренная доля времени**.
Возможная экономия от reuse, locality или inline JIT пока не измерена. Reuse
потребует отдельной проверки вложенных вызовов, отказов, очистки и многопоточности;
один общий временный буфер для всех исполнителей не подходит.

## Воспроизведение

Использовать соответствующую историческую версию, её согласованные runtime,
компилятор и native-cell образ; запускать без параллельной тестовой нагрузки.
На новых версиях флаг может охватывать больше операций — это уже новый замер.

```sh
python3 disVM/runtime/benchmark_shared_pointer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_guarded_copies.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_mapped.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_head.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_list_pointer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_list_bytes.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_integer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_small_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_product_division.py RUNTIME LIMBO INCLUDE_DIR
```

| ID | Исходный опубликованный commit |
|---|---|
| MOVP-088 | [7fd2566](https://github.com/bhgv/flashVM/commit/7fd2566c0ac7075157368399972296c08062c899) |
| COPIES-089 | [c166c24](https://github.com/bhgv/flashVM/commit/c166c243be8cc84604fc648cb39df91384b34638) |
| MAPPED-091 | [95f3274](https://github.com/bhgv/flashVM/commit/95f32743d9c2c28486854cd610313316a14fc7b5) |
| HEAD-092 | [20f8de7](https://github.com/bhgv/flashVM/commit/20f8de760ffbf4ecd5a20276b668f105c3f6caeb) |
| LISTP-093 | Commit добавления [v0.1791](../../docs/releases/v0.1791.md), также указан в отчёте FLASHVM-0109 |

Для MOVP нужны совпадающие GNU/CMake объекты для приватной перелинковки старого
тела. Остальные команды используют один binary/image и чередуют startup-профили.
При недоступном /tmp задавать TMPDIR. Для нового результата сохранять среду,
commit, команду, сырые отсчёты и медианы в новой записи; исторические файлы не
перезаписывать. Закрывать запись только после сравнимого повторного замера и
проверки сохранения семантики, не по одному изменению кода.

LISTB-094: v0.1792 / FLASHVM-0110. Пять проекций на итерацию; каждая готовит
два native-блока корней до блокировок. Доля allocator/guard/GC отдельно не измерена.

INT-095: v0.1793 / FLASHVM-0111. Конвертация не готовит корни или scratch-буфер.
Возможные затраты: preflight, выбор полос, блокировки и C-вход JIT; их доли
не измерены. Полный профиль включает также другие защищённые копирования.

ARITH-096: v0.1794 / FLASHVM-0112. Новых per-call аллокаций нет. Общий range-путь
готовит 256-entry metadata, проверяет нулевое состояние и просматривает таблицу
полос. Компактная защита трёх малых ячеек — кандидат для оптимизации; отдельная
стоимость сканирования/блокировок/C-входа ещё не измерена. Полный профиль меняет
также операции цикла, проверок и преобразований. Проценты с INT-095 не складывать.

SMALL-097: v0.1795 / FLASHVM-0113. ARITH-096 частично смягчён, запись остаётся
открытой. В отдельном парном сравнении ОБЕ стороны защищены: C generic 3,406701 →
compact 2,703037 с (время меньше на 20,66%); JIT 3,421905 →3,031322 с (меньше на 11,41%).
Это один образ и совпадающие объекты; приватно меняются только metadata и имена
реальных protected begin/end. Сырые отсчёты сохранены и заметно варьируют.
Не выводить универсальное ускорение и не вычитать эти проценты из ARITH-096: его
измерение было другой парой запусков. На локальном native64 metadata 72 вместо 2072
байт. Стоимость оставшихся preflight/guard/C-tail путей отдельно не измерена.

MULDIV-098: v0.1796 / FLASHVM-0114. Новый workload содержит 24 явных операции
и непустые делители, поэтому отдельная стоимость zero-error/cleanup не измерена.
Preflight, выбор семейства, guarded divisor-read, 6-key guards и C-вход JIT остаются
кандидатами для разбора, без измеренной доли каждого. Исторические 15-op программы
зафиксированы побайтово; их JSON/условия не перезаписаны и проценты не складываются.

SHIFT-099: v0.1797 / FLASHVM-0115. Все счётчики допустимы; стоимость ошибок
не измерена. Проверка ACTIVE, mixed-width геометрии, compact guards, чтение WORD
счётчика и C-tail остаются кандидатами без выделенной доли времени. Нагрузка
включает другие защищённые операции цикла/проверок; не приписывать весь прирост
сдвигам и не складывать проценты с предыдущими программами. Исторические JSON
не перезаписаны. Воспроизведение: benchmark_shared_shift.py RUNTIME LIMBO INCLUDE.

BRANCH-101: v0.1799 / FLASHVM-0117. Массивы выделяются до цикла; на каждой
итерации меняются значения и выполняются 18 явных BYTE/WORD/LONG условий.
Нагрузка также содержит обновления счётчика, копирования/арифметику и служебные
условия. Это стоимость полного режима, не отдельной инструкции. Все адреса
переходов допустимы: ошибка/очистка отдельно не измерена. ACTIVE/геометрия,
компактное объединение, два чтения, условный адрес и C-tail остаются кандидатами
без измеренной доли каждого. Старые JSON не изменены; проценты не складывать.
Воспроизведение: benchmark_shared_branch.py RUNTIME LIMBO INCLUDE_DIR.

ARRAYLEN-102: v0.1800 / FLASHVM-0118. Ссылка выбирается из двух заранее
созданных массивов длиной31/32; checksum630000. Внутри цикла нет выделений
массивов, но есть индексация, MOVP, сложение и условия. Это полный режим,
а не отдельная стоимость LENA или extend. Непрерывный source guard, два
этапа отбора stripes, try/CAS, metadata fallback и C-tail остаются кандидатами
без измеренной доли каждого. Все поля допустимы: ошибки/rollback отдельно
не измерены. Не сравнивать с несуществующим rooted-array baseline и не
складывать проценты с другими нагрузками. Исторические JSON сохранены.
Воспроизведение: benchmark_array_length.py RUNTIME LIMBO INCLUDE_DIR.

LISTLEN-103: v0.1801 / FLASHVM-0119. Две цепочки31/32 выделены до цикла;
20000 длин, checksum630000. Нагрузка включает выбор списка, MOVP, арифметику
и условия, поэтому это стоимость полного режима, не изолированного LENL.
Непрерывные claims всех посещённых tail, сортированное расширение, отбор stripes,
try/CAS, metadata fallback, проверка циклов и C-tail остаются кандидатами без
измеренной доли каждого. Ошибки/rollback отдельно не измерены. Для длинных
цепочек возможен охват всех256 stripes и конфликт с посторонними объектами.
Нет rooted-list альтернативы для сравнения; старые JSON не перезаписаны,
проценты между нагрузками не складывать.
Воспроизведение: benchmark_list_length.py RUNTIME LIMBO INCLUDE_DIR.

STRINGLEN-104: v0.1802 / FLASHVM-0120. Четыре ASCII/Rune-строки длиной31/32
выделены до цикла;20000 длин, checksum630000. Полный guarded включает индекс,
MOVP, арифметику и условия. Медианы после прогрева/трёх чередующихся запусков
не выделяют стоимость LENC, int→WORD, extend или C-tail. Отбор stripes, два этапа
try/CAS и metadata fallback остаются кандидатами без измеренной доли каждого.
Ошибки/rollback и rooted-string альтернативы не измерены. Не суммировать
проценты других нагрузок; исторические JSON сохранены.
Воспроизведение: benchmark_string_length.py RUNTIME LIMBO INCLUDE_DIR.

STRINGCMP-105: v0.1803 / FLASHVM-0121. Четыре короткие ASCII/Rune-строки
созданы до20000 итераций; шесть условий на каждой, checksum60000. Полный
защищённый профиль включает индекс/MOVP/арифметику и управление циклом.
Защита двух владельцев, расширение на заголовки/префиксы, сортировка stripes,
try/CAS, metadata fallback и C-tail — кандидаты без выделенной доли каждого.
Длинные префиксы могут занять все256 полос; данный замер их цену не измеряет.
Ошибка/rollback/GC и rooted-string альтернатива отдельно не измерены. Старые
JSON не изменены; проценты разных нагрузок не складывать.
Воспроизведение: benchmark_string_branch.py RUNTIME LIMBO INCLUDE_DIR.

REALBRANCH-106: v0.1804 / FLASHVM-0122. Четыре REAL выделены до20000
итераций; шесть сравнений на каждой, checksum60000. Входная пара и target
читаются в разных интервалах, FP вычисляется между ними без guards.
Полный профиль включает индекс/MOVP/арифметику/цикл. Обнуление метаданных,
первый и условный второй begin/end, try/CAS/fallback и C-tail остаются
кандидатами без выделенной доли каждого. Данная нагрузка конечная: особые
значения/флаги проверены функционально, но цена ошибок/ловушек отдельно не
измерена. Другой компактный/rooted вариант не сравнивался. Исторические JSON
не изменены; проценты разных нагрузок не складывать.
Воспроизведение: benchmark_real_branch.py RUNTIME LIMBO INCLUDE_DIR.


## ROOTREPLACE-107 — порядок снятия группы, отдельный native-замер

v0.1806 / runtime0.107. Метка стоит в `dvm_runtime_root_batch_replace`.
12000 замен заранее зарезервированной группы из128 одинаковых ключей; один
поток, реальный registry/ports, native и forced-lock backend. Отсчёт только
вокруг цикла API, без компиляции/старта/резервирования/clear. Обе стороны
сохраняют один атомарный интервал: сравниваются прямой и обратный порядки снятия
из этой же серии, а НЕ опубликованные старый/новый runtime или полный guarded.
После прогрева три чередующихся отсчёта, медианы:

| Backend | Прямой порядок | Обратный порядок | Отношение обратный/прямой |
|---|---:|---:|---:|
| native | 0,110665902 с | 0,006714688 с | 0,06068 |
| forced-lock | 0,114079031 с | 0,008183073 с | 0,07173 |

[Все исходные отсчёты и границы](performance/root-replace-runtime107.json).
Прямой порядок многократно обходит цепочку собственной группы; обратный
снимает её начиная с головы. Стоимость коллизий с другими группами/обычными
корнями, конкуренции, allocator подготовки, GC, JIT и IDE отдельно не измерена.
Это не коэффициент ускорения VM и не новый замер полного флага. Исторические
результаты выше не меняются. Следующая очередь: чужие коллизии, смешанные ключи
и длительность root-mutex при конкуренции, без ослабления атомарности.
Воспроизведение: `python3 disVM/runtime/benchmark_root_replace.py`.


REALARITH-108: v0.1807 / FLASHVM-0125. Два REAL в массиве выделены до цикла,
20000 итераций; четыре операции плюс четыре накопления, checksum305000.
Полный профиль также включает индекс/MOVP/цикл/проверки. Медианы после прогрева
и трёх чередующихся запусков включают запуск процесса; индивидуальные отсчёты
этого общего драйвера не сохранены. Два begin/end, обнуление метаданных, выбор
stripes, try/CAS/fallback, сохранённые байты и C-tail — кандидаты без выделенной
доли каждого. Ошибки, FP-ловушки, GC и contention отдельно не измерены.
Нельзя приписывать весь прирост FP или складывать его с другими строками.
ROOTREPLACE-107 выше остаётся отдельным native-экспериментом, не полным флагом.
Воспроизведение: benchmark_real_arithmetic.py RUNTIME LIMBO INCLUDE_DIR.


## REALDISPATCH-109 — завершённый dispatch против прежнего fallback

v0.1808 / FLASHVM-0126. Исправлены четыре отсутствовавших case REAL. Старый
REALARITH-108 JSON не изменён: JIT там входил в tail, получал отказ и выполнял
интерпретаторный запасной путь. Прежний замер не доказывал завершённый C-tail.

Новый парный эксперимент использует одинаковые runtime-объекты/флаги/guarded
профиль и guest:20000 итераций, checksum305000. Меняется только private-view
bytes_fast.c: отсутствие четырёх case против исправленного dispatch. Прогрев
и три чередующихся запуска; сохранены все отсчёты и SHA обеих версий исходника.
Время включает запуск процесса. C — контроль, поскольку эти tails не использует.

| Режим | Прежний fallback | Исправленный dispatch | Изменение медианы |
|---|---:|---:|---:|
| C, контроль | 0,513587309 с | 0,527325184 с | +2,67% |
| JIT | 0,552045879 с | 0,521206303 с | −5,59% |

[JSON с исходными отсчётами](performance/real-dispatch-runtime109.json). Разброс
отсчётов и контрольный сдвиг не позволяют приписать весь результат диспетчеру.
Это не оценка ускорения VM/IDE, не замер полного флага против default и не
изолированная стоимость FP/locks/tail. Подготовка, guard metadata/try/CAS/fallback,
ошибки/GC/contention остаются кандидатами для отдельного разбора. Исторические
данные не перезаписаны. Воспроизведение: benchmark_real_dispatch.py RUNTIME LIMBO INCLUDE_DIR.


REALCONVERT-110: v0.1809 / FLASHVM-0127. Массивы int/big/real выделены до
20000 итераций. Четыре реально генерируемых конвертации CVTWF/LF/FW/FL, два
REAL-сложения и обычные индекс/цикл/проверки; checksum140000. Это полный
startup guarded против default, не изолированный conversion benchmark. Прогрев
и три чередующихся запуска, все отсчёты сохранены вместе с медианами; запуск
процесса включён. Конвертация SREAL здесь не исполняется. Два begin/end, размеры
stack metadata/сохранённых байтов, выбор stripes/try/CAS/fallback и C-tail —
кандидаты без выделенной доли каждого. Ошибки/FP-ловушки/GC/contention отдельно
не измерены. Исторические JSON не меняются; проценты разных нагрузок не складывать.
Воспроизведение: benchmark_real_conversion.py RUNTIME LIMBO INCLUDE_DIR.


## HEAPHOLD-111 — общий rooted lease вместо list-specific тела

v0.1810 / FLASHVM-0128. Парное сравнение одного защищённого профиля, без
изменения остальных matching runtime-объектов: старый list_hold.c из v0.1809
против нового адаптера к heap_hold.c. Generic core связан, но не вызывается
старой стороной. SHA baseline проверяется до компиляции; SHA обеих реализаций
и core, прогрев/три чередующихся отсчёта сохранены.20000 HEADMP-проекций,
checksum `19999 reference 7`; время включает запуск процесса и остальную работу.

| Режим | Прежнее тело | Общий lease | Изменение медианы |
|---|---:|---:|---:|
| C | 0,131860223 с | 0,131209015 с | −0,49% |
| JIT | 0,158161230 с | 0,140885346 с | −10,92% |

[Все отсчёты и SHA](performance/heap-hold-runtime111.json). Диапазоны отсчётов
перекрываются: это не доказанный коэффициент ускорения. Нагрузка включает
mapped-копирование, retain/mark/roots/retirement и цикл; стоимость дополнительного
вызова, новых общих проверок, root mutex, allocator и Fast не выделена. Два native
блока root reservation на capture сохранены. Эти пути остаются кандидатами для
разбора, не приписывать результат allocator/GC и не складывать проценты. Это
не новый full-guarded/default замер; прежние восемнадцать VM-записей не меняются.
Воспроизведение и точная extraction baseline: [HEAP-HOLD.md](HEAP-HOLD.md).


## HEAPCOMMIT-112 — только native-цикл подготовленных успешных commit

v0.1811 / FLASHVM-0129.20000 переносов существующих владельцев между двумя
заранее подготовленными значениями и реальным rooted Tptr-контейнером. Корни/
lease подготовлены до CLOCK_MONOTONIC; displaced-lease попеременно переиспользуется
как следующий incoming. Захват, аллокации, GC/retirement, запуск процесса и guest
не входят в отсчёт. В конце проверены точные owner/Type checksum и настоящий Fast.

После прогрева медиана трёх чередующихся запусков: atomic-poll0,004974447 с,
locked-poll0,005203334 с. [Все отсчёты и SHA](performance/heap-commit-runtime112.json).
Это одна и та же native C-операция с двумя настройками collector poll, не два JIT/
атомарных backend и не измерение сравнительной стоимости polling. Старой базы
сравнения нет. Нагрузка тёплая/успешная, NativeWorld всё время ACTIVE, без смены
GC-эпох в цикле. Время отдельных geometry/preflight/native borrow/guard/mark/root
mutex/retarget не выделено; stale/refusal/rollback/contention и полный конструктор
остаются отдельными кандидатами для измерения. Это не скорость IDE/VM и не
новый full-guarded/default замер; восемнадцать старых VM-записей не изменяются.
Воспроизведение: benchmark_heap_commit.py RUNTIME LIMBO INCLUDE_DIR.

## PREPARE-113 — новый путь подготовки, исторической пары пока нет

`pointer_prepare.c` резервирует группу корней (две native allocation), затем
использует прежние retain/Type/poolalloc/ordinary-root/monitor стадии. После
link+mark добавляется публикация candidate root и снятие construction pin.
При stale добавляются finish, повторный hold и полная перестройка кандидата.
Это известные дополнительные пути, а не измеренный процент замедления.
Старый CONSP сохраняет прежний путь, только общий stage вынесен в отдельный файл.
Отдельный matched benchmark и длительность удержания NativeWorld ещё не измерены;
скорость IDE и параллельность из этих проверок не следуют.

## CONSP-114 — matched цена подключения конструктора

v0.1813 / FLASHVM-0131.5000 CONSP с немедленным снятием головы, строка sample,
checksum30000 и пустой итоговый список. Одна и та же текущая сборка, программа,
--guarded-copies/--gc-worker/quantum2000; в приватных бинарниках различается только
pointer_builder.c: точный опубликованный runtime0.113 против нового dispatcher.
Остальные новые объекты связаны в обоих случаях. Это сравнение конструктора
в одинаковом защищённом профиле, **не** полный профиль против default; счётчик
восемнадцати исторических сравнений не увеличиваем. Запуск процесса, цикл,
планирование и реальное освобождение включены. Изолированная цена инструкций,
шести calloc, guards, барьеров и root mutex отдельно не измерена.

После прогрева три чередующихся отсчёта финального исходника:
C медиана0.057399231 →0.072251189s (наблюдаемое +25.87%);
JIT медиана0.066887672 →0.069530421s (+3.95%). Это локальный Linux/x64 результат
короткой программы, не универсальный процент и не скорость IDE. Сырые отсчёты,
source SHA и checksum: [финальный JSON](performance/pointer-transaction-runtime114.json).
[Первый прогон](performance/pointer-transaction-runtime114-initial.json) сохранён
отдельно: до уточнения отказа неверного admission в dispatcher, C+28.57% и
JIT+16.28%. Времена этих коротких запусков заметно гуляют; нельзя выбирать один
из прогонов как доказательство стабильной цены или суммировать проценты.

Путь: source hold + tail hold + candidate reservation (шесть native allocation),
старый managed build/monitor, guarded identity/mark/store/root-retarget, полное
retirement вне guards. Stale добавляет уничтожение кандидата, свежий tail hold и
повторную полную подготовку. Подбор шести резервов/переиспользование metadata,
размерные категории пулов и локальные варианты — кандидаты для дальнейшей
оптимизации без ослабления владения. Stale/contention/destructor-error timing
отдельно не измерены. Команда: benchmark_pointer_transaction.py runtime compiler
include exact-runtime113-pointer_builder.c (ожидаемый SHA встроен в runner).

## VALUEPREP-115 — подготовка скаляров/raw, без matched замера

v0.1814 / FLASHVM-0132. Новый value_prepare.c резервирует одну root group
(две native allocation), получает extra tail/Tlist owner, выделяет node,
копирует частный снимок payload, вызывает monitor под construction pin,
связывает/маркирует tail, публикует candidate root и снимает временный pin.
Stale требует finish/rebuild. Это перечень путей, а не измеренный процент.
NativeWorld сохраняется. Ни C-опкоды скалярных/raw cons, ни прямой scalar JIT
пока не переключены: стоимость действующих опкодов этой серией не изменяется.
Отдельный matched замер после их согласованного подключения ещё предстоит.

## SCALARCONS-116 — подключены четыре scalar opcode, matched замер впереди

v0.1815 / FLASHVM-0133. Selected CONSB/W/L/F добавляет source span capture,
четыре native allocation для tail/candidate root groups, prepare/mark/commit и
полное retirement. Stale требует finish, fresh tail hold и rebuild. В x64 прямой
allocator/copy заменён tail JMP в общий C coordinator, без RX return PC.
Это реальные дополнительные пути, пока без matched исторического измерения;
проценты CONSP-114 к ним не переносим. Unselected/inline opcode bodies сохранены.
Нужно отдельно измерить группы allocator/guard/root/C-tail, не ослабляя протокол.

## RAWCONS-117 — captured CONSM, исторический matched замер ещё не выполнен

v0.1816 / FLASHVM-0134. Добавлены отдельные size/source guards, один malloc/free
для непустого native snapshot, копирование всего payload в snapshot, четыре
native allocation для tail/candidate root groups, prepare/copy/mark/commit и
retirement. Stale использует тот же snapshot, но повторяет tail hold/build/commit.
Snapshot освобождается после ВСЕХ lease даже при ошибках их деструкторов; zero
size не делает malloc и игнорирует source. Metadata+source захватываются отдельно,
текущая согласованность требует NativeWorld. В существующем aggregate x64 tail
остаётся JMP в C, новый allocator bypass не добавляется. Это перечень новых
затрат, не измеренный процент: CONSP/scalar timings к ним не переносим.

## MAPPEDPREP-118 / CONSMP-118 — typed snapshot и кандидат

v0.1817 / FLASHVM-0135. На обычном успешном пути CONSMP строятся ДВА managed
List node: traced snapshot с H tail и actual candidate. Каждая подготовка требует
root reservation(2 native blocks), retained Tlist/element Type и головы с H полями,
ordinary pin, actual mapped_copy(ещё5 native blocks для nonempty pointer map),
source/destination range guards, head retains/marks и root handoff до monitor.
Coordinator добавляет empty-tail hold(2 blocks), observed-tail hold(2 blocks) и
conditional commit/полный drain пяти lease. Для mapped heads это18 native
preparation blocks, плюс два managed node и возможные cache-dependent ordinary
pin blocks. При нулевой map scratch mapped-copy не создаётся. Stale сохраняет
snapshot, но повторяет candidate build/capture-tail/commit. Это известные пути,
не измеренный процент. Matched benchmark ещё не выполнен; CONSP/scalar/raw ratios
не переносим. Возможная оптимизация первого snapshot/candidate и reuse metadata
требует сохранения проверенных root/error/alias протоколов. NativeWorld остаётся.

## RECORDNEW-119 — local NEW/NEWZ

The selected worker path allocates a fresh record even when the legacy path
could reuse a unique plain record. Destination and candidate reservations add
four native metadata allocations. Stale retry refreshes the destination hold
and reuses the record. Matched timing is pending.

## MNEWZ-120

Imported records add a Modlink hold with two native root reservation allocations
and guarded capture of the Type index. Guarded record operations use C dispatch
from hosted leaf JIT sessions. Matched timing is pending.

## ARRAYNEW-121

Guarded array construction adds four native root reservation blocks and
allocates a fresh owning array. It retains the element and array header Types
independently. The x64 guarded leaf profile uses C for construction. A stale
comparison reuses the candidate. Matched timing is pending.

## CHANNELNEW-122

Guarded NEWC now reserves a root batch, holds the owning destination and
publishes by identity comparison. A buffered channel reports two allocations
after construction. Guarded JIT uses the C route. Measure buffered and
unbuffered construction separately in a matched run before attributing a
time difference to publication or dispatch. No matched timing is recorded.

## CHANDRAIN-122

Hosted close enters a VM error checkpoint for each nonempty detached waiter
so a failed wake cannot abandon the rest of the chain. Measure close latency
against queue length and repeated ALT registrations. No matched timing is
recorded.

## CHANMOVE-122

The hosted direct send and receive paths add an error checkpoint around the
mover after dequeue. Measure matched successful rendezvous latency with the
same payload type and scheduler mode. No matched timing is recorded.

## CHANREFILL-122

Hosted buffered RECV with a queued sender protects the temporary channel
release separately from the transfer. Measure a matched full-ring rendezvous
with a waiting sender. No matched timing is recorded.

## ALTPREFLIGHT-122

Matching a blocked ALT now walks the affected guard queues before dequeue.
Measure rendezvous latency as guard count and queue length grow, including
duplicate registrations and receive arrays. No matched timing is recorded.

## CHANCLOSESCAN-122

Hosted close scans the closing chain and each blocked ALT's guard queues
before detachment. Duplicate ALT registrations can repeat validation.
Measure matched close latency across guard and waiter counts. No matched
timing is recorded.

## CHANFINALIZE-122

Hosted finalization validates both chains before detaching either and
protects buffer release separately. Measure matched finalization latency
with waiters on both directions. No matched timing is recorded.

## CHANCLAIM-122

Hosted matched SEND and RECV reserve a waiter and add an outer error
checkpoint around dequeue, transfer and wake. Measure successful and
failed rendezvous with and without ALT. No matched timing is recorded.

## CHANCLOSECLAIM-122

Hosted close checks and claims waiters before detaching the queue owners.
Measure the extra scans with waiters in both directions and duplicate ALT
registrations. No matched timing is recorded.

## CHANQUEUEHOLD-122

Hosted channel queue publication and removal update a Prog hold per node.
Measure matched registration and cancellation with repeated ALT guards.
No matched timing is recorded.

## CHANQUEUEMUTEX-122

Hosted queue insertion and removal now use a dedicated mutex and an error
checkpoint. Measure the cost by queue length, including repeated ALT
registrations. No matched timing is recorded.

## CHANREADCLOSELOCK-122

Hosted SEND and RECV take the queue mutex for readiness and head reads.
Close holds it through preflight, claims and detachment, with wakeups
outside the lock. No matched timing is recorded.

## CHANBUFFERLOCK-122

Hosted C buffered transfers lock ring validation and reservation, with
movers outside the lock. Measure against equivalent C and JIT profiles.
No matched timing is recorded.
