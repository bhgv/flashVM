/* C89: передача числа через FIFO без создания контекста VM.
 * C89: transfer a number through a FIFO without creating a VM context. */
#include "flashvm/channel.h"
#include <stdio.h>

int main(void)
{
    FVM_Channel *channel = 0;
    FVM_Value sent, received;
    int exit_code = 0;

    /* Канал принадлежит этому примеру; после создания его нужно освободить.
     * This example owns the channel and must release it after creation. */
    if (fvm_channel_create(&channel) != FVM_OK)
        return 1;

    /* Инициализатор задаёт тип и очищает остальные поля значения.
     * The initializer sets the type and clears the remaining value fields. */
    fvm_value_number(&sent, 123.0);

    /* send копирует число в очередь; receive извлекает самое старое сообщение.
     * send copies the number; receive removes the oldest queued message. */
    if (fvm_channel_send(channel, &sent) != FVM_OK)
    {
        exit_code = 2;
        goto cleanup;
    }
    if (fvm_channel_receive(channel, &received) != FVM_OK)
    {
        exit_code = 3;
        goto cleanup;
    }
    printf("Received: %.0f\n", received.number);

cleanup:
    /* Закрытие прекращает обмен; destroy освобождает принадлежащую нам ссылку.
     * Closing stops exchange; destroy releases the reference owned here.
     * Оба действия выполняются и при ошибке отправки или получения.
     * Both actions also run after a send or receive failure. */
    fvm_channel_close(channel);
    fvm_channel_destroy(channel);
    return exit_code;
}
