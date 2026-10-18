/* FlashVM: Embedding/ActionScript tutorial: main.
 * File: examples/extending/sqlite/main.c.
 */

/* C89: optional SQLite C adapter, with an in-memory database. */
/* Подключаем объявления используемых типов и функций. */
#include "flashvm/sqlite.h"
/* Подключаем объявления используемых типов и функций. */
#include <stdio.h>
/* Initialize the example, run its API operations and release owned resources. */
/* Точка входа: запускаем последовательность действий этого примера. */
int main(void)
/* Начинаем блок инструкций или описание объекта. */
{

    /* Initialization / Инициализация локального состояния. */
    /* Сохраняем вычисленное значение: FVM_SQLite *db. */
    FVM_SQLite *db = 0;
    /* Сохраняем вычисленное значение: long result. */
    long result = 0;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_sqlite_open(":memory:", &db) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 1;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_sqlite_exec(
            /* Определяем или используем scores; назначение и владение пояснены у функции. */
            db, "CREATE TABLE scores (points INTEGER); INSERT INTO scores VALUES (7);") != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 2;
    /* Проверяем условие перед выполнением следующей ветки. */
    if (fvm_sqlite_query_int(db, "SELECT points FROM scores", &result) != FVM_OK)
        /* Возвращаем результат вызывающей стороне и завершаем функцию. */
        return 3;

    /* Output / Вывод и передача результата. */
    /* Выводим результат в стандартный поток вывода. */
    printf("SQLite score: %ld\n", result);
    /* Вызываем fvm_sqlite_close: закрываем дескриптор базы SQLite.
     * db — открытый дескриптор базы данных. */
    fvm_sqlite_close(db);
    /* Возвращаем результат вызывающей стороне и завершаем функцию. */
    return result == 7 ? 0 : 4;
    /* Завершаем текущий блок или описание объекта. */
}
