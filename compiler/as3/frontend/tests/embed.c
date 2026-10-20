/* Several compilations, including recovery, inside the host process. */
#include "frontend.h"
#include <string.h>
int main(void)
{
    unsigned i; As3Diagnostic diagnostic;
    const char *valid="var x = 1;";
    for(i=0;i<32;++i) {
        if(as3_check("@",1,&diagnostic) || !diagnostic.message[0]) return 1;
        if(!as3_check(valid,strlen(valid),&diagnostic)) return 2;
        if(as3_check("\xff",1,&diagnostic) || !strstr(diagnostic.message,"UTF-8")) return 3;
        if(!as3_check(valid,strlen(valid),NULL)) return 4;
        if(as3_check("for(var x in)",13,&diagnostic)) return 5;
        if(!as3_check("trace(x in obj);",strlen("trace(x in obj);"),&diagnostic)) return 6;
    }
    return 0;
}
