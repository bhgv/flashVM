/* C99 API regression: ownership, exact line lookup and unchanged outputs on every error. */
#include <stdio.h>
#include <stdlib.h>
#include "sblmap.h"
#include <string.h>


/* Exercise two retained maps across repeated parse failures and scanner resets. */
int main(void)
{
    char input[] = "limbo .sbl 2.1\nExample\n1\nmain.b\n2\n1.2,3 0\n4,5 1\n0\n";
    SblMap *map = NULL, *other = NULL, *before;
    SblEntry entry, saved;
    char error[2];
    size_t pc = 99, i;
    if (!(sblmap_parse(input, strlen(input), &map, error, sizeof(error)) == SBL_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(input, 'X', sizeof(input) - 1);
    if (!(!strcmp(sblmap_module(map), "Example")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(sblmap_file(map, 0), "main.b")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_entry(map, 0, &entry) == SBL_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = entry;
    if (!(sblmap_entry(map, 2, &entry) == SBL_NOT_FOUND && !memcmp(&entry, &saved, sizeof(entry))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_entry(NULL, 0, &entry) == SBL_INVALID && !memcmp(&entry, &saved, sizeof(entry))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_find(map, 0, 1, 1, &pc) == SBL_OK && pc == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pc = 99;
    if (!(sblmap_find(map, 0, 2, 0, &pc) == SBL_NOT_FOUND && pc == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_find(map, 9, 1, 0, &pc) == SBL_INVALID && pc == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!sblmap_file(map, 1) && !sblmap_module(NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = map;
    for (i = 0; i < 64; ++i)
    {
        if (!(sblmap_parse("bad", 3, &map, error, sizeof(error)) != SBL_OK && map == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(sblmap_parse("limbo .sbl 2.1\nEmpty\n0\n0\n", strlen("limbo .sbl 2.1\nEmpty\n0\n0\n"), &other, NULL, 0) ==
              SBL_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sblmap_destroy(other);
        if (!(sblmap_entry(map, 1, &entry) == SBL_OK && entry.start_line == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(sblmap_parse("x", 4u * 1024u * 1024u + 1, &map, NULL, 0) == SBL_LIMIT && map == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sblmap_parse(NULL, 1, &map, NULL, 0) == SBL_INVALID && map == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sblmap_destroy(map);
    sblmap_destroy(NULL);
    return 0;
}
