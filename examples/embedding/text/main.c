/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/embedding/text/main.c.
 */

/* C89 example: count Unicode scalars, then search an ASCII pattern. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/utf8.h"
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/regex.h"
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
    /* Сохраняем вычисленное значение: const char *text. */
    const char *text = "Hello, \xD0\xBC\xD0\xB8\xD1\x80!";
    /* Сохраняем вычисленное значение: size_t scalars. */
    size_t scalars = 0, start = 0, length = 0;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_utf8_validate(text, strlen(text), &scalars) != FVM_UTF8_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("Unicode scalars: %lu\n", (unsigned long)scalars);
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_regex_search("Hello", text, strlen(text), &start, &length) == 1)
        /* Выводим результат в стандартный поток вывода. */
        printf("Match: byte %lu, length %lu\n", (unsigned long)start, (unsigned long)length);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return 0;
    /* Завершаем текущий блок или описание объекта. */
}
