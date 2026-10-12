/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/channel_values/main.c.
 */

/* C89 example: a copied handle refers to the same synchronized queue. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/channel_value.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_ChannelValue producer, consumer;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value sent, received;
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_create(&producer) == FVM_OK);
    /* Удерживаем ещё одну ссылку на общий канал без копирования очереди. Проверяем успешность
     * вызова. */
    assert(fvm_channel_value_copy(&consumer, &producer) == FVM_OK);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&sent, 17);

    /* Output / Вывод и передача результата. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_send(&producer, &sent) == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_channel_value_receive(&consumer, &received) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(received.type == FVM_VALUE_NUMBER && received.number == 17);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем одну удержанную ссылку на канал. */
    fvm_channel_value_release(&consumer);
    /* Освобождаем одну удержанную ссылку на канал. */
    fvm_channel_value_release(&producer);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
