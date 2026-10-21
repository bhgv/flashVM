/* Allocation failure and running-clunk lifetime: wire fids vanish before native cleanup finishes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int fail;
static void *allocate(size_t n,size_t size) { return fail?0:calloc(n,size); }
#define calloc allocate
#include "../../src/protocol/r9p.c"
#undef calloc
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"9p-lifetime:%d %s\n",__LINE__,#x);return 1; } } while(0)
typedef struct Host { FVM_R9P *server; int ok; } Host;
static FVM_RResult handler(const FVM_RRequest *q,void *out,size_t capacity,size_t *used,void *raw) {
    Host *h=(Host *)raw;
    const unsigned char clunk[]={11,0,0,0,120,1,0,7,0,0,0};
    unsigned char reply[FVM_R9P_MSIZE];size_t n,consumed;
    (void)q;(void)out;(void)capacity;*used=0;
    h->ok=fvm_r9p_request(h->server,clunk,sizeof(clunk),reply,sizeof(reply),&n,&consumed)==FVM_R_OK &&
        n>=7 && reply[4]==107 && consumed==sizeof(clunk) && !r9_fid(h->server,7) &&
        h->server->fids[0].opened && !h->server->fids[0].active;
    return FVM_R_OK;
}
int main(void) {
    FVM_RDispatch *d=0;FVM_RFS *fs=0;FVM_R9P *server=0,*saved;
    FVM_RHandle target;FVM_ROperation operation={7,FVM_R_READ};FVM_RFSNode node;
    FVM_RFSFile file;Host host;size_t used,completed;
    unsigned char output[FVM_R9P_MSIZE];
    memset(&host,0,sizeof(host));memset(&node,0,sizeof(node));
    CHECK(fvm_rdispatch_create(0,&d)==FVM_R_OK);
    CHECK(fvm_rdispatch_register(d,"/test",&operation,1,handler,&host,&target)==FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d,target,1,FVM_R_READ)==FVM_R_OK && fvm_rfs_create(d,1,&fs)==FVM_R_OK);
    saved=server;fail=1;CHECK(fvm_r9p_create(fs,&server)==FVM_R_NOMEM && server==saved);fail=0;
    CHECK(fvm_r9p_create(fs,&server)==FVM_R_OK);host.server=server;server->negotiated=1;
    node.target=target;node.kind=FVM_RFS_CALL;node.operation=7;
    CHECK(fvm_rfs_open(fs,node,FVM_RFS_READ|FVM_RFS_WRITE,&file)==FVM_R_OK);
    server->fids[0].active=server->fids[0].opened=1;server->fids[0].id=7;
    server->fids[0].node=node;server->fids[0].file=file;
    CHECK(fvm_rfs_write(fs,file,0,0,0,&used)==FVM_R_OK);
    CHECK(fvm_rdispatch_pump(d,1,&completed)==FVM_R_OK && completed==1 && host.ok);
    used=999;CHECK(fvm_r9p_poll(server,output,sizeof(output),&used)==FVM_R_PENDING && used==999);
    CHECK(!server->fids[0].opened && !server->fids[0].active);
    CHECK(fvm_r9p_destroy(&server)==FVM_R_OK && fvm_rfs_destroy(&fs)==FVM_R_OK);
    CHECK(fvm_rdispatch_unregister(d,target)==FVM_R_OK && fvm_rdispatch_destroy(&d)==FVM_R_OK);
    puts("PASS 9P allocation failure, running clunk invalidation and deferred native cleanup");return 0;
}
