# Строки, массивы и точные big между FlashVM и DisVM

[English](TYPED_VALUES.md) · [Примеры SDK](../README.ru.md) · [Справочник типов FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.ru.md)

Этот пример загружает библиотеку Limbo из живой VFS, вызывает три типизированных экспорта и проверяет результаты в фильме AS2. Пример показывает независимые копии, идентичность ссылки на время вызова и канал, доступный после возврата метода.

## Сборка и запуск

Из корня репозитория FlashVM используйте нативную DisVM, загрузочный
компилятор Limbo, плеер FlashVM с установленным libstyxrpc и инструменты AS2.
Пути ниже предполагают каталоги сборки из
[руководства FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC.ru.md#сборка).
Подставьте пути к своим исполняемым файлам.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl --live-values
```

Сценарий компилирует библиотеку и фильм, открывает парную сессию и передаёт `--live-dis-load` при монтировании со стороны DisVM. По умолчанию запускается интерпретатор и используется Unix-сокет. Флаг `--jit` включает JIT, а `--tcp` выбирает локальное TCP-соединение. Флаги можно совмещать.

```text
PASS native live-values RPC paired VMs
PASS Dis application completed
```

Первую строку печатает приложение Limbo, когда видит `/wide-done`. При отказе метода, неверном значении или истечении срока ожидания метка не появляется и сценарий завершается ошибкой. После выхода обеих VM сценарий удаляет временную рабочую область.

## Как проходит обмен

1. [Библиотека](../../tests/flashvm/live-values_library.b) объявляет `wide`, `patch` и `widepipe`. DisVM извлекает сигнатуры и схемы из скомпилированных экспортов. [Приложение Limbo](../../tests/flashvm/live-values_pair.b) загружает [фильм AS2](../../tests/flashvm/live-values_pair.as). Фильм вызывает `vfs.open("/wide.dis").load()` и закрывает исходный файл. Полученный прокси продолжает работать. [Загрузка файлов](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_LIVE_DIS.ru.md)
2. [Сценарий вызовов фильма](../../tests/flashvm/live-values_calls.as) передаёт `array of string` и `array of big` в `wide`. Первый элемент каждого массива содержит цифры. Только в позиции `big` они декодируются в двоичное знаковое 64-битное значение. Dis заменяет вторую строку на `"copy"`, вычитает единицу из максимального `big` и возвращает оба массива как кортеж. AS видит правки в результате и сохраняет исходные массивы. [Вызов по схеме](../../reference/managed_invoke.ru.md)
3. `patch` проверяет минимальный `big` в однопольной ссылке `ref Wide`, записывает максимальный и возвращает ту же ссылку. После вызова AS видит изменение в исходном Array. Запись через возвращённый Array меняет тот же локальный объект. [Графовый вызов](../../reference/graph_invoke.ru.md)
4. `widepipe` получает массив из заимствованной очереди и отправляет его изменённое содержимое в новый канал результата. Вызывающий код удерживает канал, пока читает единственное сообщение. При пустой входной очереди Dis ожидал бы сообщения, поэтому фильм заполняет очередь перед вызовом. До публикации метки фильм закрывает оба канала и освобождает модуль через `os.unload(proxy)`. [Аренда канала](../../reference/channel_leases.ru.md)

Фрагмент обмена на стороне AS использует те же сигнатуры, что и запускаемый пример.

```actionscript
var queue = vfs.localChannel("a[b]", 1); // Одна ячейка для массива big.
var values = "".split("");
values.push("9223372036854775807", "-9223372036854775808");
if (queue.trySend(values)) {
    var reply = proxy.widepipe(queue); // Одолжить живую очередь на время вызова.
    var edited = reply.recv();        // ["9223372036854775806", "-9223372036854775807"]
    reply.close();                   // Освободить отдельную аренду результата.
}
queue.close();
```

В этом примере строки с цифрами задают точные значения. Целое из точного диапазона, например `42`, можно передать как AS `Number` и получить обратно как `Number`. Именованное сообщение без схемы сохраняет десятичную строку как текст. [Типы в AS](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_VALUE_TYPES.ru.md)

## Проверка той же записи через SDK для C

SDK принимает `int64_t` напрямую. Программа завершается с нулевым
статусом, если закодированный минимум `big` восстановлен точно. Соберите её
как C99 и подключите `StyxRpc::rpc` через `find_package(StyxRpc CONFIG REQUIRED)`.

```c
#include <styxrpc/managed.h>

int main(void)
{
    unsigned char bytes[64];
    size_t used;
    int64_t result;
    if (styxrpc_value_integer(STYXRPC_VALUE_BIG, INT64_MIN, bytes,
            sizeof(bytes), &used) != STYXRPC_OK)
        return 1; /* Полная вложенная запись не получена. */
    if (styxrpc_value_get_integer(STYXRPC_VALUE_BIG, bytes, used, &result) != STYXRPC_OK)
        return 1; /* Проверка записи со знаковым big завершилась ошибкой. */
    return result == INT64_MIN ? 0 : 1;
}
```

[Кодирование вложенной записи](../../reference/value_integer.ru.md) · [Декодирование вложенной записи](../../reference/value_get_integer.ru.md)
