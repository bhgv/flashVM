/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/gesture_recognizer/main.c.
 */

/* C89 example: host delivers touch samples to FlashVM. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/gesture_recognizer.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Implement on gesture for this subsystem. */
/* Определяем функцию on_gesture; параметры передаются вызывающей стороной в указанном порядке. */
static void on_gesture(FVM_Context *ctx, const FVM_InputEvent *event, FVM_InputPhase phase,
                       /* Продолжаем объявление или выражение с указанными параметрами. */
                       FVM_MouseButton button, void *user)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Computation / Вычисление и обновление состояния. */
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)ctx;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)phase;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)user;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("gesture=%d fingers=%u mapped_mouse=%d\n", (int)event->gesture, event->fingers,
           /* Завершаем выражение с указанным значением или аргументами. */
           (int)button);
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
    FVM_GestureRecognizer recognizer;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_InputRule rule;
    /* Сохраняем вычисленное значение: int dispatched. */
    int dispatched = 0;
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
    /* Two-finger tap behaves like a right mouse click. */
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
    /* Определяем или используем sizeof; назначение и владение пояснены у функции. */
    recognizer.struct_size = sizeof(recognizer);
    /* Вызываем fvm_gesture_recognizer_init: инициализируем распознаватель жестов.
     * &recognizer — состояние распознавателя жестов. */
    fvm_gesture_recognizer_init(&recognizer);

    /* Computation / Вычисление и обновление состояния. */
    /* Вызываем fvm_gesture_recognizer_feed: передаём изменение касания распознавателю жестов.
     * context — контекст интерпретатора.
     * &recognizer — состояние распознавателя жестов.
     * FVM_TOUCH_BEGIN — фаза касания: начало, движение, завершение или отмена.
     * 2 — число пальцев, участвующих в событии.
     * 10 — горизонтальная координата события.
     * 20 — вертикальная координата события.
     * 100 — время события в миллисекундах.
     * &dispatched — адрес для признака доставки распознанного жеста. */
    fvm_gesture_recognizer_feed(context, &recognizer, FVM_TOUCH_BEGIN, 2, 10, 20, 100, &dispatched);
    /* Вызываем fvm_gesture_recognizer_feed: передаём изменение касания распознавателю жестов.
     * context — контекст интерпретатора.
     * &recognizer — состояние распознавателя жестов.
     * FVM_TOUCH_END — фаза касания: начало, движение, завершение или отмена.
     * 0 — число пальцев, участвующих в событии.
     * 11 — горизонтальная координата события.
     * 20 — вертикальная координата события.
     * 160 — время события в миллисекундах.
     * &dispatched — адрес для признака доставки распознанного жеста. */
    fvm_gesture_recognizer_feed(context, &recognizer, FVM_TOUCH_END, 0, 11, 20, 160, &dispatched);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(context);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return dispatched ? 0 : 1;
    /* Завершаем текущий блок или описание объекта. */
}
