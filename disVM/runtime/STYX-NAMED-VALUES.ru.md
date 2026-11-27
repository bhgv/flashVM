# Типизированные значения Limbo в именованном канале StyxRPC

[English](STYX-NAMED-VALUES.md) · [Объявления транспорта](../module/styxnamed.m) · [Объявления значений](../module/styxvalue.m) · [Протокол](../../lib/styxrpc/reference/named_channel.ru.md)

Один канал в парном пространстве имён принимает сообщения разных типов.
В приложении подключите `sys.m` и `styxnamed.m`. Вторая декларация включает
типы значений. Загрузите `styxnamed.dis`, вызовите `init()` и откройте имя
через `open()`, например `/jobs`. Типизированные методы используют
`styxvalue.dis` и модуль Math DisVM. Для прямой работы с кодеком загрузите
`Styxvalue` и вызовите его `init()`.

`Message.Data` передаёт `Value.Numeric` (binary64), `Textual` (байты
UTF-8/WTF-8 с длиной), `Logical`, `Nullval`, `Undefinedval` и `Graph`.
`Styxvalue.text(s)` оборачивает строку Limbo. `Textual` сохраняет нулевые
байты. `Message.Shared` несёт аренду живой ячейки. Типизированный приём
открывает аренду перед возвратом. `Message.Intrefdata` содержит прежний
дескриптор. Если владелец смонтирован, передайте его точку монтирования
и байты в [Styxintref.accept](../module/styxintref.m).

Для публикации ячейки используйте `sharevalue`, для её пересылки
вызовите `sendvalue(ref Message.Shared(cell))`. `cell.getvalue()` и
`cell.setvalue(value)` читают и меняют текущее значение любого
поддерживаемого типа. Закрывайте свою аренду через `cell.close()`.
Вызовы на одном файловом дескрипторе последовательны. Обмен блокируется
до встречи с другой стороной. Доступны и кадровые `send`, `recv`, `get`, `set`.

[Styxmodref](STYX-MODULE-REFS.ru.md) захватывает и пересылает точные
экспорты Int32 подключённой FlashVM по тому же именованному каналу.

## Содержимое графа

`Value.Graph` содержит проверенную оболочку FVMG. Перешлите её целиком,
чтобы сохранить исходные метаданные FlashVM. `inspect` читает переносимые
собственные данные FVMG версий 8 и 9. Поддержаны нативные таблицы, стандартные
Object и разреженные Array с целой логической длиной. `Value.Link(index)`
указывает на узел. Несколько связей могут вести к одному узлу, в том числе
к самому себе. Числовые и строковые ключи нативной таблицы независимы.
Для другого профиля графа `inspect` возвращает nil.

`build` записывает достижимые данные в FVMG v8. Передайте поля и слоты с неповторяющимися ключами. Array сохраняет длину, пропуски
и индексированные значения. При сборке переносятся собственные данные.
Прототипы, методы, наблюдатели, пользовательские аксессоры, атрибуты
и нативные ресурсы выходят за пределы этой структуры данных.
В собранный Array включается штатный аксессор длины.

## Исполняемый сценарий

[Приложение Limbo](../../lib/styxrpc/tests/flashvm/typed_pair.b) и
[фильм AS](../../lib/styxrpc/tests/flashvm/typed_pair.as) запускаются
в обычной паре. Для сборки и запуска используйте
[команду парной проверки](STYX-CHANNEL.ru.md) с `--typed`.
Ожидаемый вывод — `PASS native typed RPC paired VMs`.
`--tcp` включает локальный TCP, а `--jit` включает JIT.

## Справочник

- Транспорт и аренды — [sendvalue](styx-named-api/sendvalue.ru.md), [recvvalue](styx-named-api/recvvalue.ru.md), [sharevalue](styx-named-api/sharevalue.ru.md), [getvalue](styx-named-api/getvalue.ru.md), [setvalue](styx-named-api/setvalue.ru.md).
- Кодек — [init](styx-named-api/value_init.ru.md), [text](styx-named-api/text.ru.md), [encode](styx-named-api/encode.ru.md), [decode](styx-named-api/decode.ru.md), [inspect](styx-named-api/inspect.ru.md), [build](styx-named-api/build.ru.md).
