/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/standard_text/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/standard_text.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_data.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../example_support.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(int argc, char **argv)
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
    int automatic = 0; /* Optional executable demonstration uses the same owner context. */
    if (argc == 2 && !strcmp(argv[1], "--auto-size")) automatic = 1; /* Select measured left-anchored field dimensions. */
    else if (argc != 1) { fputs("usage: standard_text [--auto-size]\n", stderr); return 1; } /* Reject unknown options before allocation. */
    /* Вызываем fvm_config_init: заполняем настройки VM значениями по умолчанию.
     * &config — настройки VM.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_config_init(&config) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_config_init(&config) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_runtime_create: создаём среду выполнения.
     * &config — настройки VM.
     * &runtime — адрес для новой среды выполнения; позже вызываем runtime_destroy.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_runtime_create(&config, &runtime) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_context_create: создаём независимый контекст интерпретатора.
     * runtime — среда выполнения.
     * &context — адрес для дескриптора создаваемого контекста; позже вызываем context_destroy.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_context_create(runtime, &context) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_standard_text_install: устанавливаем стандартные текстовые объекты.
     * context — контекст интерпретатора.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_standard_text_install(context) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_standard_text_install(context) == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
/* Выбираем ветку компиляции для указанной конфигурации. */
#ifdef FVM_HAS_FONTS
    /* Вызываем fvm_standard_text_register_font: регистрируем шрифт для стандартных текстовых
     * объектов.
     * context — контекст интерпретатора. "Geometry" — имя функции, объекта или свойства.
     * fvm_test_font — байты входного ресурса.
     * sizeof(fvm_test_font) — размер входных данных или буфера.
     * 0 — индекс выбираемого элемента.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_standard_text_register_font(context, "Geometry", fvm_test_font,

                                          /* Продолжаем условие проверки или аргументы диагностического сообщения. */
                                          sizeof(fvm_test_font), 0) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_standard_text_register_font(context, \"Geometry\", fvm_test_font, "
                         /* Продолжаем условие проверки или аргументы диагностического сообщения. */
                         "sizeof(fvm_test_font), 0) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_construct: создаём экземпляр через зарегистрированный конструктор.
     * context — контекст интерпретатора.
     * "TextFormat" — имя функции, объекта или свойства.
     * 0 — число передаваемых аргументов.
     * 0 — массив аргументов вызова.
     * &format — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_construct(context, "TextFormat", 0, 0, &format) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_construct(context, \"TextFormat\", 0, 0, &format) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&value, 40);
    /* Вызываем fvm_object_set: записываем именованное свойство объекта.
     * context — контекст-владелец объекта.
     * format — объект-получатель свойства.
     * "size" — имя свойства.
     * &value — значение свойства, сохраняемое в графе объекта.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_set(context, format, "size", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_set(context, format, \"size\", &value) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_textfield_create: создаём объект TextField.
     * context — контекст интерпретатора.
     * "Geometry" — имя ранее зарегистрированного шрифта.
     * 61 — ширина границы или текстового блока в пикселях.
     * 140 — высота текстового блока в пикселях.
     * &field — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_textfield_create(context, "Geometry", 61, 140, &field) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_textfield_create(context, \"Geometry\", 61, 140, &field) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&value);
    /* Сохраняем вычисленное значение: value.type. */
    value.type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: value.object. */
    value.object = field;
    /* Вызываем fvm_set_global: публикуем значение в глобальной области.
     * context — контекст интерпретатора.
     * "caption" — имя функции, объекта или свойства.
     * &value — передаваемое значение.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_set_global(context, "caption", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_set_global(context, \"caption\", &value) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Записываем логическое значение и соответствующий тег типа. */
    fvm_value_boolean(&value, 1);
    /* Вызываем fvm_object_set: записываем именованное свойство объекта.
     * context — контекст-владелец объекта.
     * field — объект-получатель свойства.
     * "multiline" — имя свойства.
     * &value — значение свойства, сохраняемое в графе объекта.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_set(context, field, "multiline", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_set(context, field, \"multiline\", &value) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_set: записываем именованное свойство объекта.
     * context — контекст-владелец объекта.
     * field — объект-получатель свойства.
     * "wordWrap" — имя свойства.
     * &value — значение свойства, сохраняемое в графе объекта.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_set(context, field, "wordWrap", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_set(context, field, \"wordWrap\", &value) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Сбрасываем все поля значения в состояние undefined. */
    fvm_value_undefined(&args[0]);
    /* Сохраняем вычисленное значение: args[0].type. */
    args[0].type = FVM_VALUE_OBJECT;
    /* Сохраняем вычисленное значение: args[0].object. */
    args[0].object = format;
    /* Вызываем fvm_object_call: вызываем именованный метод с указанным получателем.
     * context — контекст интерпретатора.
     * field — управляемый объект.
     * "setNewTextFormat" — имя функции, объекта или свойства.
     * 1 — число передаваемых аргументов.
     * args — массив аргументов вызова.
     * &result — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_call(context, field, "setNewTextFormat", 1, args, &result) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail(
            /* Продолжаем условие проверки или аргументы диагностического сообщения. */
            "fvm_object_call(context, field, \"setNewTextFormat\", 1, args, &result) == FVM_OK",
            /* Передаём имя исходного файла и номер строки неуспешной проверки. */
            __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_value_string: создаём строковое значение по правилам владения API.
     * &value — передаваемое значение.
     * "AV fi" — строка или используемый дескриптор.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_value_string(&value, "AV fi") == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_value_string(&value, \"AV fi\") == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_set: записываем именованное свойство объекта.
     * context — контекст-владелец объекта.
     * field — объект-получатель свойства.
     * "text" — имя свойства.
     * &value — значение свойства, сохраняемое в графе объекта.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_set(context, field, "text", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_set(context, field, \"text\", &value) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_get: читаем свойство с учётом прототипа и аксессоров.
     * context — контекст-владелец объекта.
     * field — объект, в котором ищем свойство.
     * "textHeight" — имя свойства.
     * &result — адрес для найденного значения.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_get(context, field, "textHeight", &result) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_get(context, field, \"textHeight\", &result) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Выводим результат в стандартный поток вывода. */
    printf("caption.textHeight = %.2f pixels\n", result.number);
    /* Replace [0,2), retaining the current uniform style and invalidating layout. */
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 0);
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[1], 2);
    /* Вызываем fvm_value_string: создаём строковое значение по правилам владения API.
     * &args[2] — передаваемое значение.
     * "B" — строка или используемый дескриптор.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_value_string(&args[2], "B") == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_value_string(&args[2], \"B\") == FVM_OK", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_call: вызываем именованный метод с указанным получателем.
     * context — контекст интерпретатора.
     * field — управляемый объект.
     * "replaceText" — имя функции, объекта или свойства.
     * 3 — число передаваемых аргументов.
     * args — массив аргументов вызова.
     * &result — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_call(context, field, "replaceText", 3, args, &result) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail(
            /* Продолжаем условие проверки или аргументы диагностического сообщения. */
            "fvm_object_call(context, field, \"replaceText\", 3, args, &result) == FVM_OK",
            /* Передаём имя исходного файла и номер строки неуспешной проверки. */
            __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_get: читаем свойство с учётом прототипа и аксессоров.
     * context — контекст-владелец объекта.
     * field — объект, в котором ищем свойство.
     * "text" — имя свойства.
     * &result — адрес для найденного значения.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_get(context, field, "text", &result) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_get(context, field, \"text\", &result) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Выводим результат в стандартный поток вывода. */
    printf("caption.text = %s\n", result.string);
    /* Проверяем результат предыдущей операции: strcmp(result.string, "B fi") == 0.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(strcmp(result.string, "B fi") == 0))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("strcmp(result.string, \"B fi\") == 0", __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Записываем числовое значение и соответствующий тег типа. */
    fvm_value_number(&args[0], 0);
    /* Вызываем fvm_object_call: вызываем именованный метод с указанным получателем.
     * context — контекст интерпретатора.
     * field — управляемый объект.
     * "getTextFormat" — имя функции, объекта или свойства.
     * 1 — число передаваемых аргументов.
     * args — массив аргументов вызова.
     * &result — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_call(context, field, "getTextFormat", 1, args, &result) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail(
            /* Продолжаем условие проверки или аргументы диагностического сообщения. */
            "fvm_object_call(context, field, \"getTextFormat\", 1, args, &result) == FVM_OK",
            /* Передаём имя исходного файла и номер строки неуспешной проверки. */
            __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Вызываем fvm_object_get: читаем свойство с учётом прототипа и аксессоров.
     * context — контекст-владелец объекта.
     * result.object — объект, в котором ищем свойство.
     * "size" — имя свойства.
     * &value — адрес для найденного значения.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_object_get(context, result.object, "size", &value) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_object_get(context, result.object, \"size\", &value) == FVM_OK",
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __FILE__, __LINE__);
        /* Завершаем ветку ошибки. */
    }
    /* Проверяем результат предыдущей операции: value.type == FVM_VALUE_NUMBER && value.number
     * == 40. Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(value.type == FVM_VALUE_NUMBER && value.number == 40))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("value.type == FVM_VALUE_NUMBER && value.number == 40", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
    if (automatic) /* Demonstrate measured geometry after the ordinary field tutorial. */
    { /* Keep C89 declarations before statements in this optional scope. */
        double width, height; /* Copy numeric observations before reusing the result value. */
        fvm_value_boolean(&value, 0); /* Disable wrapping so the first edit can fit both dimensions. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("disable wrapping", __FILE__, __LINE__); /* Check the native policy write. */
        if (fvm_value_string(&value, "AV") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("initial auto text", __FILE__, __LINE__); /* Apply the existing 40px future format. */
        fvm_value_boolean(&value, 1); /* True selects the left anchor. */
        if (fvm_object_set(context, field, "autoSize", &value) != FVM_OK) fvm_example_fail("enable autoSize", __FILE__, __LINE__); /* Measurement commits only on success. */
        if (fvm_object_get(context, field, "_width", &result) != FVM_OK) fvm_example_fail("read width", __FILE__, __LINE__); /* Borrow the field on its owner thread. */
        width = result.number; /* The analytical AV width is 48 plus four pixels of gutter. */
        if (fvm_object_get(context, field, "_height", &result) != FVM_OK) fvm_example_fail("read height", __FILE__, __LINE__); /* Copy the measured outer height. */
        height = result.number; /* The fixture ascent/descent is 44 plus the same gutter. */
        printf("autoSize left: width=%.2f height=%.2f\n", width, height); /* Expected 52.00 by 48.00. */
        if (fvm_value_string(&value, "AVA") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("grow auto text", __FILE__, __LINE__); /* A genuine edit reapplies the future style. */
        if (fvm_object_get(context, field, "_width", &result) != FVM_OK) fvm_example_fail("read edited width", __FILE__, __LINE__); /* Observe the new width without a layout call. */
        printf("autoSize edit: width=%.2f\n", result.number); /* Expected 76.00 including gutter. */
        fvm_value_boolean(&value, 1); /* Keep the current width as the wrapping constraint. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("enable wrapping", __FILE__, __LINE__); /* Existing multiline remains enabled. */
        fvm_value_number(&value, 30); /* A 26px inner width holds one 24px character. */
        if (fvm_object_set(context, field, "_width", &value) != FVM_OK) fvm_example_fail("set wrap width", __FILE__, __LINE__); /* Fit three rows before committing width. */
        if (fvm_object_get(context, field, "_height", &result) != FVM_OK) fvm_example_fail("read wrapped height", __FILE__, __LINE__); /* Three baseline rows fit to 140px with gutter. */
        printf("autoSize wrap: width=30.00 height=%.2f\n", result.number); /* The executable checker verifies 140.00. */
    } /* The field owns its measured cache; the following path export remains independent. */
    /* Вызываем fvm_textfield_path: получаем контуры текста поля.
     * context — контекст интерпретатора.
     * field — объект, с которым выполняется операция.
     * &outline — адрес или буфер для результата.
     * Если условие ниже нарушено, печатаем место ошибки и завершаем учебный процесс. */
    if (!(fvm_textfield_path(context, field, &outline) == FVM_OK))
    /* Начинаем обработку неуспешного вызова или неверного результата. */
    {
        /* Выводим исходное условие, имя файла и номер строки; этот вызов не возвращается. */
        fvm_example_fail("fvm_textfield_path(context, field, &outline) == FVM_OK", __FILE__,
                         /* Передаём имя исходного файла и номер строки неуспешной проверки. */
                         __LINE__);
        /* Завершаем ветку ошибки. */
    }
/* Начинаем альтернативную ветку компиляции. */
#else
    /* Завершаем выражение с указанным значением или аргументами. */
    (void)automatic; /* The option remains parseable in a NO_FONTS profile. */
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
    /* Вызываем fvm_path_destroy: освобождаем векторный путь.
     * outline — векторный путь. */
    fvm_path_destroy(outline);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
