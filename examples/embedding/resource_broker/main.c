/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/resource_broker/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/capability.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/mutex.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/transfer.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* A file descriptor reconnects to the living mutex; it never copies lock state. */
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{
/* Выбираем ветку компиляции для указанной конфигурации. */
#ifdef OS_PORTS
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config config;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Runtime *runtime;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Context *owner, *recipient;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_ResourceBroker *broker;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Mutex *mutex;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Channel *file;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Object *object;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value, restored, result;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    int held;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Создаём среду выполнения; её необходимо освободить после контекстов. Проверяем успешность
     * вызова. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    /* Создаём отдельный контекст интерпретатора в среде выполнения. Проверяем успешность вызова. */
    assert(fvm_context_create(runtime, &owner) == FVM_OK);
    /* Создаём отдельный контекст интерпретатора в среде выполнения. Проверяем успешность вызова. */
    assert(fvm_context_create(runtime, &recipient) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_context_set_broker(owner, broker) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_mutex_create(owner, &mutex) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_extensions_wrap_mutex(owner, mutex, &object) == FVM_OK);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&value);
    /* Сохраняем вычисленное значение: value.type. */
    value.type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: value.object. */
    value.object = object;
    /* Открываем файловый канал в указанном режиме доступа. Проверяем успешность вызова. */
    assert(fvm_channel_open_file("resource-example.bin", "w", &file) == FVM_OK);
    /* Передаём граф значения через канал с учётом режима ожидания. Проверяем успешность вызова. */
    assert(fvm_channel_send_value(owner, file, &value, 0) == FVM_OK);
    /* Выполняем вызов fvm_channel_release с указанными аргументами. */
    fvm_channel_release(file);
    /* Открываем файловый канал в указанном режиме доступа. Проверяем успешность вызова. */
    assert(fvm_channel_open_file("resource-example.bin", "r", &file) == FVM_OK);
    /* Восстанавливаем полученный граф значения в контексте получателя. Проверяем успешность вызова.
     */
    assert(fvm_channel_receive_value(recipient, file, &restored, 0) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_object_call(recipient, restored.object, "set", 0, 0, &result) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_mutex_touch(mutex, &held) == FVM_OK && held);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_object_call(recipient, restored.object, "reset", 0, 0, &result) == FVM_OK);
    /* Выполняем вызов fvm_channel_release с указанными аргументами. */
    fvm_channel_release(file);
    /* Удаляем созданный примером временный файл. */
    remove("resource-example.bin");
    /* Выполняем вызов fvm_mutex_release с указанными аргументами. */
    fvm_mutex_release(mutex);
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(recipient);
    /* Выполняем вызов fvm_broker_stop с указанными аргументами. */
    fvm_broker_stop(broker);
    /* Выполняем вызов fvm_broker_release с указанными аргументами. */
    fvm_broker_release(broker);
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(owner);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Выводим строку с завершающим переводом строки. */
    puts("File restored a shared TCP mutex capability");
/* Завершаем условный блок компиляции. */
#endif
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
