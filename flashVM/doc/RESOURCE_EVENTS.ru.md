[Файловый адаптер](RESOURCE_FILES.ru.md): дополнительные проверки watch и позиции операции описаны отдельно.

# Независимые наблюдатели ресурсов

C89 API `revents.h` добавляет к диспетчеру согласованный снимок ресурсов,
раздельные очереди событий и очистку сессий. Сокеты, mount/9P и пересылка VM
указателей здесь не создаются. Транспорт аутентифицирует сессию, владелец выдаёт
права. [Диспетчер](RESOURCE_DISPATCH.ru.md), [план ФС](RESOURCE_NAMESPACE.ru.md),
[пример](../examples/embedding/resource_events/README.ru.md).

## API, владение и потоки

Диспетчер заимствуется на время вызова. Выходные значения изменяются только при
OK. Записи C нельзя передавать по сети как структуры: поля кодируются отдельно.
Очереди команд, права и события защищены одним предоставленным host mutex.
Без lock hooks все вызовы выполняются одним потоком либо сериализуются снаружи.
После создания диспетчера новый API не выделяет память, не вызывает VM и не
создаёт корни GC. Хуки блокировки должны жить до уничтожения диспетчера.

| Функция | Контракт |
| --- | --- |
| [`fvm_rdispatch_resources`](../include/flashvm/revents.h#L42) | Сессия, массив, ёмкость и обязательный указатель числа строк. NULL/0 запрашивает количество. Малый ненулевой массив: LIMIT без изменения массива/числа. Видны только выданные ресурсы; пути копируются, операции фильтруются по текущим правам. Наследования прав от каталогов нет. |
| [`fvm_rdispatch_subscribe`](../include/flashvm/revents.h#L53) | Сессия, поколение ресурса, выходной watch. Нужен SUBSCRIBE. Каждый вызов создаёт отдельную пустую очередь; UNKNOWN для отсутствующего ресурса, DENIED без прав, LIMIT при исчерпании слотов. Обработчик ресурса не вызывается. |
| [`fvm_rdispatch_publish`](../include/flashvm/revents.h#L58) | Только владелец: ресурс, ненулевой Uint32 kind, буфер/длина, выходной sequence. Копирует до [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) каждому подписчику. Большой буфер: LIMIT; NULL при ненулевой длине: INVALID. Sequence начинается с 1, не оборачивается; при исчерпании LIMIT без публикации. Без подписчиков номер также расходуется. |
| [`fvm_rdispatch_event_take`](../include/flashvm/revents.h#L65) | Аутентифицированная сессия, watch и выходное событие. Копирует и удаляет только её старейшее событие. Пусто: PENDING; старое поколение: UNKNOWN; чужая сессия: DENIED. `missed` сообщает потери этого читателя после прошлого take и сбрасывается при успешном чтении. |
| [`fvm_rdispatch_unsubscribe`](../include/flashvm/revents.h#L73) | Закрывает очередь указанной сессии/watch с потерей непрочитанного. Другие наблюдатели не затрагиваются. Старый watch: UNKNOWN; чужая сессия: DENIED. |
| [`fvm_rdispatch_disconnect`](../include/flashvm/revents.h#L79) | Только владелец после остановки транспорта сессии. Удаляет её права, подписки, ожидающие команды и сохранённые ответы. Её выполняющаяся команда: BUSY до любых изменений. Порядок остальных команд сохраняется. Уже выполненные эффекты не отменяются. |

[`FVM_RResource`](../include/flashvm/revents.h#L31), [`FVM_RWatch`](../include/flashvm/revents.h#L13), [`FVM_REvent`](../include/flashvm/revents.h#L19) содержат собственные копии. У watch
и ресурса разные пространства поколений. SUBSCRIBE независим от READ/CONTROL:
владелец отвечает за состав доступных наблюдателю данных.

## Потери, отзыв и переподключение

[FVM_R_WATCHES](../include/flashvm/revents.h#L5) независимых очередей хранят по [FVM_R_EVENT_QUEUE](../include/flashvm/revents.h#L6) событий. При
переполнении удаляется старейшее только у медленного читателя. Его счётчик
`missed` насыщается максимумом Uint32. Sequence общий для всех ресурсов:
пропуск номера сам по себе не доказывает потерю в одной подписке. Это не часы
и не глобальный порядок событий разных компьютеров.

Отзыв SUBSCRIBE сразу уничтожает соответствующие очереди. Повторная выдача
прав не возвращает старые данные. Unregister отзывает все watch ресурса до
повторного использования слота. Исчерпанные поколения watch не оборачиваются:
слоты выводятся из оборота. Destroy удаляет оставшиеся события после остановки
пользователей; команды должны быть подтверждены через take либо disconnect.

Адаптер должен отвергать запоздавшие пакеты: нужны новые ID аутентифицированных
сессий или проверка эпохи транспорта. Disconnect не умеет распознавать сетевые
эпохи. Удалённый ответ мог относиться к уже выполненной операции; гарантия
exactly-once после обрыва не заявляется.

Это прикладная рассылка, подходящая для GUI и событий отладчика. UDP broadcast/
multicast локальной сети либо аутентифицированный relay могут быть адаптерами,
но сетевой транспорт, шифрование, 9P и медиапотоки здесь не реализованы.

## Сборка и проверка

Команды приведены в [EN](RESOURCE_EVENTS.md#verification-and-build). Target
flashvm_revents_threads доступен только в нативной POSIX-сборке; на Windows и
при cross-build его нужно исключить. Сама C89-библиотека не зависит от pthreads.
Тесты проверяют фильтрацию, неизменность outputs при ошибках, потери отдельных
читателей, отзыв/повторную выдачу, исчерпание поколений/sequence, BUSY и очистку
FIFO. Три настоящих потока сверяют 501 публикацию каждый: получено + потеряно.
Это не проверка сетевой доставки, GUI и запуска на других ОС.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `FVM_R_EVENT_QUEUE` | macro / define | [include/flashvm/revents.h:6](../include/flashvm/revents.h#L6) |
| `FVM_R_WATCHES` | macro / define | [include/flashvm/revents.h:5](../include/flashvm/revents.h#L5) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:79](../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:65](../include/flashvm/revents.h#L65) |
| `fvm_rdispatch_publish` | function | [include/flashvm/revents.h:58](../include/flashvm/revents.h#L58) |
| `fvm_rdispatch_resources` | function | [include/flashvm/revents.h:42](../include/flashvm/revents.h#L42) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:53](../include/flashvm/revents.h#L53) |
| `fvm_rdispatch_unsubscribe` | function | [include/flashvm/revents.h:73](../include/flashvm/revents.h#L73) |
| `FVM_REvent` | struct | [include/flashvm/revents.h:19](../include/flashvm/revents.h#L19) |
| `FVM_RResource` | struct | [include/flashvm/revents.h:31](../include/flashvm/revents.h#L31) |
| `FVM_RWatch` | struct | [include/flashvm/revents.h:13](../include/flashvm/revents.h#L13) |
<!-- source-links:end -->
