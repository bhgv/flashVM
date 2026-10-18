# FlashVM v0.7 — отчёт / report

В отдельном коммите v0.6 из текущего дерева удалены вспомогательные результаты
проверок: логи, JSON-сводки, копии настроек и списки хешей. Настоящие исходники
воспроизведения ошибок перенесены в `scripts/probes/`. Правила `.gitignore` и
`AGENTS.md` запрещают коммитить такие результаты снова. История Git сохранена.

В v0.7 добавлены адресные дескрипторы живых мутексов и каналов, сериализация этих
дескрипторов в файлы, TCP-сервис владельца и удалённые обёртки. Мутекс сохраняет
общую блокировку: `set` ждёт захвата на стороне владельца, `touch` опрашивает
состояние, `reset` проверяет владельца. Каналы передают значения и ресурсы;
ограниченные очереди сохраняют обратное давление. Ошибки закрытых ресурсов
записываются в очередь отладчика с местом вызова. Добавлены отмена сетевых
ожиданий рабочего потока, освобождение удалённых захватов при уничтожении
контекста, наследование сервиса скриптовыми потоками, тесты и C89-пример.

Проверки на Linux с C89/C++98:

| Профиль | Результат CTest |
| --- | --- |
| AGG + шрифты + кодеки изображений | 143/143 |
| Cairo + шрифты + кодеки изображений | 143/143 |
| Без графики/кодеков, со шрифтами | 137/137 |
| Без графики/шрифтов/кодеков и OS_PORTS | 130/130 |

Отдельные процессы проверяют восстановление каналов/мутекса из файла, удалённый
захват после освобождения, неправильного владельца и закрытие ресурса. Три
рабочих потока проверяют 300 захватов общей удалённой блокировки. Проверены
отмена ожидания мутекса, пустого канала и заполненной очереди, каноничность
обёрток, пересылка ресурсов обратно владельцу, ошибки устаревшего дескриптора
и запись источника/строки в отладчике. Проверка манифеста внешних исходников:
2094 файла, без изменений сторонних библиотек.

ASan/UBSan: 143 проверки и примера, включая два независимых процесса.
Сборщик проверок компилирует C89/C++98 с `-pedantic-errors`. Неизменённые
санитизированные объекты внешних библиотек использованы повторно. Ограничения
среды сохранены: LeakSanitizer отключён; исключения контейнеров проверяются
только UBSan; для `serialization_c.cpp` отключена стековая инструментация ASan,
при этом проверки кучи/глобальных объектов ASan и UBSan остаются включены.
Полные логи находятся вне репозитория и в архив исходного дерева не включаются.

Это проверенный этап, а не завершение всей FlashVM или распределённого отладчика.
TLS, аренды/восстановление после аварии клиента, согласование потерянного ответа,
распределённый поиск дедлоков и удалённое замораживание ещё не реализованы.
Windows/macOS/Android не проверены запуском. Подробности и API:
[RESOURCE_TRANSPORT.md](RESOURCE_TRANSPORT.md).

Зеркало ранее обновлено до v0.5 (`46bb188`) по прямой команде и проверено через
fetch/проверку вхождения коммита. v0.6/v0.7 автоматически туда не отправлялись.
Основной репозиторий — внутренний Git и версионный ZIP с полной историей.

## English

The separate v0.6 commit removes generated validation logs/summaries/configuration
copies/hash lists from the current source tree. Actual exception probes moved to
`scripts/probes/`. Ignore/development rules prevent committing generated artifacts
again; historical Git commits remain intact.

v0.7 adds live resource descriptors, file serialization, a TCP resource authority
and remote mutex/channel wrappers. Remote mutexes preserve acquisition/ownership;
channels forward graph values and resource capabilities with queue backpressure.
Closed/stale resource failures enter a debugger error queue with copied locations.
Worker cancellation, context-teardown release, script-worker broker inheritance,
a C89 example and bilingual API/limitations documentation are included.

Linux CTest: AGG 143/143, Cairo 143/143, headless/fonts 137/137, no graphics/fonts/
image codecs/OS_PORTS 130/130. Separate processes validate file restoration and
network ownership. Three workers complete 300 remote acquisitions; cancellation
covers mutex, empty receive and full send. Canonical imports, forwarding back to
the authority, closure, stale descriptors and debugger locations are checked.
The unchanged vendor manifest validates 2094 files.

ASan/UBSan: 143 checks/examples, including independent TCP processes. Strict
C89/C++98 compiler flags are retained. Unchanged vendor sanitizer objects are
reused. LeakSanitizer is disabled by the environment; container exception tests
use UBSan only; serialization exception frames omit stack ASan instrumentation
while retaining heap/global ASan and UBSan. Generated logs remain outside Git.

This is an incremental validated checkpoint. TLS, leases/crash recovery,
uncertain-response reconciliation, distributed deadlock analysis and remote freeze
remain unfinished; Windows/macOS/Android execution is unverified. See the API
and limitations document. The mirror remains at the explicitly pushed and verified
v0.5 commit; v0.6/v0.7 have not been pushed automatically.
