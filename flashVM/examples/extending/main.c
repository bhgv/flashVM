/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/extending/main.c.
 */

/* C89 native extension demo. This is statically registered, not a DLL. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/flashvm.h"

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/value.h"

/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Compute and return make point. */


/* Определяем функцию make_point; параметры передаются вызывающей стороной в указанном порядке. */
static FVM_Result make_point(FVM_Context *ctx, FVM_Object *self, unsigned int argc,
                             /* Продолжаем объявление или выражение с указанными параметрами. */
                             const FVM_Value *args, FVM_Value *out, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: FVM_Object *point. */
    FVM_Object *point = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value coordinate;


    /* Computation / Вычисление и обновление состояния. */
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
    /* Native C creates an object accessible through the public VM API. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_create(ctx, &point) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_create(ctx, &point) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_OUT_OF_MEMORY;
    } /* End conditional branch: if (fvm_object_create(ctx, &point) != FVM_OK). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&coordinate, args[0].number);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set(ctx, point, "x", &coordinate) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set(ctx, point, "x", &coordinate) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    } /* End conditional branch: if (fvm_object_set(ctx, point, "x", &coordinate) != FVM_OK). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&coordinate, args[1].number);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set(ctx, point, "y", &coordinate) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set(ctx, point, "y", &coordinate) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return FVM_ERROR;
    } /* End conditional branch: if (fvm_object_set(ctx, point, "y", &coordinate) != FVM_OK). */
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: out->type. */
    out->type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: out->object. */
    out->object = point;
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return FVM_OK;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */


/* Called by host; not yet a dynamic plugin entry point. */
/* Register extension. */


/* Определяем функцию register_extension; параметры передаются вызывающей стороной в указанном порядке. */
static FVM_Result register_extension(FVM_Context *ctx)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return fvm_register_function(ctx, "makePoint", make_point, 0);
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
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value args[2], got, x, y;
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
    /* Проверяем условие перед выполнением следующей ветки. */
    if (register_extension(ctx) != FVM_OK)
    { /* Begin conditional branch: if (register_extension(ctx) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (register_extension(ctx) != FVM_OK). */


    /* Initialization / Инициализация локального состояния. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 12);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[1], 34);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_call(ctx, "makePoint", 2, args, &got) != FVM_OK)
    { /* Begin conditional branch: if (fvm_call(ctx, "makePoint", 2, args, &got) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_call(ctx, "makePoint", 2, args, &got) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (got.type != FVM_VALUE_OBJECT)
    { /* Begin conditional branch: if (got.type != FVM_VALUE_OBJECT). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (got.type != FVM_VALUE_OBJECT). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_get(ctx, got.object, "x", &x) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_get(ctx, got.object, "x", &x) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_object_get(ctx, got.object, "x", &x) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_get(ctx, got.object, "y", &y) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_get(ctx, got.object, "y", &y) != FVM_OK). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (fvm_object_get(ctx, got.object, "y", &y) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (x.type != FVM_VALUE_NUMBER || y.type != FVM_VALUE_NUMBER)
    { /* Begin conditional branch: if (x.type != FVM_VALUE_NUMBER || y.type != FVM_VALUE_NUMBER). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (x.type != FVM_VALUE_NUMBER || y.type != FVM_VALUE_NUMBER). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (x.number != 12 || y.number != 34)
    { /* Begin conditional branch: if (x.number != 12 || y.number != 34). */
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    } /* End conditional branch: if (x.number != 12 || y.number != 34). */


    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("makePoint => (%.0f, %.0f)\n", x.number, y.number);
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
