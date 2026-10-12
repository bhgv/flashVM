/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/workers/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/worker.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
/* Compute and return task. */
/* Выполняем вызов task с указанными аргументами. */
static FVM_Result task(FVM_Context *ctx, const FVM_Value *in, FVM_Value *out, void *ud)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)ud;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (!ctx || in->type != FVM_VALUE_STRING || strcmp(in->string, "hello"))
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&value, 42);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_set_global(ctx, "answer", &value) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return fvm_get_global(ctx, "answer", out);
    /* Завершаем текущий блок или описание объекта. */
}

/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config config;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value in, out;
    /* Сохраняем вычисленное значение: FVM_Worker *worker. */
    FVM_Worker *worker = 0;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Создаём строковое представление значения по правилам API. Проверяем успешность вызова. */
    assert(fvm_value_string(&in, "hello") == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_run(&config, task, &in, 0, &worker) == FVM_OK);
    /* Ожидаем завершения рабочего потока. Проверяем успешность вызова. */
    assert(fvm_worker_join(worker, &out) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(out.type == FVM_VALUE_NUMBER && out.number == 42);
    /* Освобождаем объект рабочего потока после завершения его работы. */
    fvm_worker_destroy(worker);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
