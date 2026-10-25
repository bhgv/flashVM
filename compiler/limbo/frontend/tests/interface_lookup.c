/* Compare binary export lookup with a source-order linear oracle across many independent scopes. */
#include <stdio.h>
#include "interface.h"
#include <stdlib.h>
#include <string.h>


/* Search copied public declarations without consulting the implementation's sorted keys. */
static size_t linear(const LimboInterface *interface, size_t scope, const char *name)
{
    size_t i;
    LimboDeclaration declaration;
    for (i = 0; i < limbo_interface_count(interface); ++i)
    {
        if (!(limbo_interface_declaration(interface, i, &declaration)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (declaration.kind != LIMBO_INCLUDE && declaration.scope == scope && !strcmp(declaration.name, name))
        {
            return i;
        }
    }
    return SIZE_MAX;
}


/* Use names whose lexical order differs from their numeric source order. */
int main(void)
{
    char source[20000], name[40];
    size_t i, j, used = 0, found, scope, expected;
    LimboInterface *interface = NULL;
    LimboDeclaration declaration;
    for (i = 0; i < 20; ++i)
    {
        used += (size_t)sprintf(source + used, "M%lu:module{", (unsigned long)i);
        for (j = 0; j < 25; ++j)
        {
            used += (size_t)sprintf(source + used, "f%lu:fn(x:int):int;", (unsigned long)j);
        }
        used += (size_t)sprintf(source + used, "};");
    }
    used += (size_t)sprintf(source + used, "Upper,upper:int; \xe4\xb8\xad:int;");
    if (!(limbo_interface_parse(source, used, &interface, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < limbo_interface_count(interface); ++i)
    {
        if (!(limbo_interface_declaration(interface, i, &declaration)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        found = SIZE_MAX;
        expected = linear(interface, declaration.scope, declaration.name);
        if (!(expected == i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_find(interface, declaration.scope, declaration.name, &found)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(found == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < 20; ++i)
    {
        sprintf(name, "M%lu", (unsigned long)i);
        if (!(limbo_interface_find(interface, SIZE_MAX, name, &scope)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (j = 0; j < 30; ++j)
        {
            sprintf(name, "f%lu", (unsigned long)j);
            expected = linear(interface, scope, name);
            found = SIZE_MAX;
            if (!(limbo_interface_find(interface, scope, name, &found) == (expected != SIZE_MAX)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(found == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    found = SIZE_MAX;
    if (!(!limbo_interface_find(interface, SIZE_MAX, "UPPER", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_interface_destroy(interface);
    return 0;
}
