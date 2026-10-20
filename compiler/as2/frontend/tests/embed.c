/* Several compilations, including recovery, inside the host process. */
#include "frontend.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int main(void)
{
    unsigned i; As2Diagnostic diagnostic;
    const char *valid="var x = 1;";
    for(i=0;i<32;++i) {
        if(as2_check("@",1,&diagnostic) || !diagnostic.message[0]) return 1;
        if(!as2_check(valid,strlen(valid),&diagnostic)) return 2;
        if(as2_check("\xff",1,&diagnostic) || !strstr(diagnostic.message,"UTF-8")) return 3;
        if(!as2_check(valid,strlen(valid),NULL)) return 4;
        if(as2_check("for(var x in)",13,&diagnostic)) return 5;
        if(!as2_check("trace(x in obj);",strlen("trace(x in obj);"),&diagnostic)) return 6;
    }
    {
        char deep[4096]; unsigned j;
        strcpy(deep,"trace(");
        for(j=0;j<600;++j) strcat(deep,"(");
        strcat(deep,"1");
        for(j=0;j<600;++j) strcat(deep,")");
        strcat(deep,");");
        if(as2_check(deep,strlen(deep),&diagnostic) || diagnostic.code!=-3) return 20;
        if(!as2_check(valid,strlen(valid),&diagnostic)) return 21;
    }
    {
        size_t limit=2u*1024u*1024u,used; char *flat=(char *)malloc(limit+1);
        if(!flat) return 30;
        strcpy(flat,"function f(){"); used=strlen(flat);
        while(used+3<limit) { flat[used++]='a'; flat[used++]=';'; }
        flat[used++]='}'; flat[used]=0;
        if(as2_check(flat,used,&diagnostic) || diagnostic.code!=-4) { fprintf(stderr,"work diagnostic %d: %s\n",diagnostic.code,diagnostic.message); free(flat); return 31; }
        free(flat);
        if(!as2_check(valid,strlen(valid),&diagnostic)) return 32;
    }
    return 0;
}
