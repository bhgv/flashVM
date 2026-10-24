/* Native debugger-query inspector. Numeric file IDs avoid path/basename ambiguity. */
#include "sblmap.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Accept decimal uint32 values only, independent of locale and native unsigned long width. */
static int number(const char *text, uint32_t *out)
{
    uint32_t value = 0;
    if (!text || !*text)
        return 0;
    while (*text)
    {
        unsigned digit;
        if (*text < '0' || *text > '9')
            return 0;
        digit = (unsigned)(*text++ - '0');
        if (value > (UINT32_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
    }
    *out = value;
    return 1;
}

/* Load a bounded snapshot, run one immutable query and print one decimal logical PC.
 * Exit 0: candidate; 3: absent; 1: malformed map/query or output error; 2: usage/I/O.
 * No Inferno devices, program controls, source files or executable mappings are opened. */
int main(int argc, char **argv)
{
    FILE *input;
    unsigned char *data;
    size_t size, pc = 0;
    uint32_t values[4] = {0, 0, 0, 0};
    int i, mode;
    SblMap *map = NULL;
    SblResult result;
    char error[256] = "";
    if (argc == 7 && !strcmp(argv[2], "position"))
        mode = 0;
    else if (argc == 7 && !strcmp(argv[2], "breakpoint"))
        mode = 1;
    else if (argc == 4 && !strcmp(argv[2], "statement"))
        mode = 2;
    else
    {
        fprintf(stderr, "usage: sblquery map.sbl position file line column first_pc\n"
                        "       sblquery map.sbl breakpoint file line last_line first_pc\n"
                        "       sblquery map.sbl statement pc\n");
        return 2;
    }
    for (i = 3; i < argc; ++i)
        if (!number(argv[i], &values[i - 3]))
            return 2;
    input = fopen(argv[1], "rb");
    if (!input)
        return 2;
    data = (unsigned char *)malloc(SBLMAP_MAX_INPUT + 1);
    if (!data)
    {
        fclose(input);
        return 2;
    }
    size = fread(data, 1, SBLMAP_MAX_INPUT + 1, input);
    if (ferror(input))
    {
        free(data);
        fclose(input);
        return 2;
    }
    if (fclose(input) != 0)
    {
        free(data);
        return 2;
    }
    result = sblmap_parse(data, size, &map, error, sizeof(error));
    free(data);
    if (result != SBL_OK)
    {
        fprintf(stderr, "sblmap status %d: %s\n", (int)result, error);
        return 1;
    }
    if (mode == 0)
        result = sblmap_position(map, values[0], values[1], values[2], values[3], &pc);
    else if (mode == 1)
        result = sblmap_breakpoint(map, values[0], values[1], values[2], values[3], &pc);
    else
        result = sblmap_next_statement(map, values[0], &pc);
    sblmap_destroy(map);
    if (result == SBL_NOT_FOUND)
        return 3;
    if (result != SBL_OK)
        return 1;
    printf("%lu\n", (unsigned long)pc);
    return fflush(stdout) == EOF || ferror(stdout) ? 1 : 0;
}
