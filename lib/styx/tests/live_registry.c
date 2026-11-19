/* Copyright (c) 2026 bhgv
 * EN: Exercise the example registry's bounded storage and control grammar directly.
 * Tests use its public backend contract, not private struct layout or counters.
 * RU: Проверить ограниченный реестр примера и ctl-грамматику напрямую через
 * backend contract, без доступа к приватным структурам и счётчикам.
 * standalone-reviewed: state transitions are checked through observable rows.
 */
#include "catalog.h"
#include <stdio.h>
#include <string.h>


/* EN: Borrow command through return; send its entire non-NUL byte sequence to ctl.
 * Return backend's acknowledged count/-1; no retained buffer or concurrency.
 * RU: Заимствовать command до возврата; отправить все байты без NUL в ctl.
 * Вернуть подтверждённую длину/-1; буфер не сохраняется, конкурентности нет.
 */
static long command(const char *text)
{
    return catalog_write(1, text, strlen(text), 0);
}


/* EN: Borrow id, query one stat row; return its presence, or -1 on backend failure.
 * Private copied row dies on return. No pin is acquired by this metadata query.
 * RU: Запросить stat id; вернуть наличие либо -1 при backend failure.
 * Локальная копия живёт до возврата; metadata query не приобретает pin.
 */
static int present(uint64_t id)
{
    CatalogRow row;
    size_t count;
    if (!catalog_query(0, id, 0, &row, 1, &count))
    {
        return -1;
    }
    return count == 1 && row.id == id;
}


/* EN: Reject malformed commands and verify that the visible namespace stays intact.
 * Return 1/0; all strings are borrowed static test data, no object is retained.
 * RU: Отклонить неверные команды и проверить неизменность видимого namespace.
 * Вернуть 1/0; строки — static test data, объекты не удерживаются.
 */
static int grammar(void)
{
    const char *bad[] = {
        "", "add\nadd", "add\nremove 100", "add\r", "add\n\n", "add ",
        "remove", "remove ", "remove +100", "remove -100", "remove  100",
        "remove 100 ", "remove 100x", "remove 0", "remove 18446744073709551616",
        "remove 18446744073709551716", "remove 100\rremove 100"
    };
    for (size_t index = 0; index < sizeof(bad) / sizeof(bad[0]); index++)
    {
        if (command(bad[index]) != -1 || present(100) != 1 || present(103) != 0)
        {
            fprintf(stderr, "malformed command changed registry: case %zu\n", index);
            return 0;
        }
    }
    char nul[] = {'a', 'd', 'd', 0, 'x'};
    char long_command[80];
    memset(long_command, 'a', sizeof(long_command));
    if (catalog_write(1, nul, sizeof(nul), 0) != -1 ||
        catalog_write(1, long_command, sizeof(long_command), 0) != -1 ||
        catalog_write(1, "add", 3, 1) != -1 || catalog_write(100, "add", 3, 0) != -1)
    {
        return 0;
    }
    return present(100) == 1 && present(103) == 0;
}


/* EN: Fill all 16 slots; hold both a window and child while hiding its name.
 * Verify bounded pagination, no premature reuse, final-close reclamation and
 * never-reused identity. Return 1/0, releasing successful test pins on happy path.
 * RU: Заполнить 16 мест; удержать окно и child при скрытии имени. Проверить
 * страницы, отсутствие раннего reuse, освобождение после final close и новые ID.
 * Вернуть 1/0; успешные test pins освобождаются на успешном пути.
 */
static int lifetime(void)
{
    if (command("add\n") != 4 || command("add\r\n") != 5)
    {
        return 0;
    }
    for (unsigned index = 3; index < 16; index++)
    {
        if (command("add") != 3)
        {
            return 0;
        }
    }
    CatalogRow rows[3];
    size_t count;
    for (size_t start = 0; start < 18; start++)
    {
        if (!catalog_query(1, 0, start, rows, 3, &count) ||
            count != (start >= 17 ? 0 : (17 - start < 3 ? 17 - start : 3)))
        {
            return 0;
        }
    }
    if (command("add") != -1 || !catalog_pin(100) || !catalog_pin(101) ||
        command("remove 100\r\n") != 12 || command("remove 100") != -1)
    {
        return 0;
    }
    if (command("add") != -1 || present(101) != 1 ||
        !catalog_query(1, 0, 0, rows, 1, &count) || count != 1 || rows[0].id != 1)
    {
        return 0;
    }
    catalog_unpin(100); /* Child fid still owns the hidden window's shared slot. */
    if (command("add") != -1 || present(101) != 1)
    {
        return 0;
    }
    catalog_unpin(101); /* Only this final close permits reuse of the slot. */
    if (present(100) != 0 || present(101) != 0 || command("add") != 3 ||
        present(148) != 1 || catalog_pin(101))
    {
        return 0;
    }
    return 1;
}


/* EN/RU: Own one fresh registry process; report contract failures explicitly.
 * Владеть свежим registry process; явно сообщить об отказе контракта.
 */
int main(void)
{
    if (!catalog_init(NULL) || !grammar() || !lifetime())
    {
        fputs("FAIL live registry grammar/lifetime\n", stderr);
        return 1;
    }
    puts("PASS live registry: strict commands, full slots, pins, pagination, fresh Qids");
    return 0;
}
