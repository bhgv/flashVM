# Окна приёма уникальных частей

[English](RECEIVE_WINDOWS.md), [адаптация](CHANNEL_ADAPTATION.ru.md),
[части данных](MEDIA_SLOTS.ru.md). `smeasure.h` превращает явно проверенные
хостом поступления в существующий компактный отчёт tag 0. Это отдельный C89
модуль без сокетов, выделения памяти, часов, VM roots и глобального состояния.

## Время, идентичность и категории

Отдельный [FVM_SMeasure](../include/flashvm/smeasure.h#L20) хранит одно доверенное окно stream/epoch/window:
1..4096 ожидаемых частей и положительный относительный deadline в миллисекундах.
Хост задаёт индексы от нуля и монотонное время относительно того же начала,
что deadline, а не удалённый presentation timestamp или системное время.
Значения времени и идентификаторов Uint32; новое окно нужно до переполнения.
Пакет не может сам поменять expected или привязать новую epoch.

Таблица категорий фиксирована: 1024 байта, считаются уникальные части, а не
датаграммы. Проверенные пригодные данные до deadline включительно — on_time,
позже — late. Повреждённую часть можно записать лишь при независимо доверенном
индексе из внешнего конверта. Нельзя брать индекс из заголовка с неверной
контрольной суммой: без доверенного индекса часть остаётся отсутствующей.
Повреждения, отброшенные UDP/IP, тоже видны лишь как отсутствие при закрытии.

Повторы не увеличивают выборку. Исправленная вовремя часть меняет damaged на
on_time, исправленная поздно — на late. Поздний или повреждённый повтор не
ухудшает уже on_time; повреждённый повтор не ухудшает late. Категории не
пересекаются, missing = expected-on_time-late-damaged. Одни данные нельзя
повторно засчитывать с чужой идентичностью. Аутентификацию и проверку байтов
обеспечивает хост.

Закрывать окно можно лишь на/после deadline и последнего принятого времени.
Скопированный [FVM_SAdaptFeedback](../include/flashvm/sadapt.h#L39) замораживает историю: дальнейшая запись и
повторный finish возвращают CONFLICT. FILE может восстановить объект после
закрытия, но это не улучшает историческую статистику до deadline. RTT задаётся
отдельным измеренным known/value; модуль не отправляет probe. Неизвестный RTT
запрещает увеличение в политике адаптации. Missing, late и damaged учитываются
ею как непригодные до срока части.

| API | Владение, ошибки и выполнение |
| --- | --- |
| [`fvm_smeasure_init`](../include/flashvm/smeasure.h#L30) | Проверяет идентификаторы, число частей и deadline; атомарно заменяет caller state. INVALID сохраняет прежнее состояние. Заимствований нет; повторная инициализация явно отменяет старое окно. |
| [`fvm_smeasure_record`](../include/flashvm/smeasure.h#L38) | Принимает доверенный индекс, valid 0/1 и монотонное относительное время. INVALID при неверных аргументах, CONFLICT при закрытии/регрессии времени; ошибка сохраняет состояние. Последовательный владелец, ограниченное постоянное время. |
| [`fvm_smeasure_finish`](../include/flashvm/smeasure.h#L44) | Замораживает собственную копию отчёта. INVALID при неверном RTT/аргументах, CONFLICT при раннем/повторном вызове или регрессии времени. Ошибка сохраняет state/output. Копию можно передать [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L117) или [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L106); framing от этого сам не меняется. |

State, входные данные и выходы не перекрываются; вызовы одного окна сериализуются.
Это состояние свободно копируется для снимков: самоссылок и заимствованных VM
объектов нет. Не путать с политикой адаптации, которой нужен неизменный адрес.
Большие передачи делятся на окна с явным агрегированием, без усечения счётчиков.

## Сборка и проверка

```sh
cmake -S flashVM -B build-receive -DFVM_BUILD_TESTS=ON
cmake --build build-receive --target flashvm_srecord_measure_contract flashvm_srecord_oracle
ctest --test-dir build-receive -R flashvm_srecord_measure --output-on-failure
```

Native-контракт проверяет точный deadline, повторы/исправления, регрессию времени,
атомарность закрытия/ошибок, последний индекс bitmap и максимальное Uint32 время.
Независимая Python модель категорий проверяет 48000 поступлений в 80 окнах и
канонические байты отчётов через большие целые/zlib. Это ограниченный контракт
наблюдений, не production listener, контроль перегрузки, аутентифицированный
транспорт, кодек или полная приёмка платформ/SWF.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sadapt_feedback_write` | function | [include/flashvm/sadapt.h:117](../include/flashvm/sadapt.h#L117) |
| `fvm_sadapt_observe` | function | [include/flashvm/sadapt.h:106](../include/flashvm/sadapt.h#L106) |
| `FVM_SAdaptFeedback` | struct | [include/flashvm/sadapt.h:39](../include/flashvm/sadapt.h#L39) |
| `FVM_SMeasure` | struct | [include/flashvm/smeasure.h:20](../include/flashvm/smeasure.h#L20) |
| `fvm_smeasure_finish` | function | [include/flashvm/smeasure.h:44](../include/flashvm/smeasure.h#L44) |
| `fvm_smeasure_init` | function | [include/flashvm/smeasure.h:30](../include/flashvm/smeasure.h#L30) |
| `fvm_smeasure_record` | function | [include/flashvm/smeasure.h:38](../include/flashvm/smeasure.h#L38) |
<!-- source-links:end -->
