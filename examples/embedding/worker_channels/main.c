/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/worker_channels/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/worker.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/channel.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>

/* Define Exchange struct and its representation. */
/* Объявляем тип, используемый в этом примере. */
typedef struct Exchange
/* Начинаем блок инструкций или описание объекта. */
{
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Channel *requests;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Channel *replies;
    /* Завершаем выражение с указанным значением или аргументами. */
} Exchange;

/* The callback runs in its own VM. The channels, not Flash objects,
 * are the only shared data between this context and the host. */
/* Compute and return exchange worker. */
/* Выполняем вызов exchange_worker с указанными аргументами. */
static FVM_Result
/* Продолжаем объявление функции обмена в рабочем потоке. */
exchange_worker(FVM_Context *ctx, const FVM_Value *input, FVM_Value *output,
                /* Продолжаем объявление или выражение с указанными параметрами. */
                void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: Exchange *ex. */
    Exchange *ex = (Exchange *)userdata;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value request, reply;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Result r;

    /* Computation / Вычисление и обновление состояния. */
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)ctx;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)input;
    /* Получаем значение; вызов ожидает сообщение или закрытие канала. */
    r = fvm_channel_receive(ex->requests, &request);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (r != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return r;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (request.type != FVM_VALUE_STRING)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_INVALID_ARGUMENT;
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&reply, (double)strlen(request.string));

    /* Output / Вывод и передача результата. */
    /* Отправляем значение; вызов может ожидать освобождения очереди. */
    r = fvm_channel_send(ex->replies, &reply);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (r == FVM_OK)
        /* Записываем числовое значение и соответствующий тег типа. */
        fvm_value_number(output, 42.0);
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
    FVM_Config cfg;
    /* Сохраняем вычисленное значение: FVM_Worker *worker. */
    FVM_Worker *worker = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value input, message, reply, output;
    /* Завершаем выражение с указанным значением или аргументами. */
    Exchange ex;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    int available;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&cfg) == FVM_OK);
    /* Создаём канал для обмена сообщениями внутри процесса. Проверяем успешность вызова. */
    assert(fvm_channel_create(&ex.requests) == FVM_OK);
    /* Создаём канал для обмена сообщениями внутри процесса. Проверяем успешность вызова. */
    assert(fvm_channel_create(&ex.replies) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_try_receive(ex.replies, &reply) == FVM_NOT_FOUND);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&input);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_run_suspended(&cfg, exchange_worker, &input, &ex, &worker) == FVM_OK);
    /* Создаём строковое представление значения по правилам API. Проверяем успешность вызова. */
    assert(fvm_value_string(&message, "hello") == FVM_OK);

    /* Output / Вывод и передача результата. */
    /* Отправляем значение; вызов может ожидать освобождения очереди. Проверяем успешность вызова.
     */
    assert(fvm_channel_send(ex.requests, &message) == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_start(worker) == FVM_OK);
    /* Получаем значение; вызов ожидает сообщение или закрытие канала. Проверяем успешность вызова.
     */
    assert(fvm_channel_receive(ex.replies, &reply) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(reply.type == FVM_VALUE_NUMBER && reply.number == 5.0);
    /* Ожидаем завершения рабочего потока. Проверяем успешность вызова. */
    assert(fvm_worker_join(worker, &output) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(output.type == FVM_VALUE_NUMBER && output.number == 42.0);
    /* Освобождаем объект рабочего потока после завершения его работы. */
    fvm_worker_destroy(worker);

    /* Computation / Вычисление и обновление состояния. */
    /* Закрываем канал и пробуждаем ожидающих участников. */
    fvm_channel_close(ex.requests);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_try_receive(ex.requests, &reply) == FVM_ERROR);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_available(ex.requests, &available) == FVM_OK && !available);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем принадлежащую примеру ссылку на канал. */
    fvm_channel_destroy(ex.requests);
    /* Освобождаем принадлежащую примеру ссылку на канал. */
    fvm_channel_destroy(ex.replies);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
