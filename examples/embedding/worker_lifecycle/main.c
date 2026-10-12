/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/worker_lifecycle/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/worker.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Compute and return callback. */
/* Выполняем вызов callback с указанными аргументами. */
static FVM_Result callback(FVM_Context *ctx, const FVM_Value *in, FVM_Value *out, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)userdata;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (!ctx || in->type != FVM_VALUE_NUMBER)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(out, in->number + 1);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return FVM_OK;
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
    /* Сохраняем вычисленное значение: FVM_Worker *w. */
    FVM_Worker *w = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_WorkerState state;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value in, out;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&cfg) == FVM_OK);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&in, 41);

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_run_suspended(&cfg, callback, &in, 0, &w) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_state(w, &state) == FVM_OK && state == FVM_WORKER_SUSPENDED);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_start(w) == FVM_OK);
    /* Ожидаем завершения рабочего потока. Проверяем успешность вызова. */
    assert(fvm_worker_join(w, &out) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(out.type == FVM_VALUE_NUMBER && out.number == 42);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_worker_state(w, &state) == FVM_OK && state == FVM_WORKER_JOINED);
    /* Освобождаем объект рабочего потока после завершения его работы. */
    fvm_worker_destroy(w);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
