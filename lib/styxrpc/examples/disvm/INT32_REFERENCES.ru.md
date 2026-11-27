# Отмена доступа к живому экспорту Int32

[English](INT32_REFERENCES.md) · [Примеры](../README.ru.md) · [Протокол ссылок на экспорты](../../reference/module_refs.ru.md)

Процесс DisVM публикует переменную Int32 на чтение и запись, а значение
держит приложение Limbo. Оно посылает ссылку через парную службу
именованных каналов. Задача AS2 открывает файл в обычной VFS выдавшей
стороны при первом обращении к ссылке. Пример прерывает задачу в пяти
точках, после чего новая задача открывает тот же экспорт.

## Сборка и запуск

Нужны нативный DisVM и плеер FlashVM со StyxRPC под Linux64.
Соберите фронтенд AS2, проверку и оптимизатор FIR, а также линкер SWF.
Сборка описана в руководствах
[плеера FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.ru.md)
и [DisVM](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-CHANNEL.md).
Запустите из корня репозитория. При другой раскладке сборки подставьте
свои пути к инструментам.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl \
    --cancel-int-ref
```

`--source` задаёт каталог модулей DisVM и исходников Limbo. Скрипт
компилирует их через `--limbo` и запускает образ через `--runtime`.
Далее скрипт компилирует задачу AS2 и фильм в FIR, проверяет и оптимизирует
FIR, выпускает объекты и собирает SWF перед запуском `--player`.
По умолчанию используются интерпретатор и пары сокетов Unix. Параметр
`--jit` включает нативную компиляцию, а `--tcp` использует петлевые
TCP-подключения. Скрипт управляет временным каталогом, процессами и
сокетами. При успехе появляется `PASS native cancel-int-ref RPC paired VMs`.

При настройке парных тестов SDK задайте `STYXRPC_FLASHVM_TOOLS` как
каталог сборки инструментов, в примере это `out`. Если задан
`STYXRPC_FLASHVM_PLAYER`, CTest регистрирует четыре сочетания режима и
транспорта под именами `styxrpc-native-int-ref-*`. Если SDK собран в
`build/rpc`, запустите
`ctest --test-dir build/rpc -R styxrpc-native-int-ref --output-on-failure`.

## Ход обмена

1. Владелец Limbo публикует `/mounted/counter.dis/.var/value` через
   `Styxnodes->publishat` в режиме `rw` с длиной четыре байта. Цикл
   обслуживания начинает со значения `17` и принимает только полные
   значения со смещения ноль. `capturelocal` сохраняет Qid публикации и
   режим. В каждой встрече по именованному каналу владелец передаёт
   тот же описатель. [Исходник Limbo](../../tests/flashvm/cancel-int-ref_pair.b) ·
   [Порт ссылок](../../reference/module_ref_port.ru.md).
2. [Задача AS2](../../tests/flashvm/cancel-int-ref_worker.as) получает
   живую ссылку через `os.chan`. При чтении числа она получает текущее
   значение владельца. Присваивание меняет это значение. У каждой задачи
   свой контекст. При завершении освобождаются канал и fid задачи.
3. [Тестовый ретранслятор](../../tests/flashvm/int_ref_gate.py) удерживает
   по одному исходному ответу для каждой задачи. Последовательно
   проверяются stat до открытия, открытие, stat открытого fid, чтение и
   запись. Перед отменой [фильм](../../tests/flashvm/cancel-dis-setup_pair.as)
   читает соседний файл в той же сессии. Ретранслятор требует flush
   исходного тега и clunk точного fid. Этот барьер управляет временем
   доставки подтверждений в тесте.
4. Пока ответ на запись удерживается, владелец Limbo уже хранит `42`.
   Следующая задача заново открывает исходную ссылку, читает `42` и
   заменяет на `73`. Ретранслятор проверяет обе записи и их порядок.
   Владелец проверяет итог перед закрытием ссылки и снятием публикации.
   При ошибке передачи или отсутствии маркера восстановления тест
   завершается с ошибкой, скрипт освобождает оба процесса.

Для проверки замены публикации запустите ту же команду с
`--modref-reverse` вместо `--cancel-int-ref`. Limbo сохраняет старые
ссылки при замене по тому же пути. Их доступ прекращается, а новая
ссылка с новым Qid читает `77`.
[Исходник проверки замены](../../tests/flashvm/modref-reverse_pair.b).

## То же правило в C

Функция заново проверяет исходный Qid и выдаёт текущее значение после
полного чтения. Передайте подключённую сессию исходного владельца,
декодированный описатель и колбэк остановки. Для отдельной попытки
восстановления используйте новое состояние колбэка.

```c
#include <stddef.h>
#include <styxrpc/module_ref.h>

/* Заимствуем сессию, описатель и состояние остановки до возврата. */
static StyxRpcIoOutcome
read_export(struct StyxClient *issuer, const StyxRpcModuleRef *ref,
    int32_t *value, StyxRpcVfsStop stop, void *state)
{
    StyxRpcVfsFile *file = NULL;  /* При успехе получаем собственный fid. */
    StyxRpcIoOutcome status = styxrpc_module_ref_open_remote_until(
        issuer, ref, &file, stop, state);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_int_read_until(file, value, stop, state);
        styxrpc_vfs_close(file, 1);  /* Освобождаем локально с отложенным clunk. */
    }
    return status;  /* Чтение меняет *value только при успехе. */
}
```

Если [запись](../../reference/vfs_int_write_until.ru.md) вернула
`STYXRPC_IO_UNCERTAIN`, закройте её fid и по логике приложения решите,
нужно ли перечитать текущее значение. Избегайте автоматического повтора
присваивания. Сохраняйте подключение во время операций с его fid.
[Точное открытие](../../reference/module_ref_open_remote_until.ru.md) ·
[Чтение с отменой](../../reference/vfs_int_read_until.ru.md) · [Закрытие](../../reference/vfs_close.ru.md).
