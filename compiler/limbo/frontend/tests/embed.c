/* Several compilations, including recovery, inside the host process. */
#include "frontend.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(void)
{
    unsigned i;
    LimboDiagnostic diagnostic;
    const char *valid = "implement A;";
    for (i = 0; i < 32; ++i)
    {
        if (limbo_check("@", 1, &diagnostic) || !diagnostic.message[0])
        {
            return 1;
        }
        if (!limbo_check(valid, strlen(valid), &diagnostic))
        {
            return 2;
        }
        if (limbo_check("\xff", 1, &diagnostic) || !strstr(diagnostic.message, "UTF-8"))
        {
            return 3;
        }
        if (!limbo_check(valid, strlen(valid), NULL))
        {
            return 4;
        }
    }
    if (limbo_check(
            "implement A; f(){ x:=array[] of int; }", strlen("implement A; f(){ x:=array[] of int; }"), &diagnostic) ||
        diagnostic.code != 1301)
    {
        return 7;
    }
    if (!limbo_check(
            "implement A; f(){ x:=list of {1,2}; }", strlen("implement A; f(){ x:=list of {1,2}; }"), &diagnostic))
    {
        return 8;
    }
    {
        char deep[4096];
        unsigned j;
        strcpy(deep, "x: ");
        for (j = 0; j < 600; ++j)
        {
            strcat(deep, "ref ");
        }
        strcat(deep, "T");
        for (j = 0; j < 600; ++j)
        {
            strcat(deep, "");
        }
        strcat(deep, ";");
        if (limbo_check(deep, strlen(deep), &diagnostic) || diagnostic.code != -3)
        {
            return 20;
        }
        if (!limbo_check(valid, strlen(valid), &diagnostic))
        {
            return 21;
        }
    }
    {
        size_t limit = 2u * 1024u * 1024u, used;
        char *flat = (char *)malloc(limit + 1);
        if (!flat)
        {
            return 30;
        }
        strcpy(flat, "f(){");
        used = strlen(flat);
        while (used + 3 < limit)
        {
            flat[used++] = 'a';
            flat[used++] = ';';
        }
        flat[used++] = '}';
        flat[used] = 0;
        if (limbo_check(flat, used, &diagnostic) || diagnostic.code != -4)
        {
            fprintf(stderr, "work diagnostic %d: %s\n", diagnostic.code, diagnostic.message);
            free(flat);
            return 31;
        }
        free(flat);
        if (!limbo_check(valid, strlen(valid), &diagnostic))
        {
            return 32;
        }
    }
    return 0;
}
