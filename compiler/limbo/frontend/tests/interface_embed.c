/* Retained interface owners, exact module scopes and deterministic semantic failures. */
#include <stdio.h>
#include "interface.h"
#include <stdlib.h>
#include <string.h>


/* Check one rejected catalog while preserving the previously retained owner. */
static void rejected(const char *source, int code, LimboInterface **owner)
{
    LimboInterface *saved = *owner;
    LimboDiagnostic diagnostic;
    if (!(!limbo_interface_parse(source, strlen(source), owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(*owner == saved && diagnostic.code == code && diagnostic.message[0]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Exercise typed interface declarations without inventing C layouts or runtime signatures. */
int main(void)
{
    char source[] = "include \"missing.m\"; M:module { T:adt {x:int;}; f:fn(x:T):int; "
                    "a,b:chan of string; N:module{f:fn();}; }; Other:module{f:fn();};";
    LimboInterface *owner = NULL, *other = NULL;
    LimboDeclaration declaration, saved;
    LimboDiagnostic diagnostic;
    size_t i, used = 0, found = SIZE_MAX;
    char *large;
    if (!(limbo_interface_parse(source, strlen(source), &owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(source, 'X', sizeof(source) - 1);
    if (!(limbo_interface_count(owner) == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_declaration(owner, 0, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(declaration.kind == LIMBO_INCLUDE && !declaration.name[0]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_declaration(owner, 1, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(declaration.kind == LIMBO_MODULE && !strcmp(declaration.name, "M")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(declaration.scope == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_declaration(owner, 3, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(declaration.kind == LIMBO_FUNCTION && declaration.scope == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_declaration(owner, 7, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(declaration.name, "f") && declaration.scope == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_find(owner, SIZE_MAX, "M", &found) && found == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_find(owner, 1, "f", &found) && found == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_find(owner, 6, "f", &found) && found == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_find(owner, 8, "f", &found) && found == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    found = SIZE_MAX;
    if (!(!limbo_interface_find(owner, 3, "f", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(owner, 999, "f", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(owner, SIZE_MAX, "missing", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(owner, SIZE_MAX, "", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(owner, SIZE_MAX, NULL, &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(NULL, SIZE_MAX, "M", &found) && found == SIZE_MAX))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_find(owner, SIZE_MAX, "M", NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = declaration;
    if (!(!limbo_interface_declaration(owner, 99, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(&declaration, &saved, sizeof(saved))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_declaration(NULL, 0, &declaration)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_declaration(owner, 0, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_interface_count(NULL) && !limbo_interface_tree(NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_declaration_kind((LimboDeclKind)-1)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!limbo_declaration_kind((LimboDeclKind)999)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 32; ++i)
    {
        rejected("M:module{a:int; a:con 2;};", -7, &owner);
        rejected("a,a:int;", -7, &owner);
        rejected("M:module{a:fn(); a:fn(x:int);};", -7, &owner);
        rejected("implement M;", -6, &owner);
        rejected("f(){}", -6, &owner);
        rejected("x:=2;", -6, &owner);
        rejected("A,B:module{f:fn();};", -6, &owner);
        rejected("(x,y):=(1,2);", -6, &owner);
        if (!(limbo_interface_parse(NULL, 0, &other, NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!limbo_interface_count(other)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_interface_destroy(other);
    }
    if (!(!limbo_interface_parse(NULL, 0, NULL, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    large = (char *)malloc(400000);
    if (!(large))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    large[0] = 0;
    for (i = 0; i < 16385; ++i)
    {
        used += (size_t)sprintf(large + used, "v%lu:int;", (unsigned long)i);
    }
    rejected(large, -8, &owner);
    free(large);
    if (!(limbo_interface_parse("After:module{run:fn();};", strlen("After:module{run:fn();};"), &other, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_interface_destroy(other);
    if (!(limbo_interface_count(owner) == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_interface_destroy(owner);
    limbo_interface_destroy(NULL);
    return 0;
}
