# Отмена ожидания Limbo на экспорте Int32 из FlashVM

[English](REF_PORT_CANCELLATION.md) · [Каталог примеров](../README.ru.md) · [Порт ссылки](../../reference/module_ref_port.ru.md)

Приложение Limbo может получить живую ссылку Int32 и использовать её
после закрытия именованного канала. Гость открывает свой порт
`/.varref`. Порт владеет описателем и через общий SDK C разрешает
экспорт на исходном подключении к FlashVM. Прямой родитель в DisVM
может вызвать `Processctl.cancelwait(pid)`, пока потомок ожидает внутри
`Styxmodref.capture`, `Cell.read` или `Cell.write`.

При отмене операции над значением порт освобождает затронутый
удалённый fid. Следующее чтение того же порта проверяет полный исходный
Qid и получает текущее значение. Применённое присваивание может
сохраниться при потере подтверждения. Пара использует штатные кадры
Int32, а также `Tflush` и clunk из 9P.

## Запустите пару

Соберите нативный DisVM с терминальным запуском, компилятор Limbo,
плеер FlashVM со StyxRPC, фронтенд AS2, инструменты FIR и линкер SWF.
См. [настройку DisVM](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-CHANNEL.md)
и [справку плеера](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.ru.md).
Запустите из корня репозитория, подставив свои пути к сборкам.

```sh
python3 lib/styxrpc/tests/flashvm/mixed_pair.py \
    --source disVM --runtime build/disvm/disvm \
    --limbo build/disvm/bootstrap/limbo-bootstrap \
    --player build/flash/flashvm_styx_player \
    --as2 out/as2/as2_viewc --fir out/fir/fir1 \
    --fir-opt out/fir/fir1_opt --swl out/linker/swl \
    --cancel-ref-port
```

Скрипт берёт исходники и модули DisVM из `--source`. Он компилирует
приложение Limbo и адаптер через `--limbo`, затем запускает их через
`--runtime`. Фильм собирается фронтендом AS2, проверкой и оптимизатором
FIR, а затем линкером SWF перед загрузкой в `--player`. По умолчанию
используются интерпретатор и пары сокетов Unix. Параметр `--jit`
выбирает JIT, а `--tcp` включает петлевой TCP.

При успехе выводится `PASS native cancel-ref-port RPC paired VMs`.
Скрипт дожидается обоих процессов и закрывает сокеты ретранслятора.
Пропущенный flush, незакрытый fid, повтор присваивания или допуск
устаревшей ссылки завершают проверку с ошибкой. При обычной настройке
интеграции DisVM укажите также корень сборки инструментов
`STYXRPC_FLASHVM_TOOLS` и исполняемый плеер `STYXRPC_FLASHVM_PLAYER`.
Все четыре сочетания запускаются командой
`ctest --test-dir build/rpc -R styxrpc-native-ref-port --output-on-failure`.

## Ход обмена

1. [Фильм AS2](../../tests/flashvm/cancel-ref-port_pair.as) публикует
   записываемый счётчик со значением `17`.
   [Родитель Limbo](../../tests/flashvm/cancel-ref-port_pair.b) захватывает
   Qid, копирует описатель и принимает его в порт. Второй, ещё
   не открытый алиас остаётся для проверки замены.
2. Через последовательные вызовы у прямых потомков родитель отменяет
   stat перед открытием, открытие, stat открытого fid, чтение и запись
   значения. [Ретранслятор](../../tests/flashvm/int_ref_gate.py) удерживает
   исходный ответ на прямом плече. Перед каждой отменой родитель выполняет
   запрос на соседнем fid. Проверка требует flush именно исходного тега
   и подтверждённый clunk точного временного fid.
3. Удержанная запись уже установила `42` во FlashVM. Родитель
   сверяет исходные байты описателя и читает `42` через тот же порт
   Limbo. Затем отдельной операцией записывает и проверяет `73`.
4. Родитель перезапускает фильм и ждёт новый Qid. Прежняя открытая
   ссылка и старый отложенный алиас дают отказ. Новый захват читает
   `17`. Родитель закрывает порты. Так проверяются идентичность и
   независимое состояние новой публикации.

## Используйте порт из C

Хост C может задать абсолютный срок вместо источника отмены гостя.
Переданный `timespec` содержит инициализированный срок на
`CLOCK_MONOTONIC`. При ошибке часов колбэк запрашивает остановку.

```c
#define _POSIX_C_SOURCE 200809L  /* Доступ к clock_gettime под Linux. */
#include <time.h>
#include <styxrpc/module_ref_port.h>

static int deadline_reached(void *state)
{
    const struct timespec *deadline = state;  /* Заимствуем до возврата. */
    struct timespec now;                     /* Текущее монотонное время. */
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return 1;  /* Остановить, если сравнение невозможно. */
    }
    return now.tv_sec > deadline->tv_sec ||
        (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec);
}
```

Вызовите [чтение с отменой](../../reference/module_ref_port_read_until.ru.md)
или [запись с отменой](../../reference/module_ref_port_write_until.ru.md)
с этим колбэком и действующим портом. Сериализуйте операции порта.
Если после неопределённой записи надо узнать значение, перечитайте
его с новым сроком. За это время значение мог изменить другой писатель.
После работы [закройте порт](../../reference/module_ref_port_close.ru.md).
