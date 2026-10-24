/* Retained interface owners, exact module scopes and deterministic semantic failures. */
#include "interface.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Check one rejected catalog while preserving the previously retained owner. */
static void rejected(const char *source, int code, LimboInterface **owner)
{
    LimboInterface *saved = *owner;
    LimboDiagnostic diagnostic;
    assert(!limbo_interface_parse(source, strlen(source), owner, &diagnostic));
    assert(*owner == saved && diagnostic.code == code && diagnostic.message[0]);
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
    assert(limbo_interface_parse(source, strlen(source), &owner, &diagnostic));
    memset(source, 'X', sizeof(source) - 1);
    assert(limbo_interface_count(owner) == 10);
    assert(limbo_interface_declaration(owner, 0, &declaration));
    assert(declaration.kind == LIMBO_INCLUDE && !declaration.name[0]);
    assert(limbo_interface_declaration(owner, 1, &declaration));
    assert(declaration.kind == LIMBO_MODULE && !strcmp(declaration.name, "M"));
    assert(declaration.scope == SIZE_MAX);
    assert(limbo_interface_declaration(owner, 3, &declaration));
    assert(declaration.kind == LIMBO_FUNCTION && declaration.scope == 1);
    assert(limbo_interface_declaration(owner, 7, &declaration));
    assert(!strcmp(declaration.name, "f") && declaration.scope == 6);
    assert(limbo_interface_find(owner, SIZE_MAX, "M", &found) && found == 1);
    assert(limbo_interface_find(owner, 1, "f", &found) && found == 3);
    assert(limbo_interface_find(owner, 6, "f", &found) && found == 7);
    assert(limbo_interface_find(owner, 8, "f", &found) && found == 9);
    found = SIZE_MAX;
    assert(!limbo_interface_find(owner, 3, "f", &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(owner, 999, "f", &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(owner, SIZE_MAX, "missing", &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(owner, SIZE_MAX, "", &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(owner, SIZE_MAX, NULL, &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(NULL, SIZE_MAX, "M", &found) && found == SIZE_MAX);
    assert(!limbo_interface_find(owner, SIZE_MAX, "M", NULL));
    saved = declaration;
    assert(!limbo_interface_declaration(owner, 99, &declaration));
    assert(!memcmp(&declaration, &saved, sizeof(saved)));
    assert(!limbo_interface_declaration(NULL, 0, &declaration));
    assert(!limbo_interface_declaration(owner, 0, NULL));
    assert(!limbo_interface_count(NULL) && !limbo_interface_tree(NULL));
    assert(!limbo_declaration_kind((LimboDeclKind)-1));
    assert(!limbo_declaration_kind((LimboDeclKind)999));
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
        assert(limbo_interface_parse(NULL, 0, &other, NULL));
        assert(!limbo_interface_count(other));
        limbo_interface_destroy(other);
    }
    assert(!limbo_interface_parse(NULL, 0, NULL, NULL));
    large = (char *)malloc(400000);
    assert(large);
    large[0] = 0;
    for (i = 0; i < 16385; ++i)
        used += (size_t)sprintf(large + used, "v%lu:int;", (unsigned long)i);
    rejected(large, -8, &owner);
    free(large);
    assert(limbo_interface_parse("After:module{run:fn();};", strlen("After:module{run:fn();};"),
                                 &other, NULL));
    limbo_interface_destroy(other);
    assert(limbo_interface_count(owner) == 10);
    limbo_interface_destroy(owner);
    limbo_interface_destroy(NULL);
    return 0;
}
