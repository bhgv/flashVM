# Guest owner API: first typed path

Copyright (c) 2026 bhgv

`proxyowner.dis` и builtin `$Proxyhost` связывают explicit guest proxies с
настоящими managed roots и исполнением на постоянном Prog владельца. Это
**process-local typed channel adapter**, а не готовое соединение между двумя VM.

## Поддержанный путь

Подключать declarations в таком порядке: `proxyhost.m`, `proxyhandler.m`,
`proxyowner.m`. Вызвать `Proxyowner.init()` до spawn клиентов/владельцев.
`Proxyowner.serve(handler, cell, capture, ready)` запускается отдельным `spawn`.
Он передаёт в `ready` endpoint и два handles: variable и function/capture.
`Cell.value` сейчас имеет тип hosted NC64 `int`; это не общая dynamic Value схема.

| Вызов | Действие на исходном owner |
| --- | --- |
| Read | Прочитать исходную `Cell.value` |
| Write | Изменить ту же `Cell.value`, ответить после effect |
| Invoke | Выполнить `Proxyhandler.invoke` с исходным capture и 0..2 actual Cell refs |
| Drop | Отозвать экспорт, сохранив действующие in-flight pins и caller roots |
| Stop | Закрыть session, передать roots конкурентному Fast и завершить owner loop |

Handler module остаётся управляемым владельцем serving frame, а не C function
pointer. Его `start` и `invoke` исполняются на том же Prog. `begin` транзакционно
admit/resolve targets и refs, возвращая отдельный strong owner каждого result
occurrence, включая повторные aliases. Guest исполняет bytecode и вызывает finish
после callback, в том числе после обработанного exception. Result пока scalar int.

`Handle` содержит session identity, slot и generation. Проверяются target, каждый
ref и Drop. Совпадение slot/generation у другого endpoint либо после reopen не
разрешает доступ к локальной replacement cell. Identity здесь process-local,
не network authority и не restart epoch для разных VM.

## Deadline и завершение

`call` использует общий monotonic deadline для offer и reply. `NotSent` означает,
что channel offer не передан; `Unknown` после delivery не доказывает отмену или
admission и запрещает blind replay. `OwnerError` после callback exception может
включать частичный effect. Возвращённый успешный ответ означает completed effect.

У каждого active caller один child alarm; после результата/timeout caller делает
killwait, поэтому быстрые calls не накапливают спящие timers. Брошенный caller
оставляет finite alarm до своего deadline; signal и reply channels односекционные,
late send не требует оставшегося получателя. Cleanup/join зависит от планирования
OS; deadline не является hard real-time гарантией возврата из cleanup.

Начальная cycle policy консервативна: active owner callback не может ждать другой
`Proxyowner.call`. Возвращается OwnerError до timer/spawn/send. Это явный отказ,
не global lock; continuations для вложенных удалённых calls ещё не реализованы.
Другой независимый serving Prog продолжает работу, пока первый ждёт native I/O.

Retirement hook работает до снятия Prog root/stack, когда его REG snapshot уже
опубликован. Незавершённый callback получает finish; session закрывается, roots
передаются Fast. При pressure только этот owner временно выходит из ACTIVE,
остальные гости и collectors продолжают исполняться. Старый exclusive-handoff
и перенос Prog/живых REG на другой поток не используются.

## Проверки

`test_guest_proxy.py` компилирует настоящий library/handler/main и проверяет
C/JIT, оба poll paths, восемь клиентов, live variable mutations и repeated ref
aliases, callback owner PID, late effect без replay, handled error, NotSent,
Unknown, nested-wait refusal, независимый owner и kill активного callback.
Отдельно проверены Drop, foreign refs/Drop и reopen с теми же slot/generation.
Private omission session fence должен завершиться семантическим отказом до
использования replacement root. Компиляционная ошибка/crash не считается proof.

`PROXYHOST opened=closed exports=0 active=0`, matched native resident joins и
полностью опустошённые native RC очереди проверяются после completion. Generated
module declarations/table воспроизводятся matching NC64 producer и входят в
generation freshness test. Wide native manifest теперь включает 48 workloads.

## Следующая незакрытая граница

Handle copies не создают автоматические remote Retain: Drop отзывает общий export,
а не реализует GC каждой proxy-копии. Нет distributed GC, arbitrary closure/value
types, proxy/function results, String snapshots, cross-owner ref routing, response
cache/ack или wire authentication. Local request IDs считаются serving loop и
не являются network replay protocol. Native `stream.c` ещё не соединён с этим
endpoint: network framing, authenticated sessions и два реальных DisVM peers
остаются отдельной работой. Новые shared DIS64 IDs здесь не назначаются.
