# Удалённые переменные и функции: рабочий контракт

Copyright (c) 2026 bhgv

Это следующий блок разработки в `master`, не заявление о готовом network RPC.
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

Формат request/lease table, точные dedup/replay-window и cancellation policies,
transport authentication и typed wire records ещё требуют реализации/проверки.
Ни общий документ, ни trial tags Inferno не назначают публичные IDs за другую сторону.

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
