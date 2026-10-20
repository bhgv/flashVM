/* Several compilations, including recovery, inside the host process. */
#include "frontend.h"
#include <string.h>
int main(void)
{
    unsigned i; LimboDiagnostic diagnostic;
    const char *valid="implement A;";
    for(i=0;i<32;++i) {
        if(limbo_check("@",1,&diagnostic) || !diagnostic.message[0]) return 1;
        if(!limbo_check(valid,strlen(valid),&diagnostic)) return 2;
        if(limbo_check("\xff",1,&diagnostic) || !strstr(diagnostic.message,"UTF-8")) return 3;
        if(!limbo_check(valid,strlen(valid),NULL)) return 4;
    }
    if(limbo_check("implement A; f(){ x:=array[] of int; }",strlen("implement A; f(){ x:=array[] of int; }"),&diagnostic) || diagnostic.code!=1301) return 7;
    if(!limbo_check("implement A; f(){ x:=list of {1,2}; }",strlen("implement A; f(){ x:=list of {1,2}; }"),&diagnostic)) return 8;
    return 0;
}
