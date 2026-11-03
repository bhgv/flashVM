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
Время включает запуск процесса; после прогрева бралась медиана трёх запусков.
Это не время одной инструкции, не скорость IDE и не доказательство для других
машин. Полные индивидуальные отсчёты не сохранены; JSON содержит опубликованные
медианы, а не распределение выборки. Историческое сравнение MOVP осталось только
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

У семи последних записей флаг включает и другие защищённые операции. Поэтому
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
