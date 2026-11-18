/* Copyright (c) 2026 bhgv
 * EN: Native CLI scalar limits and owned configuration rollback, no descriptor or transport access.
 * RU: Границы скаляров CLI и rollback собственного config, без доступа к descriptors/transport.
 */
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Fatal explicit test contract with source line, never optimized away.
 * Явная обязательная проверка с номером строки, не удаляемая оптимизатором.
 */
static void check(int valid, int line)
{
    if (!valid)
    {
        fprintf(stderr, "CLI config line %d\n", line);
        exit(2);
    }
}
#define CHECK(value) check((value) != 0, __LINE__) /* Always evaluate owned-config contract. */


/* EN/RU: Exercise exact uint32/uint63/size bounds, tiny data body, and clearable failure state.
 * Проверить uint32/uint63/size-границы, минимальное data-body и очистку состояния при отказе.
 */
int main(void)
{
    char *arguments[] = {"unibus", "--profile", "ordered", "--phy-fd", "3", "--lane", "4294967295",
        "--tag", "0", "--tag", "4294967295", "--limit", "65472", "--window", "9223372036854775807",
        "--receive-records", "18446744073709551615"}; /* Native64 CLI maximums, no FD touched. */
    UnibusCliConfig config; /* Empty owned output after every parse attempt. */
    const char *error; /* Parameter-only diagnostic. */
    CHECK(unibuscli_parse(sizeof(arguments) / sizeof(*arguments), arguments, &config, &error));
    CHECK(config.profile == UNIBUS_CLI_ORDERED && config.fd == 3 && config.lane == 0xffffffffUL && config.count == 2);
    CHECK(config.window == (1ULL << 63) - 1 && config.limit == 65472 && config.records == (size_t)-1);
    unibuscli_clear(&config);
    arguments[12] = "1";
    CHECK(!unibuscli_parse(sizeof(arguments) / sizeof(*arguments), arguments, &config, &error));
    unibuscli_clear(&config);
    arguments[10] = "1";
    CHECK(unibuscli_parse(sizeof(arguments) / sizeof(*arguments), arguments, &config, &error) && config.limit == 1);
    unibuscli_clear(&config);
    arguments[10] = "4294967295";
    arguments[12] = "65472";
    const unsigned indexes[] = {4, 6, 10, 12, 14, 16}; /* Native fd, lane, tag, body, window, quota fields. */
    const char *invalid[] = {"2147483648", "4294967296", "4294967296", "65473", "9223372036854775808", "18446744073709551616"}; /* One beyond each bound. */
    for (unsigned i = 0; i < sizeof(indexes) / sizeof(*indexes); i++)
    {
        char *saved = arguments[indexes[i]]; /* Restore borrowed test argv after each independent refusal. */
        arguments[indexes[i]] = (char *)invalid[i];
        CHECK(!unibuscli_parse(sizeof(arguments) / sizeof(*arguments), arguments, &config, &error));
        unibuscli_clear(&config);
        arguments[indexes[i]] = saved;
    }
    char *bad[] = {"unibus", "--profile", "ordered", "--phy-fd", "3", "--lane", "71", "--tag", "1", "--tag", "1"}; /* Duplicate grant. */
    CHECK(!unibuscli_parse(sizeof(bad) / sizeof(*bad), bad, &config, &error));
    unibuscli_clear(&config);
    memset(&config, 0xa5, sizeof(config));
    CHECK(!unibuscli_parse(0, NULL, &config, &error));
    unibuscli_clear(&config); /* Invalid argc still leaves valid clearable ownership. */
    return 0;
}
