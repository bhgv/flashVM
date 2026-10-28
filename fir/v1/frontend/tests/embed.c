/* C99 host calls the namespaced parser repeatedly, including recovery after resource failures. */
#include "firsyn.h"
#include <stdlib.h>
#include <string.h>


/* No argv, output file, subprocess or VM is needed by the syntax API. */
int main(void)
{
    FirSyntaxDiagnostic diagnostic;
    const char *valid = "FIR 1.0\nmodule @m\nfunc @f(%x:i32)->i32 {^entry:return %x}\n";
    char *deep;
    size_t size, i;
    for (i = 0; i < 32; ++i)
    {
        if (firsyn_check("@", 1, &diagnostic) || !diagnostic.message[0])
        {
            return 1;
        }
        if (!firsyn_check(valid, strlen(valid), &diagnostic))
        {
            return 2;
        }
        if (!firsyn_check("FIR 1.0 // annotated header\nmodule @m\nfunc @f()->void {^entry:return}\n",
                          strlen("FIR 1.0 // annotated header\nmodule @m\nfunc @f()->void {^entry:return}\n"),
                          &diagnostic))
        {
            return 6;
        }
        if (firsyn_check(NULL, 1, &diagnostic))
        {
            return 3;
        }
        if (!firsyn_check(valid, strlen(valid), NULL))
        {
            return 4;
        }
    }
    deep = (char *)malloc(8192);
    if (!deep)
    {
        return 5;
    }
    strcpy(deep, "FIR 1.0\nmodule @m\nimport global @x:");
    size = strlen(deep);
    for (i = 0; i < 600; ++i)
    {
        memcpy(deep + size, "array<", 6);
        size += 6;
    }
    memcpy(deep + size, "i32", 3);
    size += 3;
    for (i = 0; i < 600; ++i)
    {
        deep[size++] = '>';
    }
    deep[size++] = '\n';
    if (firsyn_check(deep, size, &diagnostic) || diagnostic.code != -3)
    {
        free(deep);
        return 6;
    }
    free(deep);
    if (!firsyn_check(valid, strlen(valid), &diagnostic))
    {
        return 7;
    }
    return 0;
}
