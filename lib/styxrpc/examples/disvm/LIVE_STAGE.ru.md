# Живые поля, коллекции по значению и контекстные вызовы в одном мосте

[English](LIVE_STAGE.md) · [Примеры SDK](../README.ru.md) · [Живой API для C](../../LIVE_VALUES.ru.md) · [Типы FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.ru.md)

Пример соединяет нативного владельца на Limbo с AS2-приложением в формате SWF. Одна библиотека передаёт списки и разреженные массивы по значению, а затем в одном вызове меняет ref или гибридную таблицу и возвращает канал. Отдельная запись в смонтированной VFS даёт AS2-клиенту живой доступ к переменным владельца. Самостоятельный клиент C вызывает ту же библиотеку и обращается к тем же проекциям через свою хостовую сессию.

## Сборка и запуск

Из корня репозитория соберите нативную DisVM, компилятор Limbo, хостовый Styx-плеер FlashVM и средства AS2. Используйте указанные пути или подставьте пути к собранным программам. Для примера C дополнительно нужен хостовый libstyxrpc, настроенный с `STYXRPC_EXAMPLES=ON` и установленными пакетами `Styx` и `Unibus`. Соберите его цель `styxrpc-live-stage-client`. [Сборка FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC.ru.md#сборка) · [Сборка SDK](../../README.ru.md#сборка).

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl --live-stage
```

Команда компилирует библиотеку Limbo и SWF, открывает парную сессию с живой загрузкой и запускает владельцев. По умолчанию используются Unix-сокет и интерпретатор. Флаг `--jit` включает JIT, `--tcp` выбирает локальный TCP. Их можно сочетать. При успехе выводятся `PASS native live-stage RPC paired VMs` и `PASS Dis application completed`. Неверное значение или пропущенный сигнал завершает запуск с ошибкой. Временные образы и подключения удаляются после выхода.

## Как работают публикации VFS

1. [Владелец Limbo](../../tests/flashvm/live-stage_pair.b) публикует запись по пути `/live/.var`. Поле `numbers` хранит массив Int32, в котором индексы 0 и 2 разделяют одну нативную ячейку. В `details` находятся строка, вектор и скаляр `big`. `scalar` также указывает на исходную общую ячейку Int32.
2. [Проверка из AS](../../tests/flashvm/live-stage_values.as) захватывает запись и её поля. Пишет `42` по индексу 0. Читает `42` по индексу 2 и из `scalar`. Записывает строку Unicode с длиной и проверяет обе границы знакового 64-битного диапазона. Владелец читает ту же Int32 через `Vfsexport.intget`. [Таблица проекций](../../LIVE_VALUES.ru.md#контракт-проекции-для-владельца)
3. [Задание](../../tests/flashvm/live-stage_stale.as) получает поле с правом записи до снятия записи владельцем. Владелец публикует новую запись по тому же пути и подаёт сигнал. Попытка записи через старое поле приводит к исключению. [Точка входа AS2](../../tests/flashvm/live-stage_pair.as) явно открывает новую публикацию и читает `99`. Владелец сверяет `42` в старой ячейке. Поля и корень в обычных вызовах закрываются явно. Оставшиеся привязки задания освобождаются при его завершении.

## Что вызывает ActionScript

В точке входа `file.load()` возвращает `proxy` для скомпилированного модуля `Library`. Ниже перечислены все его методы, которые вызывает этот пример. [AS2](../../tests/flashvm/live-stage_pair.as#L12)

| Вызов AS2 | Объявление Limbo | Реализация |
| --- | --- | --- |
| `proxy.words(list)` | [`Library.words`](../../tests/flashvm/live-stage_library.b#L11) | [`words`](../../tests/flashvm/live-stage_library.b#L21) |
| `proxy.sparse(packed)` | [`Library.sparse`](../../tests/flashvm/live-stage_library.b#L12) | [`sparse`](../../tests/flashvm/live-stage_library.b#L41) |
| `proxy.combine(refFields, input)` | [`Library.combine`](../../tests/flashvm/live-stage_library.b#L13) | [`combine`](../../tests/flashvm/live-stage_library.b#L59) |
| `proxy.newtable()` | [`Library.newtable`](../../tests/flashvm/live-stage_library.b#L14) | [`newtable`](../../tests/flashvm/live-stage_library.b#L74) |
| `proxy.update(table)` | [`Library.update`](../../tests/flashvm/live-stage_library.b#L15) | [`update`](../../tests/flashvm/live-stage_library.b#L83) |

Тип гибридной таблицы объявлен в модуле Bigtable. [`Bigtable->Handle`](../../../../disVM/module/bigtable.m#L10)

## Копия списка и разреженного массива

В [библиотеке Limbo](../../tests/flashvm/live-stage_library.b) метод `words` объявляет `list of (string, big)`. Для `l[(sb)]` AS2-клиент передаёт плотный Array из кортежей либо `null` для nil. Пустой входной Array также даёт nil. Limbo читает `hd` и `tl`, переставляет элементы и изменяет 64-битные поля. [Проверка вызовов в AS](../../tests/flashvm/live-stage_calls.as) получает результат как собственный плотный Array. Отдельно проверяется, что nil превращается в `null`.

Метод `sparse` использует явный кортеж Limbo `(int, array of (int, string))`. Логическая длина хранится отдельно от числа занятых индексов. Упакуйте массив, вызовите метод и восстановите результат.

```actionscript
var values = [];
values[5] = "end";
values[2] = "☀";
values.length = 12;                   // Сохраняем пропуски в конце.
var packed = vfs.packSparse(values);  // [12, [[2, "☀"], [5, "end"]]]
var edited = proxy.sparse(packed);    // Limbo переносит второй слот на индекс 9.
var restored = vfs.unpackSparse(edited);
var hasOldSlot = restored.hasOwnProperty("5"); // false. У исходного массива индекс 5 остался.
```

Длина результата равна 12, значения лежат на индексах 2 и 9. `hasOwnProperty` отличает пропущенный индекс от любого значения. Тип элементов в примере `string`. Значение должно соответствовать скомпилированной схеме. Обычный `array of string` в Limbo использует плотное представление. [Типизированные вызовы](../../reference/managed_invoke.ru.md) · [API разреженных массивов](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PACK_SPARSE.ru.md).

## Обратная запись и аренда канала из одного ответа

`combine` получает `ref Box` с одним полем и заимствованный `chan of int`. Он читает сообщение, меняет поле ref, затем возвращает кортеж с этой ссылкой и новым `chan of big`. Скомпилированные списки аргументов и результата — `(r[b]c[i])` и `((r[b]c[b]))`. [Проверка на AS2](../../tests/flashvm/live-stage_mixed.as) видит один Array у исходного аргумента и возвращённой ссылки. Проверка присваивает через возвращённую ссылку. Канал результата остаётся доступен после освобождения прокси модуля. Его аренда удерживает выдавшее поколение до закрытия.

Вот объявление типа и функции из Limbo. Поле `box.value` соответствует `refFields[0]` в AS2. [Исходник Limbo](../../tests/flashvm/live-stage_library.b#L8-L13)

```limbo
Box: adt {
    value: big;
};
combine: fn(box: ref Box, input: chan of int): (ref Box, chan of big);
```

```actionscript
var refFields = ["-9223372036854775808"];
var input = vfs.localChannel("i", 1);
var sent = input.trySend(7);                   // Заполняем буфер для нативного приёма.
var result = proxy.combine(refFields, input);  // Владелец возвращает ту же ссылку и новую очередь.
var copyback = refFields[0];                   // "-9223372036854775801"
var output = result[1];                        // Канал результата держит свою аренду.
var inputClosed = input.close();
var unloaded = os.unload(proxy);
var observed = output.recv();                  // Значение по аренде остаётся доступно.
var outputClosed = output.close();
```

AS2-клиент также передаёт гибридную таблицу в `update`. Колбэк добавляет ключ `"wide"` и возвращает свой канал. После проверки ответа применяется изменение ключа в прежнем объекте таблицы. Контекстный вызов использует общую таблицу идентичности для аргументов, результата и обновлений. Заимствованная входная очередь принадлежит вызывающему, её полномочие истекает с вызовом. Аренда выходной очереди действует до освобождения. [Вызов с арендой](../../reference/channel_invoke.ru.md) · [Обновления графа](../../reference/graph_update_index.ru.md).

## Клиент C с нативной DisVM

```sh
python3 lib/styxrpc/tests/flashvm/live_stage_sdk.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --driver build/rpc/styxrpc-live-stage-client
```

Флаги выбора режима и транспорта те же. У [драйвера C](live_stage_client.c) две роли.

| Роль | Шаги |
| --- | --- |
| Вызов типизированных методов | Публикует встречный PID, считывает скомпилированные предложения, отправляет список `l[(sb)]` и проверяет ответ и случай nil. Вызывает метод таблицы с каналом, подготавливает `result.updates` на основе исходной таблицы, проверяет независимый ключ и получает точный знаковый минимум из канала. Освобождает аренду перед закрытием клиента. |
| Работа с живыми полями | Открывает клиента, захватывает [запись гостя Limbo](../../tests/flashvm/live-stage_sdk.b) и четыре поля. Закрывает родителя. Отличает nil от пустой строки, затем пишет пятибайтную строку с нулём. Меняет скаляр `big` на знаковый максимум. Выбирает относительный индекс плотной части таблицы и обновляет разреженный числовой ключ. Записывает Int32 и считывает значение через другой алиас. Ставит метку завершения только после успешной сверки. Закрывает поля, затем сессию. |

При успехе выводятся `PASS independent C SDK and native DisVM`, `PASS DisVM observed C SDK writes` и `PASS Dis application completed`. SDK сверяет ответ с арендой и выдаёт ссылки на записи в буфере вызывающего. Приложение готовит и применяет свои подтверждённые обновления до повторного использования буфера. [Хостовый API](../../reference/live_open_until.ru.md) работает без VM, когда другая сторона публикует те же файлы.
