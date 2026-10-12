/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/input/main.c.
 */

/* C89: configure multitap and swipe mappings. Host must recognize gestures. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/input.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config cfg;
    /* Сохраняем вычисленное значение: FVM_Runtime *r. */
    FVM_Runtime *r = 0;
    /* Сохраняем вычисленное значение: FVM_Context *c. */
    FVM_Context *c = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_InputRule rule;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_InputEvent e;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_MouseButton b;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &r) != FVM_OK ||
        /* Создаём отдельный контекст интерпретатора в среде выполнения. */
        fvm_context_create(r, &c) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов sizeof с указанными аргументами. */
    rule.struct_size = sizeof(rule);
    /* Сохраняем вычисленное значение: rule.gesture. */
    rule.gesture = FVM_GESTURE_TAP;
    /* Сохраняем вычисленное значение: rule.fingers. */
    rule.fingers = 2;
    /* Сохраняем вычисленное значение: rule.direction. */
    rule.direction = 0;
    /* Сохраняем вычисленное значение: rule.button. */
    rule.button = FVM_MOUSE_RIGHT;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_input_set_rule(c, &rule) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 2;

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: rule.gesture. */
    rule.gesture = FVM_GESTURE_SWIPE;
    /* Сохраняем вычисленное значение: rule.fingers. */
    rule.fingers = 3;
    /* Сохраняем вычисленное значение: rule.direction. */
    rule.direction = -2;
    /* Сохраняем вычисленное значение: rule.button. */
    rule.button = FVM_MOUSE_WHEEL_UP;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_input_set_rule(c, &rule) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 3;

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: e.gesture. */
    e.gesture = FVM_GESTURE_SWIPE;
    /* Сохраняем вычисленное значение: e.fingers. */
    e.fingers = 3;
    /* Сохраняем вычисленное значение: e.direction. */
    e.direction = -2;
    /* Сохраняем вычисленное значение: e.x. */
    e.x = 10;
    /* Сохраняем вычисленное значение: e.y. */
    e.y = 20;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_input_map(c, &e, &b) != FVM_OK || b != FVM_MOUSE_WHEEL_UP)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 4;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("3-finger upward swipe mapped to button %d\n", (int)b);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(c);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(r);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
