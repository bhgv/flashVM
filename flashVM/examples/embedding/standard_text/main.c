/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/standard_text/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/standard_text.h"
#include "flashvm/standard.h" /* The tab Array mode explicitly installs ordinary builtins. */
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_data.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_bold_data.h" /* Original embedded bold face; no runtime synthesis. */
#include "../../../tests/fixtures/font_italic_data.h" /* Independent italic outline and advance data. */
#include "../../../tests/fixtures/font_bold_italic_data.h" /* Separate combined face, selected by both bits. */
#include "../../example_support.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Подключаем объявления используемых типов и функций. */
#include <string.h>
#include "scroll.inc" /* Add an annotated owner-thread viewport and contour demonstration. */
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
    int scrolling = 0; /* Optional explicit visual-line and pixel scrolling demonstration. */
    int tabbed = 0; /* Optional native Array tabStops demonstration. */
    int insets = 0; /* Optional paragraph inset demonstration on the same owner thread. */
    int variants = 0; /* Optional real-face tutorial shares the existing owner context. */
    int automatic = 0; /* Optional executable demonstration uses the same owner context. */
    if (argc == 2 && !strcmp(argv[1], "--auto-size")) automatic = 1; /* Select measured left-anchored field dimensions. */
    else if (argc == 2 && !strcmp(argv[1], "--anchors")) automatic = 2; /* Also demonstrate parent-space horizontal anchors. */
    else if (argc == 2 && !strcmp(argv[1], "--font-variants")) variants = 1; /* Select actual regular/bold/italic/bold-italic sources. */
    else if (argc == 2 && !strcmp(argv[1], "--insets")) insets = 1; /* Measure wrapped and fitted paragraph insets. */
    else if (argc == 2 && !strcmp(argv[1], "--indent")) insets = 2; /* Combine first-line indentation with paragraph margins. */
    else if (argc == 2 && !strcmp(argv[1], "--tabs")) tabbed = 1; /* Apply an explicit Array of tab positions. */
    else if (argc == 2 && !strcmp(argv[1], "--default-tabs")) tabbed = 2; /* Keep tabStops empty so the font determines the repeated interval. */
    else if (argc == 2 && !strcmp(argv[1], "--scroll")) scrolling = 1; /* Demonstrate measured viewport offsets and contours. */
    else if (argc != 1) { fputs("usage: standard_text [--auto-size|--anchors|--font-variants|--insets|--indent|--tabs|--default-tabs|--scroll]\n", stderr); return 1; } /* Reject unknown options before allocation. */
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
    if (tabbed && fvm_standard_install(context) != FVM_OK) fvm_example_fail("install Array builtins", __FILE__, __LINE__); /* Install Array only for the optional managed-table mode. */
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
    if (variants) /* Copy distinct real faces before any format selects their slots. */
    { /* Every source array is borrowed only during its registration call. */
        if (fvm_standard_text_register_font_variant(context, "Geometry", FVM_TEXT_FONT_BOLD, fvm_test_font_bold, sizeof(fvm_test_font_bold), 0) != FVM_OK) fvm_example_fail("register bold", __FILE__, __LINE__); /* Bind the bold slot; the context owns a copy. */
        if (fvm_standard_text_register_font_variant(context, "Geometry", FVM_TEXT_FONT_ITALIC, fvm_test_font_italic, sizeof(fvm_test_font_italic), 0) != FVM_OK) fvm_example_fail("register italic", __FILE__, __LINE__); /* Bind the independent italic slot. */
        if (fvm_standard_text_register_font_variant(context, "Geometry", FVM_TEXT_FONT_VARIANTS, fvm_test_font_bold_italic, sizeof(fvm_test_font_bold_italic), 0) != FVM_OK) fvm_example_fail("register bold italic", __FILE__, __LINE__); /* Both style bits choose the combined face. */
    } /* All copies are released by context destruction. */
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
    if (automatic == 2) /* Keep exported geometry local while inspecting placement separately. */
    { /* Each iteration starts with the same 100-pixel parent-space box. */
        unsigned int anchor; /* C89 declarations precede statements in this scope. */
        const char *modes[] = {"left", "center", "right"}; /* These modes preserve left edge, center or right edge. */
        for (anchor = 0; anchor < 3; ++anchor) /* Compare all native fitting policies. */
        { /* No VM objects or font data cross context/thread ownership. */
            double x; /* Copy a scalar before the next borrowed value overwrites result. */
            fvm_value_boolean(&value, 0); /* Disable fitting before resetting manual dimensions. */
            if (fvm_object_set(context, field, "autoSize", &value) != FVM_OK || fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("disable fitting/wrap", __FILE__, __LINE__); /* Preserve ordinary error handling. */
            fvm_value_number(&value, 100); /* Start with a 100-pixel outer width. */
            if (fvm_object_set(context, field, "_width", &value) != FVM_OK) fvm_example_fail("reset width", __FILE__, __LINE__); /* Manual width is now writable. */
            fvm_value_number(&value, 10); /* Locate the initial left edge at parent x=10. */
            if (fvm_object_set(context, field, "_x", &value) != FVM_OK) fvm_example_fail("reset position", __FILE__, __LINE__); /* This does not translate the exported local path. */
            if (fvm_value_string(&value, "AV") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("reset text", __FILE__, __LINE__); /* The future format remains 40 pixels. */
            if (fvm_value_string(&value, modes[anchor]) != FVM_OK || fvm_object_set(context, field, "autoSize", &value) != FVM_OK) fvm_example_fail("select anchor", __FILE__, __LINE__); /* Activation fits the current box transactionally. */
            if (fvm_object_get(context, field, "_x", &result) != FVM_OK) fvm_example_fail("read position", __FILE__, __LINE__); /* Read through the same accessors used by AVM1. */
            x = result.number; /* Keep the position while querying width into result. */
            if (fvm_object_get(context, field, "_width", &result) != FVM_OK) fvm_example_fail("read anchored width", __FILE__, __LINE__); /* Every mode measures 48 pixels plus gutter. */
            printf("anchor %s: x=%.2f width=%.2f\n", modes[anchor], x, result.number); /* Expect x=10,34,58 and width=52. */
        } /* The final field retains right anchoring; no path is borrowed yet. */
    } /* The existing path export below owns an independent field-local copy. */
    if (variants) /* Compare the four real source advances at the same 40-pixel size. */
    { /* C89 declarations precede statements in this optional scope. */
        unsigned int face; /* Iterate the public bit combinations in slot order. */
        const char *names[] = {"regular", "bold", "italic", "bold-italic"}; /* Human-readable observation labels. */
        fvm_value_boolean(&value, 0); /* Measure a single unwrapped line. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("disable face wrapping", __FILE__, __LINE__); /* Keep geometry comparisons independent of line breaking. */
        if (fvm_value_string(&value, "AV") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("face probe text", __FILE__, __LINE__); /* Apply the existing future regular format. */
        fvm_value_undefined(&args[0]); /* Replace earlier editing indices with a borrowed format argument. */
        args[0].type = FVM_VALUE_OBJECT; /* Declare the VM object representation. */
        args[0].object = format; /* The existing global keeps this format reachable. */
        for (face = 0; face <= FVM_TEXT_FONT_VARIANTS; ++face) /* Select each actual supplied face. */
        { /* Boolean fields independently control the two style bits. */
            fvm_value_boolean(&value, (face & FVM_TEXT_FONT_BOLD) != 0); /* Choose the bold component. */
            if (fvm_object_set(context, format, "bold", &value) != FVM_OK) fvm_example_fail("set bold", __FILE__, __LINE__); /* Store the requested style without touching the current field yet. */
            fvm_value_boolean(&value, (face & FVM_TEXT_FONT_ITALIC) != 0); /* Choose the italic component independently. */
            if (fvm_object_set(context, format, "italic", &value) != FVM_OK) fvm_example_fail("set italic", __FILE__, __LINE__); /* Both true select the registered combined face. */
            if (fvm_object_call(context, field, "setTextFormat", 1, args, &result) != FVM_OK) fvm_example_fail("apply real face", __FILE__, __LINE__); /* Validate source and layout before publication. */
            if (fvm_object_get(context, field, "textWidth", &result) != FVM_OK) fvm_example_fail("measure real face", __FILE__, __LINE__); /* Read logical advance rather than ink bounds. */
            printf("face %s: advance=%.2f\n", names[face], result.number); /* Expect 48,56,52,60 for the four embedded faces. */
        } /* The final combined face remains selected for owned path export. */
    } /* No source or path handle escapes the documented ownership rules. */
    if (insets) /* Demonstrate uniform margins without changing font or ownership. */
    { /* Keep declarations first for C89 callers. */
        const char *names[] = {"leftMargin", "rightMargin", "blockIndent"}; /* Select whole-paragraph properties. */
        const double sizes[] = {10, 12, 8}; /* Left origin is 18 and total reserved width is 30. */
        unsigned int i; /* Traverse the three independently stored properties. */
        double width, height; /* Copy observations before result is reused. */
        for (i = 0; i < 3; ++i) /* Store values in the managed TextFormat. */
        { /* Applying the format below validates every value before publication. */
            fvm_value_number(&value, sizes[i]); /* Borrow a stack numeric value. */
            if (fvm_object_set(context, format, names[i], &value) != FVM_OK) fvm_example_fail("store inset", __FILE__, __LINE__); /* Same-context property write may allocate. */
        } /* No native resource is transferred by these writes. */
        if (insets == 2) /* Apply an 18px offset only to nonempty paragraph first lines. */
        { /* Format mutation precedes future-style application and text assignment. */
            fvm_value_number(&value, 18); /* TextFormat accepts bounded integral offsets. */
            if (fvm_object_set(context, format, "indent", &value) != FVM_OK) fvm_example_fail("store first-line indent", __FILE__, __LINE__); /* Borrow the managed same-context format. */
        } /* Existing zero-indent mode remains unchanged. */
        fvm_value_undefined(args); /* Prepare the one borrowed method argument. */
        args[0].type = FVM_VALUE_OBJECT; /* Select the object representation. */
        args[0].object = format; /* Existing global roots the shared format. */
        if (fvm_object_call(context, field, "setNewTextFormat", 1, args, &result) != FVM_OK) fvm_example_fail("future insets", __FILE__, __LINE__); /* Later full assignment selects this style. */
        fvm_value_boolean(&value, 1); /* Permit multiline wrapping. */
        if (fvm_object_set(context, field, "multiline", &value) != FVM_OK || fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("inset wrapping", __FILE__, __LINE__); /* Both policies are required for wrapping. */
        fvm_value_number(&value, 124); /* Reserve 4px gutter plus 30px insets, leaving 90px content. */
        if (fvm_object_set(context, field, "_width", &value) != FVM_OK) fvm_example_fail("inset outer width", __FILE__, __LINE__); /* Preserve the manual outer width. */
        if (fvm_value_string(&value, "A V A V") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("inset text", __FILE__, __LINE__); /* Copy counted input and apply the future style. */
        fvm_value_boolean(&value, 1); /* Enable the left automatic anchor. */
        if (fvm_object_set(context, field, "autoSize", &value) != FVM_OK) fvm_example_fail("inset fitting", __FILE__, __LINE__); /* Wrapped fitting changes height only. */
        if (fvm_object_get(context, field, "textWidth", &result) != FVM_OK) fvm_example_fail("inset advance", __FILE__, __LINE__); /* Read glyph-only width. */
        width = result.number; /* Natural first-line advance is 72px; first-line mode adds 18px to its intrinsic extent. */
        if (fvm_object_get(context, field, "_height", &result) != FVM_OK) fvm_example_fail("inset height", __FILE__, __LINE__); /* Read two-line outer height. */
        height = result.number; /* Ascent/descent and leading produce 94px with gutter. */
        printf("%s wrap: advance=%.2f height=%.2f\n", insets == 2 ? "indent" : "insets", width, height); /* Expected 90 or 72 advance, with 94 height. */
        fvm_value_boolean(&value, 0); /* Disable wrapping for horizontal fitting. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK || fvm_object_get(context, field, "_width", &result) != FVM_OK) fvm_example_fail("inset fit width", __FILE__, __LINE__); /* Fit includes 132px text, 30px insets, 4px gutter and the optional 18px first-line offset. */
        printf("%s fit: width=%.2f\n", insets == 2 ? "indent" : "insets", result.number); /* Expected 184 or 166; exported outlines remain field-local. */
    } /* The ordinary path export and context cleanup below retain their ownership. */
    if (tabbed) /* Copy managed Array values into the field's future native style. */
    { /* Declare every local before statements for C89. */
        FVM_Object *array = 0; /* Managed Array stays in this context and receives a global root. */
        double advance, width, height; /* Copy numeric observations before reusing result. */
        if (fvm_construct(context, "Array", 0, 0, &array) != FVM_OK) fvm_example_fail("construct tab Array", __FILE__, __LINE__); /* Start empty to avoid the one-number length constructor. */
        fvm_value_undefined(&value); /* Prepare a borrowed object value. */
        value.type = FVM_VALUE_OBJECT; /* Use the public object representation. */
        value.object = array; /* The context owns the Array lifetime. */
        if (fvm_set_global(context, "tabs", &value) != FVM_OK || fvm_object_set(context, format, "tabStops", &value) != FVM_OK) fvm_example_fail("root and store tab Array", __FILE__, __LINE__); /* Root source storage before later calls. */
        if (tabbed == 1) /* An empty default-mode Array intentionally has no positions. */
        { /* Explicit mode alone populates the borrowed source table. */
        fvm_value_number(&value, 40); /* First custom stop is 40 pixels from content origin. */
        if (fvm_object_set(context, array, "0", &value) != FVM_OK) fvm_example_fail("first tab stop", __FILE__, __LINE__); /* Canonical Array property storage updates length. */
        fvm_value_number(&value, 80); /* Second custom stop is strictly greater. */
        if (fvm_object_set(context, array, "1", &value) != FVM_OK) fvm_example_fail("second tab stop", __FILE__, __LINE__); /* The native field will retain copied numbers. */
        } /* setNewTextFormat copies either explicit positions or the default selection. */
        fvm_value_undefined(args); /* Prepare one borrowed format argument. */
        args[0].type = FVM_VALUE_OBJECT; /* Select the managed object representation. */
        args[0].object = format; /* Existing global retains this TextFormat. */
        if (fvm_object_call(context, field, "setNewTextFormat", 1, args, &result) != FVM_OK) fvm_example_fail("future tab format", __FILE__, __LINE__); /* Validate and copy the source Array before publication. */
        fvm_value_boolean(&value, 0); /* Observe natural width without wrapping first. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("disable tab wrapping", __FILE__, __LINE__); /* Keep existing multiline policy. */
        if (fvm_value_string(&value, "A\tV\tA") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) fvm_example_fail("tabbed text", __FILE__, __LINE__); /* Full assignment uses the copied future style. */
        fvm_value_boolean(&value, 1); /* Enable the left automatic anchor. */
        if (fvm_object_set(context, field, "autoSize", &value) != FVM_OK) fvm_example_fail("fit tabbed field", __FILE__, __LINE__); /* Measurement includes actual tab advances. */
        if (fvm_object_get(context, field, "textWidth", &result) != FVM_OK) fvm_example_fail("tabbed advance", __FILE__, __LINE__); /* Read intrinsic advance without gutter. */
        advance = result.number; /* Explicit mode gives 104px; default mode gives 199px. */
        if (fvm_object_get(context, field, "_width", &result) != FVM_OK) fvm_example_fail("tabbed fitted width", __FILE__, __LINE__); /* Outer width adds the four-pixel gutter. */
        width = result.number; /* The gutter adds four pixels in both modes. */
        printf("%s fit: advance=%.2f width=%.2f\n", tabbed == 2 ? "default-tabs" : "tabs", advance, width); /* The executable checker verifies both analytical values. */
        fvm_value_boolean(&value, 1); /* Enable wrapping before constraining width. */
        if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) fvm_example_fail("enable tab wrapping", __FILE__, __LINE__); /* The table restarts on each visual line. */
        fvm_value_number(&value, 54); /* Leave a 50px content box after gutter. */
        if (fvm_object_set(context, field, "_width", &value) != FVM_OK || fvm_object_get(context, field, "_height", &result) != FVM_OK) fvm_example_fail("wrapped tab height", __FILE__, __LINE__); /* Default oversized tabs occupy their own lines. */
        height = result.number; /* Explicit mode gives 140px; default mode gives 232px. */
        printf("%s wrap: width=54.00 height=%.2f\n", tabbed == 2 ? "default-tabs" : "tabs", height); /* Owned path export continues below. */
    } /* Source Array and native style have independent identities and owner lifetimes. */
    if (scrolling && scroll_demo(context, field, format)) { /* Execute the optional checked workflow on the same owner thread. */
        fvm_context_destroy(context); fvm_runtime_destroy(runtime); return 1; /* Release owners after a failed demonstration. */
    } /* Successful contours below reflect the final active scroll position. */
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
    (void)scrolling; /* NO_FONTS keeps explicit option parsing without geometry support. */
    (void)tabbed; /* Option parsing remains available without font support. */
    (void)insets; /* The option remains parseable without font support. */
    (void)variants; /* The option is still parseable in a NO_FONTS profile. */
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
