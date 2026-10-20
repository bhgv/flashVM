/* No shell or subprocess: transactional adds and reusable output callbacks. */
#include "swl.h"
#include <stdlib.h>
#include <string.h>
typedef struct { unsigned char *data; size_t size; } Capture;
static int capture(void *user, const unsigned char *p, size_t n)
{
    Capture *c=(Capture *)user;
    c->data=(unsigned char *)malloc(n); if(!c->data) return 0;
    memcpy(c->data,p,n); c->size=n; return 1;
}
static int reject(void *user, const unsigned char *p, size_t n)
{ (void)user; (void)p; (void)n; return 0; }
int main(void)
{
    SwlContext *c=swl_create(NULL); Capture first={0},second={0}; char e[256];
    unsigned char object[26]={83,87,79,49,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,97,0};
    unsigned char archive[47]={83,87,65,49,1,0,0,0,2,0,0,0,26,0,0,0};
    uint32_t hash=UINT32_C(2166136261); unsigned i;
    hash=(hash^97)*UINT32_C(16777619); hash=hash*UINT32_C(16777619);
    for(i=0;i<4;++i) object[20+i]=(unsigned char)(hash>>(8*i));
    memcpy(archive+16,object,sizeof(object)); archive[42]=1;
    if(swl_add_archive(c,archive,sizeof(archive),e,sizeof(e))) return 8;
    if(!swl_add_object(c,object,sizeof(object),e,sizeof(e))) return 9;
    if(!c || !swl_add_rect(c,"panel",120,80,0x20a060ff,e,sizeof(e))) return 1;
    if(swl_add_rect(c,"panel",1,1,0,e,sizeof(e)) || swl_add_rect(c,"bad",0,1,0,e,sizeof(e))) return 2;
    if(!swl_place(c,"panel",20,30,1,e,sizeof(e)) || swl_place(c,"panel",20,30,1,e,sizeof(e))) return 3;
    if(swl_play(c,"panel",e,sizeof(e)) || swl_add_archive(c,"SWA1",4,e,sizeof(e))) return 4;
    if(swl_emit(c,reject,NULL,e,sizeof(e))) return 5;
    if(!swl_emit(c,capture,&first,e,sizeof(e)) || !swl_emit(c,capture,&second,e,sizeof(e))) return 6;
    if(first.size!=second.size || memcmp(first.data,second.data,first.size)) return 7;
    swl_destroy(c); free(first.data); free(second.data); return 0;
}
