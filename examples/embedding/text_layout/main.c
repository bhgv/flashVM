/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/text_layout/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/text_layout.h"
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
/* Выбираем ветку компиляции для указанной конфигурации. */
#ifdef FVM_HAS_FONTS
    /* Сохраняем вычисленное значение: const char text[]. */
    const char text[] = "AV fi \xd7\x90\xd7\x91\n\xd8\xa8\xd8\xa8\xd8\xa8";
    /* Сохраняем вычисленное значение: FVM_Font *font. */
    FVM_Font *font = 0;
    /* Сохраняем вычисленное значение: FVM_TextLayout *layout. */
    FVM_TextLayout *layout = 0;
    /* Сохраняем вычисленное значение: FVM_TextLayoutOptions options. */
    FVM_TextLayoutOptions options = {120, 48, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_TextLayoutMetrics metrics;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_TextLayoutLine line;
    /* Сохраняем вычисленное значение: FVM_Path *path. */
    FVM_Path *path = 0, *letter = 0;
    /* Сохраняем вычисленное значение: FVM_Affine skew. */
    FVM_Affine skew = {1, 0, 0.2, 1, 0, 0};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned char pixels[192 * 160 * 4];
    /* Сохраняем вычисленное значение: FVM_RasterSurface surface. */
    FVM_RasterSurface surface = {0, 192, 160, 768};
    /* Сохраняем вычисленное значение: FVM_RasterColor color. */
    FVM_RasterColor color = {30, 90, 220, 255};
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    size_t i;
    /* Сохраняем вычисленное значение: surface.pixels. */
    surface.pixels = pixels;
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_text_layout_create(font, text, strlen(text), &options, &layout) == FVM_OK);
    /* Выполняем вызов fvm_font_destroy с указанными аргументами. */
    fvm_font_destroy(font);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK);
    /* Выводим результат в стандартный поток вывода. */
    printf("lines=%lu glyphs=%lu advance=%.2f height=%.2f\n", (unsigned long)metrics.line_count,
           /* Завершаем выражение с указанным значением или аргументами. */
           (unsigned long)metrics.glyph_count, metrics.advance_width, metrics.height);
    /* Организуем повторение операций до выполнения условия завершения. */
    for (i = 0; i < metrics.line_count; ++i)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Выполняем вызов assert с указанными аргументами. */
        assert(fvm_text_layout_line(layout, i, &line) == FVM_OK);
        /* Выводим результат в стандартный поток вывода. */
        printf("line %lu: bytes [%lu,%lu), baseline %.2f, direction %s\n", (unsigned long)i,
               /* Продолжаем объявление или выражение с указанными параметрами. */
               (unsigned long)line.byte_start, (unsigned long)line.byte_end, line.baseline,
               /* Завершаем выражение с указанным значением или аргументами. */
               line.direction == FVM_TEXT_RTL ? "RTL" : "LTR");
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_text_layout_path(layout, &path) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_text_layout_glyph_path(layout, 0, &letter) == FVM_OK);
    /* Выполняем вызов fvm_text_layout_destroy с указанными аргументами. */
    fvm_text_layout_destroy(layout);
    /* Both the block and its first glyph are now independent editable vectors. */
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_path_transform(letter, &skew) == FVM_OK);
    /* Инициализируем область памяти заданным байтом. */
    memset(pixels, 0, sizeof(pixels));
/* Выбираем ветку компиляции для указанной конфигурации. */
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_OK);
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_path_draw(letter, &surface, color, 1, 0) == FVM_OK);
/* Начинаем альтернативную ветку компиляции. */
#else
    /* Выполняем вызов assert с указанными аргументами. */
    assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_UNSUPPORTED);
/* Завершаем условный блок компиляции. */
#endif
    /* Выполняем вызов fvm_path_destroy с указанными аргументами. */
    fvm_path_destroy(letter);
    /* Выполняем вызов fvm_path_destroy с указанными аргументами. */
    fvm_path_destroy(path);
/* Начинаем альтернативную ветку компиляции. */
#else
    /* Выводим строку с завершающим переводом строки. */
    puts("Text layout requires FONTS in DEFS.txt.");
/* Завершаем условный блок компиляции. */
#endif
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
