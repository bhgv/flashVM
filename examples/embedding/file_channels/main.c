/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/file_channels/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/transfer.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/table.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
/* The receiver can be a new VM or a later process opening the same file. */
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Config config;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Runtime *runtime;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Context *writer, *reader;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Channel *file;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Object *object;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value, restored, field;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Создаём среду выполнения; её необходимо освободить после контекстов. Проверяем успешность
     * вызова. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    /* Создаём отдельный контекст интерпретатора в среде выполнения. Проверяем успешность вызова. */
    assert(fvm_context_create(runtime, &writer) == FVM_OK);
    /* Создаём управляемый объект в указанном контексте. Проверяем успешность вызова. */
    assert(fvm_object_create(writer, &object) == FVM_OK);
    /* Создаём строковое представление значения по правилам API. Проверяем успешность вызова. */
    assert(fvm_value_string(&value, "Hello from FlashVM") == FVM_OK);
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(writer, object, "message", &value) == FVM_OK);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&value);

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: value.type. */
    value.type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: value.object. */
    value.object = object;
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(writer, object, "self", &value) == FVM_OK);
    /* Открываем файловый канал в указанном режиме доступа. Проверяем успешность вызова. */
    assert(fvm_channel_open_file("flashvm-example.bin", "w", &file) == FVM_OK);

    /* Output / Вывод и передача результата. */
    /* Передаём граф значения через канал с учётом режима ожидания. Проверяем успешность вызова. */
    assert(fvm_channel_send_value(writer, file, &value, 1) == FVM_OK);

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем принадлежащую примеру ссылку на канал. */
    fvm_channel_destroy(file);
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(writer);
    /* Создаём отдельный контекст интерпретатора в среде выполнения. Проверяем успешность вызова. */
    assert(fvm_context_create(runtime, &reader) == FVM_OK);

    /* Computation / Вычисление и обновление состояния. */
    /* Открываем файловый канал в указанном режиме доступа. Проверяем успешность вызова. */
    assert(fvm_channel_open_file("flashvm-example.bin", "r", &file) == FVM_OK);
    /* Восстанавливаем полученный граф значения в контексте получателя. Проверяем успешность вызова.
     */
    assert(fvm_channel_receive_value(reader, file, &restored, 1) == FVM_OK);
    /* Читаем свойство объекта, включая поиск по прототипу. Проверяем успешность вызова. */
    assert(fvm_object_get(reader, restored.object, "self", &field) == FVM_OK &&
           /* Завершаем выражение с указанным значением или аргументами. */
           field.object == restored.object);
    /* Читаем свойство объекта, включая поиск по прототипу. Проверяем успешность вызова. */
    assert(fvm_object_get(reader, restored.object, "message", &field) == FVM_OK);
    /* Сравниваем содержимое строк, а не адреса их буферов. Проверяем успешность вызова. */
    assert(!strcmp(field.string, "Hello from FlashVM"));

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем принадлежащую примеру ссылку на канал. */
    fvm_channel_destroy(file);
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(reader);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Удаляем созданный примером временный файл. Проверяем успешность вызова. */
    assert(!remove("flashvm-example.bin"));
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
