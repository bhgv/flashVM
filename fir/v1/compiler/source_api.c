/* In-process source compiler: diagnostics and output belong to the caller. */
#include "fir.h"
#include "src1p.h"
#include "lower.h"
#include <stdlib.h>
#include <string.h>
extern int fir_utf8(const unsigned char *, size_t);
extern FILE *fir_temporary(void);
extern int source_parser_run(unsigned char *, int);
extern Error_Func Custom_Error;
static char *active_error;
static size_t active_capacity;
static void report(int code, int line, int column, long offset)
{
    if (active_error && active_capacity && !active_error[0])
        snprintf(active_error, active_capacity, "source:%d:%d: syntax E%d (byte %ld)", line, column, code, offset);
}
int fir_source(FILE *input, FILE *output, FirLanguage language, const char *name, char *error, size_t capacity)
{
    unsigned char *text; FILE *temporary; size_t length, i; int ok;
    if (error && capacity) error[0] = 0;
    if (!name) name = "tutorial";
    length = strlen(name);
    if (!input || !output || language < FIR_AS1 || language > FIR_LIMBO || !length || length > 254) goto invalid;
    for (i=0; i<length; ++i) {
        unsigned ch=(unsigned char)name[i];
        if (!((ch>='A' && ch<='Z') || (ch>='a' && ch<='z') || ch=='_' ||
            (i && ((ch>='0' && ch<='9') || ch=='.')))) goto invalid;
    }
    text=(unsigned char *)calloc(65538,1); temporary=fir_temporary();
    if (!text || !temporary) { free(text); if(temporary) fclose(temporary); goto invalid; }
    length=fread(text,1,65537,input);
    ok=!ferror(input) && length<=65536 && fir_utf8(text,length);
    if (!ok) { if(error && capacity) snprintf(error,capacity,"read error, invalid UTF-8/NUL or source exceeds 64 KiB"); goto done; }
    active_error=error; active_capacity=capacity;
    source_config(temporary,(int)language,error,capacity); Custom_Error=report;
    fprintf(temporary,"FIR 1.0\nmodule @%s\nfunc @main() -> void {\n^entry:\n",name);
    ok=source_parser_run(text,(int)length);
    if(!ok && error && capacity) snprintf(error,capacity,"scanner allocation failure");
    fputs("    return\n}\n",temporary);
    ok=ok && Successful() && !source_failed() && !ferror(temporary);
    Custom_Error=0; active_error=NULL; active_capacity=0;
    if (ok && (fflush(temporary) || fseek(temporary,0,SEEK_SET))) ok=0;
    if (ok) while ((length=fread(text,1,65536,temporary))!=0)
        if (fwrite(text,1,length,output)!=length) { ok=0; break; }
    if (ferror(temporary)) ok=0;
done: fclose(temporary); free(text); return ok;
invalid:
    if(error && capacity) snprintf(error,capacity,"invalid source arguments or temporary allocation failure");
    return 0;
}
