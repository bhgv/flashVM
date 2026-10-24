# Модульное взаимодействие AS1/AS2 и Dis

Статус: проект обязательного расширения; runtime-возможность ещё не реализована. [English](MODULE_INTEROP.md).

## Общая модель приложения

Limbo остаётся основным языком первоначальной IDE. Части можно писать на AS1/AS2 и связывать с Limbo-модулями. Позднейшая AS-реализация/порт IDE допускается; UI, resource и module contracts не зависят от языка реализации. Это часть надмножества SWF <= 8, а не утверждение, что прежний AVM1 сам поддерживает модули Dis.

Обязательны оба направления:

- AS1/AS2-компонент загружает/разрешает интерфейс Dis-модуля и вызывает его exports.
- AS1/AS2-компонент объявляет/реализует интерфейс, регистрируется как модуль и вызывается из Limbo/Dis.
- AS и Limbo обмениваются явно типизированными каналами и данными по общему контракту lifetime/serialization.

Интерфейс модуля имеет identity, version и проверяемые signatures. Import проверяет аргументы/результаты и имена exports до исполнения. AS-реализация предоставляется через native Dis-module adapter/proxy; она не трактуется как Dis machine state или байткод. Кучи, стеки, instruction sets и JIT pipelines AVM1/Dis остаются отдельными.

## Вызовы и планирование

Первый вариант — коррелированные асинхронные request/reply calls. Сам факт загрузки модуля не добавляет AS1/AS2 синхронный блокирующий/yielding native call convention. Синхронно выглядящий синтаксис требует отдельно реализованного continuation/scheduling contract; он не блокирует владельца FlashVM в ожидании Dis или наоборот.

Вызов выполняется на владельце runtime получателя. Dis-to-AS adapter ставит AVM1 invocation в очередь и возобновляет ожидающую Dis activity после доставки ответа. AS-to-Dis соблюдает обратное правило владения. Определяются callback/reentrant ordering и отсутствие цикла взаимного ожидания owners. Запрос содержит module identity/generation и call identity; ошибки load/signature, exception, cancellation, unload и stale result получают различные terminal outcomes.

## Типизированная граница

Это обязательные правила представления; первый исполняемый bridge объявляет поддержанный subset и явно отвергает остальное.

| Значение Limbo/Dis | Требование границы AS |
| --- | --- |
| byte / int | Проверка диапазона/целочисленности без молчаливого wrap и усечения дробей |
| big | Точное 64-bit представление/обёртка; без потери через binary64 Number |
| real | Явная политика IEEE binary64, включая nonfinite и signed zero |
| string | Счётная кодировка/конверсия с правилами NUL и версии SWF; без неявного обрезания по NUL |
| array / list / ADT | Поддержанная схема копируемого графа либо opaque typed handle; сохранение обещанных identity/cycles |
| channel | Типизированный endpoint handle с lifetime, flow control и cancel/close semantics |
| module / callable | Проверенный интерфейс и экспортированная операция, а не произвольное имя/адрес C-функции |

Native structs, raw pointers и GC heap addresses не передаются как payload приложения. Opaque handle не превращает чужой объект в локальный AVM1/Dis object. Pending calls удерживают roots в каждой собственной runtime до успеха, ошибки или cancellation; unload/disconnect явно освобождают или retired такие связи. Циклы между collectors требуют разработанной handle/lease policy, а не предположения, что один GC видит другую кучу.

## UI по-прежнему только через каналы

Модульный вызов не обходит UI protocol. Контролы и widgets изменяются только через канальный сервис независимо от языка вызывающей логики — Limbo или AS. Сложный AS-компонент может реализовать модуль, но получает endpoint capabilities/messages вместо неограниченных чужих MovieClip pointers. Native owner-side handlers могут обращаться к соответствующим API реализации FlashVM.

## JIT, отладка и приёмка

Модульная граница работает с interpreted/JIT callers/callees. Отладчик показывает boundary frames с языком/модулем, pending-call state, доступные source/Dis-PC/native mappings, exceptions и шаги через границу без потери asynchronous call identity. Reload модуля retired прежние signatures, breakpoints, references и pending completions по явной политике.

Приёмка требует настоящих AS-to-Dis и Dis-to-AS calls, отказов typed input/output, точных big/string случаев, каналов, ошибок/cancellation, GC при pending call, unload/reload, вложенных вызовов без deadlock и interpreter/JIT сочетаний. Составной Limbo/AS-контрол IDE всё равно обновляется через каналы и получает настоящие события ввода. Нынешние numeric FIR и host Python build tests этих требований не проверяют.

[Архитектура host](ARCHITECTURE.ru.md) · [Порядок интеграции](DIS_ACCEPTANCE.ru.md) · [Правила читаемого кода](AGENTS.md).
