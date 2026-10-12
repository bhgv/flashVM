/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/standard_text/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/standard_text.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_data.h"
/* Подключаем объявления используемых типов и функций. */
#include <assert.h>
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
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
    /* Сохраняем вычисленное значение: FVM_Object *format. */
    FVM_Object *format = 0, *field = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Value value, args[3], result;
    /* Сохраняем вычисленное значение: FVM_Path *outline. */
    FVM_Path *outline = 0;
    /* Инициализируем настройки безопасными значениями по умолчанию. Проверяем успешность вызова. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Создаём среду выполнения; её необходимо освободить после контекстов. Проверяем успешность
     * вызова. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    /* Создаём отдельный контекст интерпретатора в среде выполнения. Проверяем успешность вызова. */
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_standard_text_install(context) == FVM_OK);
/* Выбираем ветку компиляции для указанной конфигурации. */
#ifdef FVM_HAS_FONTS
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_standard_text_register_font(context, "Geometry", fvm_test_font,
                                           /* Выполняем вызов sizeof с указанными аргументами. */
                                           sizeof(fvm_test_font), 0) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_construct(context, "TextFormat", 0, 0, &format) == FVM_OK);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&value, 40);
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(context, format, "size", &value) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_textfield_create(context, "Geometry", 61, 140, &field) == FVM_OK);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&value);
    /* Сохраняем вычисленное значение: value.type. */
    value.type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: value.object. */
    value.object = field;
    /* Сохраняем значение в глобальной области контекста. Проверяем успешность вызова. */
    assert(fvm_set_global(context, "caption", &value) == FVM_OK);
    /* Записываем логическое значение и соответствующий тег типа. */
    fvm_value_boolean(&value, 1);
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(context, field, "multiline", &value) == FVM_OK);
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(context, field, "wordWrap", &value) == FVM_OK);
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&args[0]);
    /* Сохраняем вычисленное значение: args[0].type. */
    args[0].type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: args[0].object. */
    args[0].object = format;
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_object_call(context, field, "setNewTextFormat", 1, args, &result) == FVM_OK);
    /* Создаём строковое представление значения по правилам API. Проверяем успешность вызова. */
    assert(fvm_value_string(&value, "AV fi") == FVM_OK);
    /* Записываем свойство объекта; значение сохраняется в его графе. Проверяем успешность вызова.
     */
    assert(fvm_object_set(context, field, "text", &value) == FVM_OK);
    /* Читаем свойство объекта, включая поиск по прототипу. Проверяем успешность вызова. */
    assert(fvm_object_get(context, field, "textHeight", &result) == FVM_OK);
    /* Выводим результат в стандартный поток вывода. */
    printf("caption.textHeight = %.2f pixels\n", result.number);
    /* Replace [0,2), retaining the current uniform style and invalidating layout. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 0);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[1], 2);
    /* Создаём строковое представление значения по правилам API. Проверяем успешность вызова. */
    assert(fvm_value_string(&args[2], "B") == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_object_call(context, field, "replaceText", 3, args, &result) == FVM_OK);
    /* Читаем свойство объекта, включая поиск по прототипу. Проверяем успешность вызова. */
    assert(fvm_object_get(context, field, "text", &result) == FVM_OK);
    /* Выводим результат в стандартный поток вывода. */
    printf("caption.text = %s\n", result.string);
    /* Сравниваем содержимое строк, а не адреса их буферов. Проверяем успешность вызова. */
    assert(strcmp(result.string, "B fi") == 0);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 0);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_object_call(context, field, "getTextFormat", 1, args, &result) == FVM_OK);
    /* Читаем свойство объекта, включая поиск по прототипу. Проверяем успешность вызова. */
    assert(fvm_object_get(context, result.object, "size", &value) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(value.type == FVM_VALUE_NUMBER && value.number == 40);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_textfield_path(context, field, &outline) == FVM_OK);
/* Начинаем альтернативную ветку компиляции. */
#else
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)format;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)field;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)value;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)args;
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)result;
    /* Выводим строку с завершающим переводом строки. */
    puts("TextFormat objects are available; font measurement requires FONTS.");
/* Завершаем условный блок компиляции. */
#endif

    /* Cleanup / Освобождение ресурсов. */
    /* Освобождаем контекст и принадлежащие ему объекты. */
    fvm_context_destroy(context);
    /* Освобождаем среду выполнения после завершения работы контекстов. */
    fvm_runtime_destroy(runtime);
    /* Outlines are owned independently and can be edited/drawn/masked here. */
    /* Выполняем вызов fvm_path_destroy с указанными аргументами. */
    fvm_path_destroy(outline);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
