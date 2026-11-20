/* Copyright (c) 2026 bhgv
 * EN: Explicit C99 CLI for a generated scalar Dis remote owner.
 * RU: Явный C99 CLI генератора scalar Dis remote owner.
 */
#include "remote_owner.h"
#include <stdlib.h>
#include <string.h>


/* EN: Dispatch one validated CLI form. NULL out preflights without writing.
 * argv, catalog and optional diagnostic buffer are borrowed during dispatch.
 * RU: Выбрать проверенный вариант CLI. NULL out только проверяет параметры.
 * argv, catalog и буфер диагностики заимствованы на время вызова. */
static int generate(FILE *out, const LimboInterface *catalog, char **argv,
    int styx, int client, int interface_only, char *error, size_t capacity)
{
    if (styx)
    {
        return limbo_styx_client(out, catalog, argv[3], argv[4], argv[5], argv[6], argv[7], error, capacity);
    }
    if (client)
    {
        return limbo_remote_client(out, catalog, argv[3], interface_only, error, capacity);
    }
    return limbo_remote_owner(out, catalog, argv[2], argv[3], error, capacity);
}


/* EN: argv selects INPUT.m, include basename, provider implementation path and
 * OUTPUT.b. Parse/preflight before opening output; malformed/unsupported input
 * preserves its destination. Return 0 success, 2 failure. Own all temporary
 * memory/streams/catalog; no VM, network or implicit include lookup.
 * RU: argv задаёт INPUT.m, include basename, implementation path и OUTPUT.b.
 * Parse/preflight до открытия output сохраняют destination при invalid input.
 * Коды 0/2. Владеть memory/streams/catalog; без VM, сети и implicit includes.
 */
int main(int argc, char **argv)
{
    FILE *input = NULL;
    FILE *output = NULL;
    unsigned char *data = NULL;
    LimboInterface *catalog = NULL;
    LimboDiagnostic diagnostic;
    char error[256] = "input read/parse failed";
    size_t size;
    int success = 0;
    int interface_only = argc == 4 && !strcmp(argv[1], "--client-interface");
    int client = interface_only || (argc == 5 && !strcmp(argv[1], "--client"));
    int styx = argc == 9 && !strcmp(argv[1], "--styx-client");
    const char *input_path = (argc >= 4) ? argv[(client || styx) ? 2 : 1] : NULL;
    if (!input_path || (argc != 5 && !interface_only && !styx) || !strcmp(input_path, argv[argc - 1]))
    {
        fputs("usage: limbo_remote_owner INPUT.m INCLUDE.m IMPLEMENTATION.dis OUTPUT.b\n"
              "       limbo_remote_owner --client-interface INPUT.m OUTPUT.m\n"
              "       limbo_remote_owner --client INPUT.m GENERATED.m OUTPUT.b\n"
              "       limbo_remote_owner --styx-client INPUT.m INCLUDE.m REQUEST REPLY SENDTAG REPLYTAG OUTPUT.b\n", stderr);
        return 2;
    }
    input = fopen(input_path, "rb");
    data = malloc(2U * 1024U * 1024U + 1U);
    if (!input || !data)
    {
        goto done;
    }
    size = fread(data, 1, 2U * 1024U * 1024U + 1U, input);
    if (ferror(input) || size > 2U * 1024U * 1024U ||
        !limbo_interface_parse(data, size, &catalog, &diagnostic) ||
        !generate(NULL, catalog, argv, styx, client, interface_only, error, sizeof(error)))
    {
        goto done;
    }
    output = fopen(argv[argc - 1], "wb");
    if (!output)
    {
        snprintf(error, sizeof(error), "cannot open output");
        goto done;
    }
    success = generate(output, catalog, argv, styx, client, interface_only, error, sizeof(error));
    if (fclose(output))
    {
        success = 0;
    }
    output = NULL;
done:
    if (input)
    {
        fclose(input);
    }
    free(data);
    limbo_interface_destroy(catalog);
    if (!success)
    {
        fprintf(stderr, "remote owner: %s\n", error);
    }
    return success ? 0 : 2;
}
