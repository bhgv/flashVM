/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/gesture_dispatch/main.c.
 */

/* C89 example: map a two-finger tap to right mouse button and observe it. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/flashvm.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/input.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Implement on gesture for this subsystem. */
/* Определяем функцию on_gesture; параметры передаются вызывающей стороной в указанном порядке. */
static void on_gesture(FVM_Context *context, const FVM_InputEvent *event, FVM_InputPhase phase,
                       /* Продолжаем объявление или выражение с указанными параметрами. */
                       FVM_MouseButton button, void *userdata)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Computation / Вычисление и обновление состояния. */
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)context;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)userdata;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("gesture=%d fingers=%u phase=%d mouse=%d\n", (int)event->gesture, event->fingers,
           /* Завершаем выражение с указанным значением или аргументами. */
           (int)phase, (int)button);
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
    FVM_InputRule rule;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_InputEvent event;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_context_create(runtime, &context) != FVM_OK)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Освобождаем среду выполнения после завершения работы контекстов. */
        fvm_runtime_destroy(runtime);
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
        /* Завершаем текущий блок или описание объекта. */
    }

    /* Computation / Вычисление и обновление состояния. */
    /* Определяем или используем sizeof; назначение и владение пояснены у функции. */
    rule.struct_size = sizeof(rule);
    /* Сохраняем вычисленное значение: rule.gesture. */
    rule.gesture = FVM_GESTURE_TAP;
    /* Сохраняем вычисленное значение: rule.fingers. */
    rule.fingers = 2;
    /* Сохраняем вычисленное значение: rule.direction. */
    rule.direction = 0;
    /* Сохраняем вычисленное значение: rule.button. */
    rule.button = FVM_MOUSE_RIGHT;
    /* Вызываем fvm_input_set_rule: устанавливаем правило преобразования ввода.
     * context — контекст интерпретатора.
     * &rule — правило преобразования события ввода. */
    fvm_input_set_rule(context, &rule);
    /* Вызываем fvm_input_set_callback: назначаем нативный обработчик ввода.
     * context — контекст интерпретатора.
     * on_gesture — нативная функция обратного вызова.
     * 0 — не копируемые данные хоста, живущие до завершения вызовов. */
    fvm_input_set_callback(context, on_gesture, 0);
    /* Сохраняем вычисленное значение: event.gesture. */
    event.gesture = FVM_GESTURE_TAP;
    /* Сохраняем вычисленное значение: event.fingers. */
    event.fingers = 2;
    /* Сохраняем вычисленное значение: event.direction. */
    event.direction = 0;
    /* Сохраняем вычисленное значение: event.x. */
    event.x = 100;
    /* Сохраняем вычисленное значение: event.y. */
    event.y = 50;
    /* Вызываем fvm_input_dispatch: передаём событие зарегистрированному обработчику.
     * context — контекст интерпретатора.
     * &event — описание события ввода.
     * FVM_INPUT_END — фаза доставки события. */
    fvm_input_dispatch(context, &event, FVM_INPUT_END);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(context);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
