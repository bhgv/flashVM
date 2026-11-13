# Удалённые переменные и функции: рабочий контракт

Copyright (c) 2026 bhgv

Здесь развивается proxy-блок в `master`. Native session/transport foundation уже
реализован; подключение живых managed variables/functions DisVM ещё впереди.
Основание — ранее выбранная архитектура
[канальных proxy к владельцу](../../fir/v1/doc/DYNAMIC_VALUES.md),
[проекции](../../ide/dis/LIVE-PROJECTIONS.md) и
[граница долгих операций](../OWNED_OPERATIONS.md).
[Обмен с Inferno](https://github.com/bhgv/Inferno-OS-bhgv/commit/7f2241455734e1142a3254fad9d1ec4f94032ac7)
передаёт этот контекст другой стороне. Новых DIS64 kind/opcode/ABI IDs здесь нет.

## Уже выбранная семантика

- Значение, замыкание, global/capture и правила типа остаются у владельца.
  Другой процесс держит endpoint и проверяемую identity/generation, не heap pointer.
  Один router может обслуживать много объектов; процесс на каждый объект не нужен.
- Snapshot и live proxy различаются. Скалярная копия не становится live-переменной.
  `ref`-аргумент передаёт proxy к оригиналу, включая identity/aliases, а не незаметную
  копию графа. Результат может быть snapshot, variable proxy или function proxy.
- Describe/ReadAs/Get/Set/Invoke/Apply/Retain/Release исполняются в VM-владельце.
  ReadAs одним запросом проверяет тип/политику/ревизию и читает значение; два
  раздельных запроса Describe + read не образуют атомарной операции.
- Caller/owner проверяют authority, session/generation и correlation ID. Remote ID
  не является доверенным operation token для локального payload guard.
- Target и все ref-аргументы удерживаются до завершения принятого запроса.
  Новый proxy-result получает owner до публикации; провал публикации откатывает
  это владение. Release/close/GC не освобождают объект под выполняющимся запросом.
- GC клиента не заменяет remote lifetime protocol. Нужны явные владельцы/leases,
  release/revoke, cleanup при закрытии и определённое поведение потери клиента.
  Нельзя объявить distributed GC готовым на основании одного локального finalizer.
- Transport receipt не равен completion. Успех Set/Invoke означает завершённый
  эффект. После timeout/разрыва принятого запроса исход может быть неизвестен;
  side effects не повторяются автоматически.
- UI/owner pump не ждёт синхронно запрос, обслуживаемый им самим. Для reentrancy
  нужны continuation/маршрутизация либо явный cycle refusal, не global exclusive.
- Все Prog сохраняют native OS thread и живые TLS REG. Запрещены мировой stop,
  reference executor и старый FlashVM function_serve exclusive-handoff как адаптер.

## Последовательность реализации и проверяемые границы

1. Explicit proxy library/owner adapters и bounded request/lease state machine.
   Типы snapshot/proxy/error различимы; ticket/session IDs не переиспользуются.
2. Версионированный bounded transport: частичные чтения/записи, общий deadline,
   quotas, остановка и повторное подключение. Никаких C padding/pointers в wire.
3. Native DisVM adapter: реальные variable/function owners, вызовы и ref-аргументы,
   результаты, удержание GC-корней и cleanup на permanent owner threads.
4. Fault/GC/concurrency acceptance на двух реальных процессах и независимых
   native владельцах. Лишь затем compiler-transparent lowering.

Private request/lease tables и bounded admission replay-window реализованы ниже.
Result cache/ack policy, transport authentication, typed wire records и связь с
managed owner ещё требуют реализации/проверки.
Ни общий документ, ни trial tags Inferno не назначают публичные IDs за другую сторону.

## Реализованная native-основа

`session.h` / `session.c` / `admission.c` содержат ограниченные таблицы owners и
запросов. У каждой сессии свой mutex; внутри нет GC callbacks, исполнения VM,
сетевого ожидания или world lock. Успешный publish принимает уже зарегистрированный
owner root token. Collect возвращает его owner-потоку после последнего peer ref и
admitted pin. Сам этот код не регистрирует managed roots и не выполняет Dis calls.

Admission одной транзакцией проверяет target, все variable refs и overflow,
копирует request и удерживает каждое вхождение ref, включая aliases. Отказ не меняет
pins/ID window/output. Только `OK` разрешает исполнение; точный повтор сообщает
active/completed, другой payload/operation/порядок refs даёт conflict. Window имеет
заданное при создании число slots; ID вне окна получает stale, активная запись
никогда не вытесняется. Это окно admission, а не result cache: транспортный adapter
должен сохранять результат/подтверждения отдельно и не вызывать effect при duplicate.
Закрытие снимает peer refs, но принятые calls сохраняют pins до finish.

Идентификаторы и layout здесь приватные, process-local. Они не являются wire ABI,
сетевой authentication или DIS64 IDs. Session tick требует monotonic driver;
истёкшая сессия не возобновляется. Reconnect создаёт новую identity. Межсессионные
и межузловые refs должны маршрутизироваться adapter-ом; текущая pin transaction
проверяет только roots этой сессии. Result proxy сначала должен получить отдельный
root/peer owner; failure publish сохраняет token у caller для rollback.

`stream.h` / `stream.c` передают bounded native bytes через уже connected socket:
partial read/write, EOF, errno и один absolute monotonic deadline. Нет hidden
allocation, изменения fd flags, SIGPIPE или replay. FD остаётся у caller до конца
операции; для cancellation допустим shutdown, close/reuse ждёт завершения I/O.
Один read и write могут идти одновременно; same-direction frames сериализует
будущий connection adapter. Даже успешная отправка всех bytes не означает RPC
completion. Framing, authentication, response cache и network FD/GC adapter пока
не реализованы.

Четыре native host-unit проверки собираются также из runtime CMake и запускаются
CI на AMD64/ARM64, включая AMD64 TSan. Есть 8000 эффектов из восьми pthreads,
disconnect при восьми удерживаемых calls с прогрессом независимой сессии, replay
после потерянного ответа, stale generations, counter/generation overflow, rollback,
lease expiry, два OS-процесса, fragmentation, truncation, trickle deadline и partial
blocked write. Это native foundation tests, не доказательство managed GC или двух
реальных DisVM peers. Локальные ASan/UBSan и TSan прошли; LeakSanitizer отключён
из-за ограничения ptrace среды, поэтому leak-sanitizer coverage не заявляется.

Повторение без VM:

```sh
cmake -S disVM/remote -B build/remote
cmake --build build/remote --parallel 4
ctest --test-dir build/remote --output-on-failure
```

## Обязательные регрессии

| Группа | Проверяемый результат |
| --- | --- |
| Создание и работа | Оригинал живёт у владельца; read/write и функция видят актуальное состояние; копия остаётся копией |
| Несколько потоков | Параллельные клиенты, общий proxy и повторные ref aliases не теряют обновления; независимые владельцы продолжают работу |
| Аргументы/результаты | `ref` остаётся live proxy; scalar/String snapshots независимы; возвращённая функция вызывает оригинал |
| GC | Последний proxy, активный call, ref-аргумент, capture и новый результат имеют отдельные сроки жизни; нет use-after-free/double release/root leak |
| До admission | Разрыв/отмена не исполняют непринятый запрос; очищаются staging/roots/queue reservations |
| После admission | Потеря связи не выдаётся за доказанную отмену; owner завершает/подтверждает cleanup; результат может быть unknown |
| После эффекта | Потеря ответа не вызывает слепой replay; одинаковый ID с иным payload отвергается |
| Reconnect | Старые capabilities/ответы не попадают в новую session/generation |
| Плохая связь | Delays, truncation, fragmented header/payload/reply, общий deadline и bounded memory; работа прочих owner не останавливается |
| Отказы | OOM, capacity, malformed/wrong-kind/wrong-authority, ID exhaustion и провал публикации результата оставляют проверяемое владение |

## Что дала синхронизация Inferno

Свежие fixed Sys guards проверяют managed FD, byte arrays, read/write, slices,
return-slot ownership и rollback. Это полезные части будущего transport adapter,
но 14 проверенных exports не включают dial/announce/listen. Их нельзя считать
уже готовым network layer. См. [отчёт синхронизации](../UPSTREAM-SYNC.md).
Наш NC64 и новый DIS64 проверяются раздельно; успешные native workloads не
выдаются за выполнение новой wire ABI или за готовность этого proxy-блока.
