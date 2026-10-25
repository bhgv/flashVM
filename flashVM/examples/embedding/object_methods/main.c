/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/object_methods/main.c.
 */

/* C89 example: call a native method with an explicit ActionScript receiver. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/value.h"

/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Add the current value. */


/* Определяем функцию add; параметры передаются вызывающей стороной в указанном порядке. */
static FVM_Result add(FVM_Context *ctx, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                      /* Продолжаем объявление или выражение с указанными параметрами. */
                      FVM_Value *result, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value base;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)userdata;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (argc != 1 || args[0].type != FVM_VALUE_NUMBER)
    { /* Begin conditional branch: if (argc != 1 || args[0].type != FVM_VALUE_NUMBER). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_INVALID_ARGUMENT;
    } /* End conditional branch: if (argc != 1 || args[0].type != FVM_VALUE_NUMBER). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_get(ctx, self, "base", &base) != FVM_OK || base.type != FVM_VALUE_NUMBER)
    { /* Begin conditional branch: if (fvm_object_get(ctx, self, "base", &base) != FVM_OK || base.type != FVM_VALUE_NUMBER). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    } /* End conditional branch: if (fvm_object_get(ctx, self, "base", &base) != FVM_OK || base.type != FVM_VALUE_NUMBER). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(result, base.number + args[0].number);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return FVM_OK;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */


/* Initialize the example, run its API operations and release owned resources. */


/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config config;
    /* Сохраняем вычисленное значение: FVM_Runtime *runtime. */
    FVM_Runtime *runtime = 0;
    /* Сохраняем вычисленное значение: FVM_Context *ctx. */
    FVM_Context *ctx = 0;
    /* Сохраняем вычисленное значение: FVM_Object *obj. */
    FVM_Object *obj = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value base, arg, result;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK)
    { /* Begin conditional branch: if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    } /* End conditional branch: if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_context_create(runtime, &ctx) != FVM_OK)
    { /* Begin conditional branch: if (fvm_context_create(runtime, &ctx) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 2;
    } /* End conditional branch: if (fvm_context_create(runtime, &ctx) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_create(ctx, &obj) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_create(ctx, &obj) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 3;
    } /* End conditional branch: if (fvm_object_create(ctx, &obj) != FVM_OK). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&base, 35);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set(ctx, obj, "base", &base) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set(ctx, obj, "base", &base) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 4;
    } /* End conditional branch: if (fvm_object_set(ctx, obj, "base", &base) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_register_function(ctx, "add", add, 0) != FVM_OK)
    { /* Begin conditional branch: if (fvm_register_function(ctx, "add", add, 0) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 5;
    } /* End conditional branch: if (fvm_register_function(ctx, "add", add, 0) != FVM_OK). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&arg, 7);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_call(ctx, obj, "add", 1, &arg, &result) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_call(ctx, obj, "add", 1, &arg, &result) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 6;
    } /* End conditional branch: if (fvm_object_call(ctx, obj, "add", 1, &arg, &result) != FVM_OK). */

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("Result: %.0f\n", result.number); /* Expected: 42 */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(ctx);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return result.type == FVM_VALUE_NUMBER && result.number == 42 ? 0 : 7;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */
