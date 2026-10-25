/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/prototypes/main.c.
 */

/* C89: prototype inheritance and property shadowing from a host application. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/value.h"

/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Initialize the example, run its API operations and release owned resources. */


/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: FVM_Runtime *runtime. */
    FVM_Runtime *runtime = 0;
    /* Сохраняем вычисленное значение: FVM_Context *context. */
    FVM_Context *context = 0;
    /* Сохраняем вычисленное значение: FVM_Object *base. */
    FVM_Object *base = 0, *instance = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value, result;
    /* Начинаем блок инструкций или описание объекта. */
    { /* Begin scope: Начинаем блок инструкций или описание объекта. */
        /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
        FVM_Config cfg;
        /* Проверяем условие перед выполнением следующей ветки. */
        if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &runtime) != FVM_OK)
        { /* Begin conditional branch: if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &runtime) != FVM_OK). */
            /* Возвращаем результат вызывающей стороне и завершаем функцию. */
            return 1;
        } /* End conditional branch: if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &runtime) != FVM_OK). */
        /* Завершаем текущий блок или описание объекта. */
    } /* End scope: Начинаем блок инструкций или описание объекта. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_context_create(runtime, &context) != FVM_OK)
    { /* Begin conditional branch: if (fvm_context_create(runtime, &context) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 2;
    } /* End conditional branch: if (fvm_context_create(runtime, &context) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_create(context, &base) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_create(context, &base) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 3;
    } /* End conditional branch: if (fvm_object_create(context, &base) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_create(context, &instance) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_create(context, &instance) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 4;
    } /* End conditional branch: if (fvm_object_create(context, &instance) != FVM_OK). */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&value, 42);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set(context, base, "inherited", &value) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set(context, base, "inherited", &value) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 5;
    } /* End conditional branch: if (fvm_object_set(context, base, "inherited", &value) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_set_prototype(context, instance, base) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_set_prototype(context, instance, base) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 6;
    } /* End conditional branch: if (fvm_object_set_prototype(context, instance, base) != FVM_OK). */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_object_get(context, instance, "inherited", &result) != FVM_OK)
    { /* Begin conditional branch: if (fvm_object_get(context, instance, "inherited", &result) != FVM_OK). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 7;
    } /* End conditional branch: if (fvm_object_get(context, instance, "inherited", &result) != FVM_OK). */

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("Inherited value: %.0f\n", result.number);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(context);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return result.number == 42 ? 0 : 8;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */
