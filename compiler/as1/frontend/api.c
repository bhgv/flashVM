#include "frontend.h"
#include "as1p.h"
#include <stdlib.h>
#include <string.h>
extern Error_Func Custom_Error;
extern int as1_parser_run(unsigned char *, int);
extern int as1_parser_limit_hit(void);
static As1Diagnostic *active;
static const char *const messages[] = {
#include "as1e.h"
};
static void report(int code, int line, int column, long offset)
{
    if(!active || active->message[0]) return;
    active->code=code; active->line=line; active->column=column; active->offset=offset;
    snprintf(active->message,sizeof(active->message),"%s",code>=0 && (size_t)code<sizeof(messages)/sizeof(messages[0]) ? messages[code] : "syntax error");
}
static int valid_utf8(const unsigned char *text, size_t n)
{
    size_t i;
    int remaining = 0;
    unsigned low = 0x80, high = 0xbf;
    for (i = 0; i < n; ++i) {
        unsigned c = text[i];
        if (remaining) {
            if (c < low || c > high) return 0;
            --remaining; low = 0x80; high = 0xbf;
        } else if (!c) return 0;
        else if (c < 0x80) continue;
        else if (c >= 0xc2 && c <= 0xdf) remaining = 1;
        else if (c >= 0xe0 && c <= 0xef) {
            remaining = 2;
            low = c == 0xe0 ? 0xa0 : 0x80; high = c == 0xed ? 0x9f : 0xbf;
        } else if (c >= 0xf0 && c <= 0xf4) {
            remaining = 3;
            low = c == 0xf0 ? 0x90 : 0x80; high = c == 0xf4 ? 0x8f : 0xbf;
        } else return 0;
    }
    return !remaining;
}

int as1_check(const void *data, size_t size, As1Diagnostic *diagnostic)
{
    As1Diagnostic local; int ok;
    if(!diagnostic) diagnostic=&local;
    memset(diagnostic,0,sizeof(*diagnostic));
    if((!data && size) || size>2u*1024u*1024u || !valid_utf8((const unsigned char *)data,size)) {
        diagnostic->code=-1;
        snprintf(diagnostic->message,sizeof(diagnostic->message),"invalid UTF-8/NUL or input exceeds 2 MiB"); return 0;
    }
    active=diagnostic; Custom_Error=report;
    ok=as1_parser_run((unsigned char *)data,(int)size);
    if(!ok) {
        int limit=as1_parser_limit_hit();
        diagnostic->code=limit ? limit==1 ? -3 : -4 : -2;
        snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",limit ? limit==1 ? "parser nesting limit exceeded" : "parser work limit exceeded" : "scanner allocation failure");
    }
    ok=ok && Successful(); Custom_Error=0; active=NULL; return ok;
}
int as1_read(FILE *input, As1Diagnostic *diagnostic)
{
    unsigned char *data; size_t n; int ok;
    if(!input) return as1_check(NULL,1,diagnostic);
    data=(unsigned char *)malloc(2u*1024u*1024u+1);
    if(!data) return as1_check(NULL,1,diagnostic);
    n=fread(data,1,2u*1024u*1024u+1,input);
    ok=!ferror(input) ? as1_check(data,n,diagnostic) : as1_check(NULL,1,diagnostic);
    free(data); return ok;
}
