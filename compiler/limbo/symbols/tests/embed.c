/* C99 API regression: ownership, exact line lookup and unchanged outputs on every error. */
#include "sblmap.h"
#include <assert.h>
#include <string.h>
/* Exercise two retained maps across repeated parse failures and scanner resets. */
int main(void)
{
    char input[] = "limbo .sbl 2.1\nExample\n1\nmain.b\n2\n1.2,3 0\n4,5 1\n0\n";
    SblMap *map = NULL, *other = NULL, *before;
    SblEntry entry, saved;
    char error[2];
    size_t pc = 99, i;
    assert(sblmap_parse(input, strlen(input), &map, error, sizeof(error)) == SBL_OK);
    memset(input, 'X', sizeof(input) - 1);
    assert(!strcmp(sblmap_module(map), "Example"));
    assert(!strcmp(sblmap_file(map, 0), "main.b"));
    assert(sblmap_entry(map, 0, &entry) == SBL_OK);
    saved = entry;
    assert(sblmap_entry(map, 2, &entry) == SBL_NOT_FOUND && !memcmp(&entry, &saved, sizeof(entry)));
    assert(sblmap_entry(NULL, 0, &entry) == SBL_INVALID && !memcmp(&entry, &saved, sizeof(entry)));
    assert(sblmap_find(map, 0, 1, 1, &pc) == SBL_OK && pc == 1);
    pc = 99;
    assert(sblmap_find(map, 0, 2, 0, &pc) == SBL_NOT_FOUND && pc == 99);
    assert(sblmap_find(map, 9, 1, 0, &pc) == SBL_INVALID && pc == 99);
    assert(!sblmap_file(map, 1) && !sblmap_module(NULL));
    before = map;
    for (i = 0; i < 64; ++i)
    {
        assert(sblmap_parse("bad", 3, &map, error, sizeof(error)) != SBL_OK && map == before);
        assert(sblmap_parse("limbo .sbl 2.1\nEmpty\n0\n0\n",
                            strlen("limbo .sbl 2.1\nEmpty\n0\n0\n"), &other, NULL, 0) == SBL_OK);
        sblmap_destroy(other);
        assert(sblmap_entry(map, 1, &entry) == SBL_OK && entry.start_line == 1);
    }
    assert(sblmap_parse("x", 4u * 1024u * 1024u + 1, &map, NULL, 0) == SBL_LIMIT && map == before);
    assert(sblmap_parse(NULL, 1, &map, NULL, 0) == SBL_INVALID && map == before);
    sblmap_destroy(map);
    sblmap_destroy(NULL);
    return 0;
}
