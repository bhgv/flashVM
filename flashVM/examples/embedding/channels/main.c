/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/channels/main.c.
 */

/* C89: передача числа через FIFO без создания контекста VM.
 * C89: transfer a number through a FIFO without creating a VM context. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/channel.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: FVM_Channel *channel. */
    FVM_Channel *channel = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value sent, received;
    /* Сохраняем вычисленное значение: int exit_code. */
    int exit_code = 0;

    /* Канал принадлежит этому примеру; после создания его нужно освободить.
     * This example owns the channel and must release it after creation. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_channel_create(&channel) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;

    /* Инициализатор задаёт тип и очищает остальные поля значения.
     * The initializer sets the type and clears the remaining value fields. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&sent, 123.0);

    /* send копирует число в очередь; receive извлекает самое старое сообщение.
     * send copies the number; receive removes the oldest queued message. */
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_channel_send(channel, &sent) != FVM_OK)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Сохраняем вычисленное значение: exit_code. */
        exit_code = 2;
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto cleanup;
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_channel_receive(channel, &received) != FVM_OK)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Сохраняем вычисленное значение: exit_code. */
        exit_code = 3;
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto cleanup;
        /* Завершаем текущий блок или описание объекта. */
    }

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("Received: %.0f\n", received.number);

/* Точка перехода для общей ветки обработки и очистки. */
cleanup:
    /* Закрытие прекращает обмен; destroy освобождает принадлежащую нам ссылку.
     * Closing stops exchange; destroy releases the reference owned here.
     * Оба действия выполняются и при ошибке отправки или получения.
     * Both actions also run after a send or receive failure. */
    /* Закрываем канал и пробуждаем ожидающих участников. */
    fvm_channel_close(channel);
    /* Освобождаем принадлежащую примеру ссылку на канал. */
    fvm_channel_destroy(channel);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return exit_code;
    /* Завершаем текущий блок или описание объекта. */
}
