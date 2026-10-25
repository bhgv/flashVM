/* Diagnostic source-map inspector; no Dis process or filesystem path is opened by the map API. */
#include "sblmap.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>


/* Print expanded numeric source coordinates in a simple stable tabular form. */
int main(int argc, char **argv)
{
    FILE *input;
    unsigned char *data;
    size_t size, i;
    SblMap *map = NULL;
    SblEntry entry;
    SblResult result;
    char error[256] = "";
    if (argc != 2)
    {
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (!input)
    {
        return 2;
    }
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
    fclose(input);
    result = sblmap_parse(data, size, &map, error, sizeof(error));
    free(data);
    if (result != SBL_OK)
    {
        fprintf(stderr, "sblmap status %d: %s\n", (int)result, error);
        return 1;
    }
    printf("module\t%s\nfiles\t%lu\npc\t%lu\nprefix\t%lu\n",
           sblmap_module(map),
           (unsigned long)sblmap_files(map),
           (unsigned long)sblmap_count(map),
           (unsigned long)sblmap_prefix_bytes(map));
    for (i = 0; i < sblmap_files(map); ++i)
    {
        printf("file\t%lu\t%s\n", (unsigned long)i, sblmap_file(map, i));
    }
    for (i = 0; i < sblmap_count(map); ++i)
    {
        if (sblmap_entry(map, i, &entry) != SBL_OK)
        {
            sblmap_destroy(map);
            return 1;
        }
        printf("entry\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n",
               (unsigned long)i,
               (unsigned long)entry.start_file,
               (unsigned long)entry.start_line,
               (unsigned long)entry.start_column,
               (unsigned long)entry.stop_file,
               (unsigned long)entry.stop_line,
               (unsigned long)entry.stop_column,
               (unsigned long)entry.statement);
    }
    sblmap_destroy(map);
    return fflush(stdout) == EOF || ferror(stdout) ? 1 : 0;
}
