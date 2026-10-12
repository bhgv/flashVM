/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/extending/channel_worker/main.c.
 */

/* C89: worker gets a retained channel through userdata.
 * Caller MUST join worker before releasing its retained handle. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/channel_value.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/worker.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Compute and return task. */
/* Выполняем вызов task с указанными аргументами. */
static FVM_Result task(FVM_Context *ctx, const FVM_Value *input, FVM_Value *output, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: FVM_ChannelValue *channel. */
    FVM_ChannelValue *channel = (FVM_ChannelValue *)userdata;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value message;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Result r;

    /* Computation / Вычисление и обновление состояния. */
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)ctx;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)input;
    /* Выполняем вызов fvm_channel_value_receive с указанными аргументами. */
    r = fvm_channel_value_receive(channel, &message);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (r == FVM_OK && message.type == FVM_VALUE_NUMBER)
        /* Записываем числовое значение и соответствующий тег типа. */
        fvm_value_number(output, message.number + 1);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return r;
    /* Завершаем текущий блок или описание объекта. */
}

/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_ChannelValue owner, worker_channel;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config config;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value input, message, result;
    /* Сохраняем вычисленное значение: FVM_Worker *worker. */
    FVM_Worker *worker = 0;
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_create(&owner) == FVM_OK);
    /* Удерживаем ещё одну ссылку на общий канал без копирования очереди. Проверяем успешность
     * вызова. */
    assert(fvm_channel_value_copy(&worker_channel, &owner) == FVM_OK);

    /* Initialization / Инициализация локального состояния. */
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&input);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_run(&config, task, &input, &worker_channel, &worker) == FVM_OK);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&message, 41);

    /* Output / Вывод и передача результата. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_send(&owner, &message) == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Ожидаем завершения рабочего потока. Проверяем успешность вызова. */
    assert(fvm_worker_join(worker, &result) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(result.type == FVM_VALUE_NUMBER && result.number == 42);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем объект рабочего потока после завершения его работы. */
    fvm_worker_destroy(worker);
    /* Освобождаем одну удержанную ссылку на канал. */
    fvm_channel_value_release(&worker_channel);
    /* Освобождаем одну удержанную ссылку на канал. */
    fvm_channel_value_release(&owner);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
