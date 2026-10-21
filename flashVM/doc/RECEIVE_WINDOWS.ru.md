# Окна приёма уникальных частей

[English](RECEIVE_WINDOWS.md), [адаптация](CHANNEL_ADAPTATION.ru.md),
[части данных](MEDIA_SLOTS.ru.md). `smeasure.h` превращает явно проверенные
хостом поступления в существующий компактный отчёт tag 0. Это отдельный C89
модуль без сокетов, выделения памяти, часов, VM roots и глобального состояния.

## Время, идентичность и категории

Отдельный [FVM_SMeasure](../include/flashvm/smeasure.h#L22) хранит одно доверенное окно stream/epoch/window:
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
| [`fvm_smeasure_init`](../include/flashvm/smeasure.h#L36) | Проверяет идентификаторы, число частей и deadline; атомарно заменяет caller state. INVALID сохраняет прежнее состояние. Заимствований нет; повторная инициализация явно отменяет старое окно. |
| [`fvm_smeasure_record`](../include/flashvm/smeasure.h#L45) | Принимает доверенный индекс, valid 0/1 и монотонное относительное время. INVALID при неверных аргументах, CONFLICT при закрытии/регрессии времени; ошибка сохраняет состояние. Последовательный владелец, ограниченное постоянное время. |
| [`fvm_smeasure_finish`](../include/flashvm/smeasure.h#L51) | Замораживает собственную копию отчёта. INVALID при неверном RTT/аргументах, CONFLICT при раннем/повторном вызове или регрессии времени. Ошибка сохраняет state/output. Копию можно передать [`fvm_sadapt_feedback_write`](../include/flashvm/sadapt.h#L117) или [`fvm_sadapt_observe`](../include/flashvm/sadapt.h#L106); framing от этого сам не меняется. |

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

## Адаптер проверенного медиаприёмника (v0.748)

[FVM_SMeasureMedia](../include/flashvm/smeasure.h#L57) привязывает один инициализированный [FVM_SMediaReceiver](../include/flashvm/smedia.h#L47),
ещё не принявший ни одной части. Копируются согласованные mode, stream, epoch,
object, presentation time, total, profile и tag; сам native receiver и его
готовое хранилище заимствуются до finish. Приём после смены объекта/профиля даёт
CONFLICT. Не переинициализируйте и не меняйте receiver отдельно до закрытия;
state, буферы и выходы не перекрываются, все вызовы сериализует один владелец.

[`fvm_smeasure_media_accept`](../include/flashvm/smeasure.h#L78) проверяет время хоста, затем slot и настоящие данные
через [`fvm_smedia_accept`](../include/flashvm/smedia.h#L82). Лишь успешное принятие засчитывает проверенный индекс.
Точный повтор остаётся одной частью, другой валидный payload того же индекса
даёт CONFLICT. Повреждённый или чужой slot сохраняет adapter, receiver, image
и complete. Он остаётся отсутствующим: неверный заголовок не доказывает индекс.
Хост может вызвать [`fvm_smeasure_record`](../include/flashvm/smeasure.h#L45) для вложенного measurement, лишь когда
отдельный доверенный конверт идентифицирует повреждение; позиция в batch сама
по себе не является индексом медиачасти.

Закрывайте окно по настоящему deadline хоста. Потом обычный receiver может
восстанавливать FILE, сохраняя исторический отчёт. Закрытие касается только
копии окна, без I/O. Эти функции не устанавливают аутентификацию, pacing,
контроль перегрузки, MTU discovery или сетевой listener.

| API | Владение и результат |
| --- | --- |
| [`fvm_smeasure_media_init`](../include/flashvm/smeasure.h#L71) | Привязывает пустой initialized receiver; выводит expected из согласованных total/profile. Без нового image allocation. INVALID при неверном descriptor, CONFLICT при уже принятых частях; ошибка сохраняет выход. |
| [`fvm_smeasure_media_accept`](../include/flashvm/smeasure.h#L78) | Заимствует slot лишь на вызов; проверяет identity/checksum/повтор до копирования и учёта. Закрытое окно, регрессия времени, другой объект/профиль дают CONFLICT; ошибки codec атомарны. complete меняется лишь после успешного принятия. |
| [`fvm_smeasure_media_finish`](../include/flashvm/smeasure.h#L82) | Замораживает собственный отчёт с elapsed и явным RTT хоста. Контракт ошибок/атомарности как у обычного finish; после успеха заимствование receiver заканчивается, память не освобождается. |

[C89 пример media_parts](../examples/embedding/media_parts/README.ru.md)
теперь формирует FILE отчёт до восстановления и отдельный LIVE отчёт с
неизвестным RTT. [Тест настоящих кадров](../frontend/tests/frame_parts_example.py)
принимает реальные экспортированные части через IPv4 UDP loopback, использует
монотонные часы хоста, измеряет RTT TCP request/echo и закрывает окно по
настоящему deadline. Первое окно теряет две части из шести: испорченный slot
без доверенного индекса и пропуск. Следующие получают все одиннадцать и шесть.
Байты native отчёта совпадают с независимым Python кодированием integer/zlib.
Измеренные отчёты подаются плееру и воспроизводят последовательность профилей
normal→small→normal→large в FILE и LIVE; сравниваются все RGBA байты.

Loopback подтверждает ограниченный транспортный сценарий, не Internet/device
приёмку. Deadline теста 100 мс и размеры датаграмм — параметры теста, а не
production рекомендации по времени или path MTU. RTT и планирование на других
хостах могут отличаться; production хост выбирает собственную политику.

Проверка v0.748: прошли 59 выбранных native проверок, 14 shared-host и отдельный набор C++98/measurement. Новые measurement контракты прошли ASan/UBSan; затронутые protocol/cache пути также проверены с настоящими кадрами interpreter/JIT. Проверка утечек отключена ограничениями хоста; вся VM не инструментирована. Strict C89 measurement контракты линкуются, C++98 consumer собирается для ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x64/MinGW. Чужие бинарники не запускались. Все девять стадий открыты.

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
| `FVM_SMeasure` | struct | [include/flashvm/smeasure.h:22](../include/flashvm/smeasure.h#L22) |
| `fvm_smeasure_finish` | function | [include/flashvm/smeasure.h:51](../include/flashvm/smeasure.h#L51) |
| `fvm_smeasure_init` | function | [include/flashvm/smeasure.h:36](../include/flashvm/smeasure.h#L36) |
| `fvm_smeasure_media_accept` | function | [include/flashvm/smeasure.h:78](../include/flashvm/smeasure.h#L78) |
| `fvm_smeasure_media_finish` | function | [include/flashvm/smeasure.h:82](../include/flashvm/smeasure.h#L82) |
| `fvm_smeasure_media_init` | function | [include/flashvm/smeasure.h:71](../include/flashvm/smeasure.h#L71) |
| `fvm_smeasure_record` | function | [include/flashvm/smeasure.h:45](../include/flashvm/smeasure.h#L45) |
| `FVM_SMeasureMedia` | struct | [include/flashvm/smeasure.h:57](../include/flashvm/smeasure.h#L57) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:82](../include/flashvm/smedia.h#L82) |
| `FVM_SMediaReceiver` | struct | [include/flashvm/smedia.h:47](../include/flashvm/smedia.h#L47) |
<!-- source-links:end -->
