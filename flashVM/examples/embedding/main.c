/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/main.c.
 */

/* C89 embedding demo: object globals and a registered native function.
 * SWF/ActionScript execution is not yet implemented.
 */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/flashvm.h"

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/value.h"

/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Compute and return sum. */


/* Определяем функцию sum; параметры передаются вызывающей стороной в указанном порядке. */
static FVM_Result sum(FVM_Context *ctx, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                      /* Продолжаем объявление или выражение с указанными параметрами. */
                      FVM_Value *out, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */


    /* Computation / Вычисление и обновление состояния. */
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)ctx;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)self;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)userdata;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (argc != 2 || args[0].type != FVM_VALUE_NUMBER || args[1].type != FVM_VALUE_NUMBER)
    { /* Begin conditional branch: if (argc != 2 || args[0].type != FVM_VALUE_NUMBER || args[1].type != FVM_VALUE_NUMBER). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_INVALID_ARGUMENT;
    } /* End conditional branch: if (argc != 2 || args[0].type != FVM_VALUE_NUMBER || args[1].type != FVM_VALUE_NUMBER). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(out, args[0].number + args[1].number);
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
    FVM_Config cfg;
    /* Сохраняем вычисленное значение: FVM_Runtime *rt. */
    FVM_Runtime *rt = 0;
    /* Сохраняем вычисленное значение: FVM_Context *ctx. */
    FVM_Context *ctx = 0;
    /* Сохраняем вычисленное значение: FVM_Object *obj. */
    FVM_Object *obj = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value, got, args[2];
    /* Сохраняем вычисленное значение: int status. */
    int status = 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_config_init(&cfg) != FVM_OK)
    { /* Begin conditional branch: if (fvm_config_init(&cfg) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    } /* End conditional branch: if (fvm_config_init(&cfg) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_runtime_create(&cfg, &rt) != FVM_OK)
    { /* Begin conditional branch: if (fvm_runtime_create(&cfg, &rt) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    } /* End conditional branch: if (fvm_runtime_create(&cfg, &rt) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_context_create(rt, &ctx) != FVM_OK)
    { /* Begin conditional branch: if (fvm_context_create(rt, &ctx) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_context_create(rt, &ctx) != FVM_OK). */
    /* VM owns this object until context destruction. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_create(ctx, &obj) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_create(ctx, &obj) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_object_create(ctx, &obj) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_value_string(&value, "Hello from C host") != FVM_OK)
    { /* Begin conditional branch: if (fvm_value_string(&value, "Hello from C host") != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_value_string(&value, "Hello from C host") != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set(ctx, obj, "message", &value) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set(ctx, obj, "message", &value) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_object_set(ctx, obj, "message", &value) != FVM_OK). */
    /* Publish the object to the context's global namespace. */
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&value);

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: value.type. */
    value.type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: value.object. */
    value.object = obj;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_set_global(ctx, "hostObject", &value) != FVM_OK)
    { /* Begin conditional branch: if (fvm_set_global(ctx, "hostObject", &value) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_set_global(ctx, "hostObject", &value) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_get_global(ctx, "hostObject", &got) != FVM_OK)
    { /* Begin conditional branch: if (fvm_get_global(ctx, "hostObject", &got) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_get_global(ctx, "hostObject", &got) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (got.type != FVM_VALUE_OBJECT || got.object != obj)
    { /* Begin conditional branch: if (got.type != FVM_VALUE_OBJECT || got.object != obj). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (got.type != FVM_VALUE_OBJECT || got.object != obj). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_get(ctx, obj, "message", &got) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_get(ctx, obj, "message", &got) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_object_get(ctx, obj, "message", &got) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (got.type != FVM_VALUE_STRING)
    { /* Begin conditional branch: if (got.type != FVM_VALUE_STRING). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (got.type != FVM_VALUE_STRING). */


    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("%s\n", got.string); /* Borrowed string; don't free it. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_register_function(ctx, "nativeSum", sum, 0) != FVM_OK)
    { /* Begin conditional branch: if (fvm_register_function(ctx, "nativeSum", sum, 0) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_register_function(ctx, "nativeSum", sum, 0) != FVM_OK). */


    /* Initialization / Инициализация локального состояния. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 10);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[1], 20);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_call(ctx, "nativeSum", 2, args, &got) != FVM_OK)
    { /* Begin conditional branch: if (fvm_call(ctx, "nativeSum", 2, args, &got) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_call(ctx, "nativeSum", 2, args, &got) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (got.type != FVM_VALUE_NUMBER || got.number != 30)
    { /* Begin conditional branch: if (got.type != FVM_VALUE_NUMBER || got.number != 30). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (got.type != FVM_VALUE_NUMBER || got.number != 30). */


    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("nativeSum(10, 20) = %.0f\n", got.number);
    /* Сохраняем вычисленное значение: status. */
    status = 0;
/* Точка перехода для общей ветки обработки и очистки. */
done:
    /* Проверяем условие перед выполнением следующей ветки. */
    if (ctx)
    { /* Begin conditional branch: if (ctx). */
        /* Освобождаем контекст и принадлежащие ему объекты. */
        fvm_context_destroy(ctx);
    } /* End conditional branch: if (ctx). */
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(rt);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return status;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */
