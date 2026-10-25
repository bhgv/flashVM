/* Query contracts: multiline geometry, deterministic breakpoints and statement boundaries. */
#include <stdio.h>
#include <stdlib.h>
#include "sblmap.h"
#include <string.h>


/* Compare a complete map against endpoint, sentinel and unchanged-output expectations. */
int main(void)
{
    const char text[] = "limbo .sbl 2.1\nQueries\n2\na/main.b\nb/main.b\n8\n"
                        "0:10.20,0:12.3 7\n"
                        "0:10.20,0:12.3 7\n"
                        "0:15.0,0:15.1 8\n"
                        "0:13.0,0:13.2 8\n"
                        "1:10.0,1:10.8 9\n"
                        "0:0.0,0:0.0 0\n"
                        "0:20.8,0:19.9 7\n"
                        "0:25.0,1:26.0 7\n";
    SblMap *map = NULL;
    size_t pc = 999;
    if (!(sblmap_parse(text, sizeof(text) - 1, &map, NULL, 0) == SBL_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 11, 999, 0, &pc) == SBL_OK && pc == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 10, 20, 1, &pc) == SBL_OK && pc == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 12, 3, 0, &pc) == SBL_OK && pc == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 1, 10, 3, 0, &pc) == SBL_OK && pc == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pc = 999;
    if (!(sblmap_position(map, 0, 10, 19, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 12, 4, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 20, 0, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 25, 0, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 0, 0, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 2, 1, 0, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 11, 0, (size_t)-1, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(NULL, 0, 1, 0, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_position(map, 0, 1, 0, 0, NULL) == SBL_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 11, 15, 0, &pc) == SBL_OK && pc == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 10, 10, 1, &pc) == SBL_OK && pc == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 1, 10, 10, 0, &pc) == SBL_OK && pc == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pc = 999;
    if (!(sblmap_breakpoint(map, 0, 11, 12, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 0, 12, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 12, 11, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(NULL, 0, 1, 2, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 2, 1, 2, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 1, UINT32_MAX, (size_t)-1, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_breakpoint(map, 0, 1, 2, 0, NULL) == SBL_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, 0, &pc) == SBL_OK && pc == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, 2, &pc) == SBL_OK && pc == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, 5, &pc) == SBL_OK && pc == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pc = 999;
    if (!(sblmap_next_statement(map, 6, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, (size_t)-1, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(NULL, 0, &pc) == SBL_INVALID && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, 0, NULL) == SBL_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sblmap_destroy(map);
    if (!(sblmap_parse("limbo .sbl 2.1\nEmpty\n0\n0\n", strlen("limbo .sbl 2.1\nEmpty\n0\n0\n"), &map, NULL, 0) ==
          SBL_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_next_statement(map, 0, &pc) == SBL_NOT_FOUND && pc == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sblmap_destroy(map);
    return 0;
}
