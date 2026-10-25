/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/parallel_render/main.c.
 */

/* Подключаем объявления используемых типов и функций. */
#include "flashvm/render_jobs.h"

/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>

/* Each worker owns a different pixel. A real renderer would use immutable
 * MovieClip display snapshots, then composite layers in display-list order. */
/* Draw the current value. */


/* Определяем функцию draw; параметры передаются вызывающей стороной в указанном порядке. */
static void draw(void *buffer, unsigned int pixel)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */
    /* Сохраняем вычисленное значение: unsigned char *rgba. */
    unsigned char *rgba = (unsigned char *)buffer;

    /* Computation / Вычисление и обновление состояния. */
    /* Сохраняем вычисленное значение: rgba[pixel * 4]. */
    rgba[pixel * 4] = 255;
    /* Сохраняем вычисленное значение: rgba[pixel * 4 + 1]. */
    rgba[pixel * 4 + 1] = (unsigned char)(pixel & 255);
    /* Сохраняем вычисленное значение: rgba[pixel * 4 + 2]. */
    rgba[pixel * 4 + 2] = 0;
    /* Сохраняем вычисленное значение: rgba[pixel * 4 + 3]. */
    rgba[pixel * 4 + 3] = 255;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */


/* Initialize the example, run its API operations and release owned resources. */


/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{ /* Begin scope: Начинаем блок инструкций или описание объекта. */
    /* Объявляем данные или дескрипторы, используемые в следующем блоке. */
    unsigned char pixels[64 * 4];
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_render_jobs_run(64, 4, draw, pixels) != 0)
    { /* Begin conditional branch: if (fvm_render_jobs_run(64, 4, draw, pixels) != 0). */
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    } /* End conditional branch: if (fvm_render_jobs_run(64, 4, draw, pixels) != 0). */

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("RGBA pixel 0: %u %u %u %u\n", (unsigned int)pixels[0], (unsigned int)pixels[1],
           /* Завершаем выражение с указанным значением или аргументами. */
           (unsigned int)pixels[2], (unsigned int)pixels[3]);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
} /* End scope: Начинаем блок инструкций или описание объекта. */
