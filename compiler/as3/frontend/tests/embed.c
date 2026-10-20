/* Several compilations, including recovery, inside the host process. */
#include "frontend.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int main(void)
{
    unsigned i;
    As3Diagnostic diagnostic;
    const char *valid = "var x = 1;";
    for (i = 0; i < 32; ++i)
    {
        if (as3_check("@", 1, &diagnostic) || !diagnostic.message[0])
            return 1;
        if (!as3_check(valid, strlen(valid), &diagnostic))
            return 2;
        if (as3_check("\xff", 1, &diagnostic) || !strstr(diagnostic.message, "UTF-8"))
            return 3;
        if (!as3_check(valid, strlen(valid), NULL))
            return 4;
        if (as3_check("for(var x in)", 13, &diagnostic))
            return 5;
        if (!as3_check("trace(x in obj);", strlen("trace(x in obj);"), &diagnostic))
            return 6;
    }
    if (as3_check("function f(a:Number=1,b:Number):void {}",
                  strlen("function f(a:Number=1,b:Number):void {}"), &diagnostic) ||
        diagnostic.code != 1201)
        return 7;
    if (!as3_check("function f(a:Number=1,...rest):void {}",
                   strlen("function f(a:Number=1,...rest):void {}"), &diagnostic))
        return 8;
    {
        char deep[4096];
        unsigned j;
        strcpy(deep, "trace(");
        for (j = 0; j < 600; ++j)
            strcat(deep, "(");
        strcat(deep, "1");
        for (j = 0; j < 600; ++j)
            strcat(deep, ")");
        strcat(deep, ");");
        if (as3_check(deep, strlen(deep), &diagnostic) || diagnostic.code != -3)
            return 20;
        if (!as3_check(valid, strlen(valid), &diagnostic))
            return 21;
    }
    {
        size_t limit = 2u * 1024u * 1024u, used;
        char *flat = (char *)malloc(limit + 1);
        if (!flat)
            return 30;
        strcpy(flat, "function f(){");
        used = strlen(flat);
        while (used + 3 < limit)
        {
            flat[used++] = 'a';
            flat[used++] = ';';
        }
        flat[used++] = '}';
        flat[used] = 0;
        if (as3_check(flat, used, &diagnostic) || diagnostic.code != -4)
        {
            fprintf(stderr, "work diagnostic %d: %s\n", diagnostic.code, diagnostic.message);
            free(flat);
            return 31;
        }
        free(flat);
        if (!as3_check(valid, strlen(valid), &diagnostic))
            return 32;
    }
    {
        const char *xml = "var x=<a>{<b/>}</a>; var r=/[a-z]+/gi; var v:Vector.<Vector.<int>>;";
        const char *bad = "var x=<a>{1+}</a>;";
        for (i = 0; i < 32; ++i)
        {
            if (as3_check(bad, strlen(bad), &diagnostic))
                return 40;
            if (!as3_check(xml, strlen(xml), &diagnostic))
                return 41;
            if (as3_check("var r=/abc/gg;", 14, &diagnostic))
                return 42;
            if (!as3_check(valid, strlen(valid), &diagnostic))
                return 43;
        }
    }
    {
        char xml[8192];
        unsigned j;
        size_t used;
        strcpy(xml, "var x=");
        used = strlen(xml);
        for (j = 0; j < 600; ++j)
        {
            memcpy(xml + used, "<a>", 3);
            used += 3;
        }
        for (j = 0; j < 600; ++j)
        {
            memcpy(xml + used, "</a>", 4);
            used += 4;
        }
        xml[used++] = ';';
        xml[used] = 0;
        if (as3_check(xml, used, &diagnostic) || diagnostic.code != -3)
            return 44;
        if (!as3_check(valid, strlen(valid), &diagnostic))
            return 45;
    }
    {
        const char *bad[] = {"var x=<a>&#0;</a>;", "var x=<a><!--a--b--></a>;", "var x=<{tag}></>;",
                             "var x=<a p=\"&bad\"/>;"};
        unsigned n;
        for (n = 0; n < sizeof(bad) / sizeof(bad[0]); ++n)
        {
            if (as3_check(bad[n], strlen(bad[n]), &diagnostic))
                return 46;
            if (!as3_check(valid, strlen(valid), &diagnostic))
                return 47;
        }
    }
    {
        char pattern[2048];
        unsigned j;
        size_t used;
        strcpy(pattern, "var r=/");
        used = strlen(pattern);
        for (j = 0; j < 600; ++j)
            pattern[used++] = '(';
        pattern[used++] = 'a';
        for (j = 0; j < 600; ++j)
            pattern[used++] = ')';
        pattern[used++] = '/';
        pattern[used++] = ';';
        pattern[used] = 0;
        if (as3_check(pattern, used, &diagnostic) || diagnostic.code != -3)
            return 48;
        if (!as3_check("var r=/(?P<n>a)\\k<n>/;", strlen("var r=/(?P<n>a)\\k<n>/;"), &diagnostic))
            return 49;
        if (as3_check("var \\u0030x=1;", strlen("var \\u0030x=1;"), &diagnostic))
            return 50;
        if (!as3_check(valid, strlen(valid), &diagnostic))
            return 51;
    }
    return 0;
}
