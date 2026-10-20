/* Link source and FIR libraries into one process; recover after bad input. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>
extern FILE *fir_temporary(void);
static FILE *input(const char *text)
{
    FILE *f=fir_temporary();
    if(!f) return NULL;
    if(fputs(text,f)<0 || fseek(f,0,SEEK_SET)) { fclose(f); return NULL; }
    return f;
}
static int snapshots(void *user, const char *phase, const FirModule *m)
{
    int *count=(int *)user;
    if(strcmp(phase,(*count%2) ? "after" : "before") || !m->count) return 0;
    ++*count; return 1;
}
static int cancel(void *user, const char *phase, const FirModule *module)
{ (void)user; (void)phase; (void)module; return 0; }
int main(void)
{
    int i,j,count=0; char error[512];
    FirRunOptions options={1,snapshots,&count};
    for(i=0;i<8;++i) for(j=FIR_AS1;j<=FIR_LIMBO;++j) {
        FILE *bad=input("trace(1 + );"), *fir=fir_temporary(), *obj=fir_temporary(), *src;
        unsigned char magic[4];
        if(!bad || !fir || !obj) return 1;
        if(fir_source(bad,fir,(FirLanguage)j,"broken",error,sizeof(error)) || ftell(fir)!=0 || !error[0]) return 2;
        fclose(bad);
        src=input(j==FIR_LIMBO ? "implement A; A: module { value: fn(): int; }; value(): int { x: int = 6; return x * 7; }" : "var x = 6; trace(x * 7);");
        if(!src || !fir_source(src,fir,(FirLanguage)j,"valid",error,sizeof(error))) return 3;
        fclose(src); rewind(fir);
        bad=input("FIR 1.0\nmodule @broken\nfunc @main() -> void { ^entry: trace return }");
        if(!bad || fir_run(FIR_CHECK,bad,NULL,NULL,error,sizeof(error))) return 4;
        fclose(bad);
        if(!fir_run(FIR_OBJECT,fir,obj,&options,error,sizeof(error))) return 5;
        rewind(obj);
        if(fread(magic,1,4,obj)!=4 || memcmp(magic,"SWO1",4)) return 6;
        fclose(obj); fclose(fir);
    }
    {
        FILE *fir=input("FIR 1.0\nmodule @callback\nfunc @main() -> void { ^entry: return }\n"),*out=fir_temporary();
        FirRunOptions canceled={1,cancel,NULL};
        if(!fir || !out) return 8;
        strcpy(error,"stale diagnostic");
        if(fir_run(FIR_OBJECT,fir,out,&canceled,error,sizeof(error)) || !strstr(error,"rejected") || ftell(out)!=0) return 9;
        rewind(fir);
        if(!fir_run(FIR_CHECK,fir,NULL,NULL,error,sizeof(error))) return 10;
        fclose(fir); fclose(out);
    }
    return count==64 ? 0 : 7;
}
