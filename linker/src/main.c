/* CLI policy only: files, options and diagnostics; all linking is in swl.h. */
#include "swl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INPUT_LIMIT (64u*1024u*1024u)
static int publish(void *user, const unsigned char *data, size_t size)
{
    FILE *out=fopen((const char *)user,"wb"); int ok;
    if(!out) return 0;
    ok=fwrite(data,1,size,out)==size; if(fclose(out)) ok=0; return ok;
}
static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *in=fopen(path,"rb"); unsigned char *data;
    if(!in) return NULL;
    data=(unsigned char *)malloc(INPUT_LIMIT+1);
    if(!data) { fclose(in); return NULL; }
    *size=fread(data,1,INPUT_LIMIT+1,in);
    if(ferror(in) || *size>INPUT_LIMIT) { free(data); fclose(in); return NULL; }
    if(fclose(in)) { free(data); return NULL; } return data;
}
int main(int argc, char **argv)
{
    SwlOptions options={550,400,12,0}; SwlContext *c; SwlObject *objects=NULL;
    int i,archive,ok=1; size_t count=0; char error[256]="read/allocation failure";
    if(argc==2 && !strcmp(argv[1],"--version")) { puts("swl 0.2.0 (SWO1/SWA1 to SWF8)"); return 0; }
    if(argc==2 && !strcmp(argv[1],"--help")) {
        puts("usage: swl link OUTPUT.swf [--extensions] INPUT.swo|INPUT.swa ...\n"
             "       swl archive OUTPUT.swa INPUT.swo ...\n"
             "Initializers run in input/member order. Duplicate modules fail.\n"
             "Libraries include all members. No implicit paths or lazy resolution.\n"
             "--extensions permits FlashVM channel requirements.\n"
             "C embedding: include swl.h, link swl_core; callbacks own output policy.\n"
             "Exit: 0 success, 1 input/link/output failure, 2 usage error."); return 0;
    }
    if(argc<4 || (strcmp(argv[1],"link") && strcmp(argv[1],"archive"))) return 2;
    archive=!strcmp(argv[1],"archive");
    for(i=3;i<argc;++i) if(!strcmp(argv[i],"--extensions") && !archive) options.allow_flashvm_extensions=1;
    c=swl_create(&options); if(!c) return 1;
    if(archive) { objects=(SwlObject *)calloc((size_t)argc,sizeof(*objects)); if(!objects) ok=0; }
    for(i=3;ok && i<argc;++i) {
        unsigned char *data; size_t n;
        if(!strcmp(argv[i],"--extensions") && !archive) continue;
        data=read_file(argv[i],&n);
        if(!data) { ok=0; break; }
        if(archive) { objects[count].data=data; objects[count++].size=n; }
        else {
            ok=n>=4 && !memcmp(data,"SWA1",4) ? swl_add_archive(c,data,n,error,sizeof(error)) : swl_add_object(c,data,n,error,sizeof(error));
            free(data);
        }
    }
    if(ok) ok=archive ? swl_archive(objects,count,publish,argv[2],error,sizeof(error)) : swl_emit(c,publish,argv[2],error,sizeof(error));
    while(count) free((void *)objects[--count].data);
    free(objects); swl_destroy(c);
    if(!ok) fprintf(stderr,"swl: %s\n",error);
    return ok ? 0 : 1;
}
