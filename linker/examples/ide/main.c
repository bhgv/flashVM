/* Complete in-process host: source -> FIR -> optimizer -> SWO -> library -> SWF.
 * No command strings, argv-based compiler API or subprocess invocation. */
#include "fir.h"
#include "swl.h"
#include <stdlib.h>
#include <string.h>
#ifdef IDE_WITH_FRONTENDS
#include "as1/frontend/frontend.h"
#include "as2/frontend/frontend.h"
#include "as3/frontend/frontend.h"
#include "limbo/frontend/frontend.h"
#endif
typedef struct { unsigned char *data; size_t size; } Memory;
static int capture(void *user, const unsigned char *data, size_t n)
{
    Memory *m=(Memory *)user; m->data=(unsigned char *)malloc(n);
    if(!m->data) return 0;
    memcpy(m->data,data,n); m->size=n; return 1;
}
static int publish(void *user, const unsigned char *data, size_t n)
{
    FILE *out=fopen((const char *)user,"wb"); int ok;
    if(!out) return 0;
    ok=fwrite(data,1,n,out)==n; if(fclose(out)) ok=0; return ok;
}
static int snapshot(void *user, const char *phase, const FirModule *module)
{
    int *count=(int *)user;
    if(!module->count || strcmp(phase,*count%2 ? "after" : "before")) return 0;
    ++*count; /* An IDE can call fir_print to persist this borrowed snapshot. */
    return 1;
}
static int compile(const char *text, FirLanguage language, const char *name, Memory *object, char *error, size_t cap, int *snapshots)
{
    FILE *source=fir_temporary(),*fir=fir_temporary(),*out=fir_temporary();
    FirRunOptions options={1,snapshot,snapshots}; long n; unsigned char *bytes=NULL; int ok=0;
    if(!source || !fir || !out) goto done;
    if(fputs(text,source)<0 || fseek(source,0,SEEK_SET)) goto done;
    if(!fir_source(source,fir,language,name,error,cap) || fseek(fir,0,SEEK_SET)) goto done;
    if(!fir_run(FIR_OBJECT,fir,out,&options,error,cap) || fseek(out,0,SEEK_END)) goto done;
    n=ftell(out); if(n<0 || fseek(out,0,SEEK_SET)) goto done;
    bytes=(unsigned char *)malloc((size_t)n);
    if(!bytes || fread(bytes,1,(size_t)n,out)!=(size_t)n) goto done;
    object->data=bytes; object->size=(size_t)n; bytes=NULL; ok=1;
done:
    free(bytes);
    if(source) fclose(source);
    if(fir) fclose(fir);
    if(out) fclose(out);
    return ok;
}
int main(int argc, char **argv)
{
    Memory first={0},second={0},library={0}; SwlObject objects[2];
    SwlContext *linker=NULL; char error[512]="IDE I/O failure"; int ok=0,snapshots=0;
    if(argc!=2) return 2;
#ifdef IDE_WITH_FRONTENDS
    if(!as1_check("var x = 1;",10,NULL) || !as2_check("var x = 1;",10,NULL) ||
       !as3_check("var x = 1;",10,NULL) || !limbo_check("implement A;",12,NULL)) goto done;
#endif
    if(!compile("var x:Number = 6; trace(x * 7);",FIR_AS2,"first",&first,error,sizeof(error),&snapshots) ||
       !compile("implement V; V: module { value: fn(): int; }; value(): int { x: int = 42; return x - 6; }",FIR_LIMBO,"second",&second,error,sizeof(error),&snapshots)) goto done;
    objects[0].data=first.data; objects[0].size=first.size;
    objects[1].data=second.data; objects[1].size=second.size;
    if(!swl_archive(objects,2,capture,&library,error,sizeof(error))) goto done;
    linker=swl_create(NULL);
    if(!linker || !swl_add_archive(linker,library.data,library.size,error,sizeof(error)) ||
       !swl_add_rect(linker,"panel",120,80,0x20a060ff,error,sizeof(error)) ||
       !swl_place(linker,"panel",20,30,1,error,sizeof(error)) || snapshots!=4) goto done;
    ok=swl_emit(linker,publish,argv[1],error,sizeof(error));
done:
    if(!ok) fprintf(stderr,"%s\n",error);
    swl_destroy(linker); free(first.data); free(second.data); free(library.data); return ok ? 0 : 1;
}
