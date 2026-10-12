/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/extending/lua_style/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/auxlib.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Arguments remain on the callback stack. Returning 1 selects the topmost
 * value as the result; the host bridge discards the callback's arguments. */
/* Add the current value. */
/* Выполняем вызов add с указанными аргументами. */
static int add(FVM_Stack *stack)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Организуем повторение операций до выполнения условия завершения. */
    double left, right;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Result result;
    /* Выполняем вызов fvm_stack_checknumber с указанными аргументами. */
    result = fvm_stack_checknumber(stack, 1, &left);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (result != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return result;
    /* Выполняем вызов fvm_stack_checknumber с указанными аргументами. */
    result = fvm_stack_checknumber(stack, 2, &right);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (result != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return result;
    /* Выполняем вызов fvm_stack_pushnumber с указанными аргументами. */
    result = fvm_stack_pushnumber(stack, left + right);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return result == FVM_OK ? 1 : result;
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
    /* Сохраняем вычисленное значение: FVM_Runtime *runtime. */
    FVM_Runtime *runtime = 0;
    /* Сохраняем вычисленное значение: FVM_Context *context. */
    FVM_Context *context = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value args[2], result;
    /* Сохраняем вычисленное значение: int status. */
    int status = 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_config_init(&config) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_context_create(runtime, &context) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_stack_register_cfunction(context, "add", add, 0) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;

    /* Initialization / Инициализация локального состояния. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 20);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[1], 22);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_call(context, "add", 2, args, &result) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (result.type != FVM_VALUE_NUMBER || result.number != 42)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("add(20, 22) = %.0f\n", result.number);
    /* Сохраняем вычисленное значение: status. */
    status = 0;
/* Точка перехода для общей ветки обработки и очистки. */
done:
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(context);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return status;
    /* Завершаем текущий блок или описание объекта. */
}
