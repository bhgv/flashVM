/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/font_libraries/main.c.
 */

/* C89 embedding example: OpenType shaping -> independent vector contours ->
 * transform/control-point edit -> AGG/Cairo fill, border and image mask.
 * Uses the CC0 geometric fixture so it runs without installed system fonts. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/font.h"
/* Подключаем объявления используемых типов и функций. */
#include "../../../tests/fixtures/font_data.h"
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
    /* Сохраняем вычисленное значение: FVM_Font *font. */
    FVM_Font *font = 0;
    /* Сохраняем вычисленное значение: FVM_TextRun *run. */
    FVM_TextRun *run = 0;
    /* Сохраняем вычисленное значение: FVM_Path *path. */
    FVM_Path *path = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_PathBounds bounds;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_PathCommand first;
    /* Сохраняем вычисленное значение: FVM_Affine placement. */
    FVM_Affine placement = {1, 0, 0.2, 1, 10, 42};
    /* Организуем повторение операций до выполнения условия завершения. */
    double advance_x, advance_y;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned char pixels[64 * 64 * 4], image[64 * 64 * 4];
    /* Сохраняем вычисленное значение: FVM_RasterSurface surface. */
    FVM_RasterSurface surface = {0, 64, 64, 64 * 4}, source = {0, 64, 64, 64 * 4};
    /* Сохраняем вычисленное значение: FVM_RasterColor fill. */
    FVM_RasterColor fill = {30, 120, 220, 255}, border = {255, 255, 255, 255};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_Result result;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    size_t count;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned int x, y;
    /* Сохраняем вычисленное значение: int exit_code. */
    int exit_code = 1;

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: surface.pixels. */
    surface.pixels = pixels;
    /* Сохраняем вычисленное значение: source.pixels. */
    source.pixels = image;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 32, &font) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_font_shape(font, "AV fi", 5, FVM_TEXT_LTR, "en", &run) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Вызываем fvm_text_run_count: получаем число глифов в сформированном фрагменте.
     * run — сформированный фрагмент глифов. */
    count = fvm_text_run_count(run);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_text_run_path(font, run, &path, &advance_x, &advance_y) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* The exported path owns its geometry after font/run destruction. */
    /* Вызываем fvm_font_destroy: освобождаем шрифтовой дескриптор.
     * font — шрифтовой дескриптор. */
    fvm_font_destroy(font);
    /* Сохраняем вычисленное значение: font. */
    font = 0;
    /* Вызываем fvm_text_run_destroy: освобождаем сформированный фрагмент глифов.
     * run — сформированный фрагмент глифов. */
    fvm_text_run_destroy(run);
    /* Сохраняем вычисленное значение: run. */
    run = 0;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_path_transform(path, &placement) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_path_get(path, 0, &first) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_path_set_point(path, 0, 0, first.x[0] + 0.5, first.y[0]) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_path_bounds(path, &bounds) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;

    /* Computation / Вычисление и обновление состояния. */
    /* Инициализируем область памяти заданным байтом. */
    memset(pixels, 0, sizeof(pixels));
    /* Вызываем fvm_path_draw: отрисовываем путь с указанной заливкой и границей.
     * path — векторный путь.
     * &surface — строка или используемый дескриптор.
     * fill — цвет заливки/границы в RGBA.
     * 0 — ширина границы или текстового блока в пикселях.
     * 0 — 1 — правило чётности заливки, 0 — правило ненулевого обхода. */
    result = fvm_path_draw(path, &surface, fill, 0, 0);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (result == FVM_UNSUPPORTED)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Выводим строку с завершающим переводом строки. */
        puts("Vector export passed; raster backend disabled.");
        /* Сохраняем вычисленное значение: exit_code. */
        exit_code = 0;
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Проверяем условие перед выполнением следующей ветки. */
    if (result != FVM_OK || fvm_path_draw(path, &surface, border, 1, 0) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;
    /* Организуем повторение операций до выполнения условия завершения. */
    for (y = 0; y < 64; ++y)
        /* Организуем повторение операций до выполнения условия завершения. */
        for (x = 0; x < 64; ++x)
        /* Начинаем блок инструкций или описание объекта. */
        {
            /* Сохраняем вычисленное значение: size_t i. */
            size_t i = ((size_t)y * 64 + x) * 4;
            /* Сохраняем вычисленное значение: image[i]. */
            image[i] = (unsigned char)(x * 4);
            /* Сохраняем вычисленное значение: image[i + 1]. */
            image[i + 1] = (unsigned char)(y * 4);
            /* Сохраняем вычисленное значение: image[i + 2]. */
            image[i + 2] = 100;
            /* Сохраняем вычисленное значение: image[i + 3]. */
            image[i + 3] = 255;
            /* Завершаем текущий блок или описание объекта. */
        }
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_path_mask(path, &surface, &source, 0) != FVM_OK)
        /* Переходим к общему обработчику завершения и очистки ресурсов. */
        goto done;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("%lu glyphs; advance %.3f; vector bounds %.3f %.3f %.3f %.3f; fill/stroke/mask passed\n",
           /* Завершаем выражение с указанным значением или аргументами. */
           (unsigned long)count, advance_x, bounds.x_min, bounds.y_min, bounds.x_max, bounds.y_max);
    /* Сохраняем вычисленное значение: exit_code. */
    exit_code = 0;
/* Точка перехода для общей ветки обработки и очистки. */
done:
    /* Вызываем fvm_path_destroy: освобождаем векторный путь.
     * path — векторный путь. */
    fvm_path_destroy(path);

    /* Cleanup / Освобождение ресурсов. */
    /* Вызываем fvm_text_run_destroy: освобождаем сформированный фрагмент глифов.
     * run — сформированный фрагмент глифов. */
    fvm_text_run_destroy(run);
    /* Вызываем fvm_font_destroy: освобождаем шрифтовой дескриптор.
     * font — шрифтовой дескриптор. */
    fvm_font_destroy(font);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return exit_code;
    /* Завершаем текущий блок или описание объекта. */
}
