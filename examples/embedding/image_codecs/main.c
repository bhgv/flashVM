/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/image_codecs/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/svg.h"
/* Подключаем объявления используемых типов и функций. */
#include "png.h"
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
    /* Сохраняем вычисленное значение: const char *xml. */
    const char *xml = "<svg width='8' height='8'><circle cx='4' cy='4' r='3' fill='red'/></svg>";
    /* Сохраняем вычисленное значение: FVM_SVG *svg. */
    FVM_SVG *svg = 0;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    FVM_RasterSurface surface;
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned char rgba[8 * 8 * 4], png[4096];
    /* Завершаем выражение с указанным значением или аргументами. */
    png_image image;
    /* Выполняем вызов sizeof с указанными аргументами. */
    png_alloc_size_t size = sizeof(png);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_svg_read(xml, strlen(xml), &svg) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: surface.pixels. */
    surface.pixels = rgba;
    /* Сохраняем вычисленное значение: surface.width. */
    surface.width = surface.height = 8;
    /* Сохраняем вычисленное значение: surface.stride. */
    surface.stride = 32;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_svg_rasterize(svg, &surface) != FVM_OK)
    /* Начинаем блок инструкций или описание объекта. */
    {
        /* Выполняем вызов fvm_svg_destroy с указанными аргументами. */
        fvm_svg_destroy(svg);
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 2;
        /* Завершаем текущий блок или описание объекта. */
    }
    /* Выполняем вызов fvm_svg_destroy с указанными аргументами. */
    fvm_svg_destroy(svg);

    /* Computation / Вычисление и обновление состояния. */
    /* Инициализируем область памяти заданным байтом. */
    memset(&image, 0, sizeof(image));
    /* Сохраняем вычисленное значение: image.version. */
    image.version = PNG_IMAGE_VERSION;
    /* Сохраняем вычисленное значение: image.width. */
    image.width = image.height = 8;
    /* Сохраняем вычисленное значение: image.format. */
    image.format = PNG_FORMAT_RGBA;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (!png_image_write_to_memory(&image, png, &size, 0, rgba, 0, 0))
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 3;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("SVG decoded to 8x8 RGBA; PNG encoded to %lu bytes\n", (unsigned long)size);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
