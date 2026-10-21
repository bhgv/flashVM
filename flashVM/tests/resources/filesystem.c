/* Public C89 filesystem contracts, with private exhaustion/allocation fault injection. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int fail_alloc;
static void *test_calloc(size_t n, size_t size) { return fail_alloc ? 0 : calloc(n, size); }
#define calloc test_calloc
#include "../../src/protocol/rfs.c"
#undef calloc
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"rfs:%d %s\n",__LINE__,#x); return 1; } } while(0)
typedef struct Host { int calls; FVM_RFS *fs; FVM_RFSFile file; FVM_RResult close_result; } Host;
static FVM_RResult handler(const FVM_RRequest *q, void *out, size_t cap, size_t *used, void *data) {
    Host *host = (Host *)data;
    ++host->calls;
    if (host->fs) host->close_result = fvm_rfs_close(host->fs, host->file);
    if (q->size > cap) return FVM_R_LIMIT;
    if (q->size) memcpy(out,q->data,q->size);
    *used = q->size; return FVM_R_OK;
}
int main(void) {
    FVM_RDispatch *d = 0;
    FVM_RFS *fs = 0, *other = 0, *saved;
    FVM_ROperation ops[2] = {{4294967295UL,FVM_R_CONTROL},{7,FVM_R_READ}};
    FVM_RHandle resource, replacement;
    FVM_RFSNode root;
    FVM_RFSEntry entry, object, opdir, call, event, path, other_call, untouched;
    FVM_RFSFile dir, file, event_file, second_event, old, many[FVM_RFS_FILES];
    Host host;
    unsigned char bytes[FVM_RFS_BUFFER], encoded[FVM_RFS_BUFFER];
    size_t used, total, completed, i, consumed;
    unsigned long sequence, request_id;
    FVM_SBinding slots[1]; FVM_SBindings bindings; FVM_RReply reply; FVM_REvent received;
    memset(&host,0,sizeof(host)); memset(&root,0,sizeof(root));
    CHECK(fvm_rdispatch_create(0,&d)==FVM_R_OK);
    CHECK(fvm_rdispatch_register(d,"/scene/root",ops,2,handler,&host,&resource)==FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d,resource,1,FVM_R_RIGHTS)==FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d,resource,2,FVM_R_READ)==FVM_R_OK);
    saved = fs; fail_alloc = 1;
    CHECK(fvm_rfs_create(d,1,&fs)==FVM_R_NOMEM && fs==saved); fail_alloc = 0;
    CHECK(fvm_rfs_create(d,1,&fs)==FVM_R_OK && fvm_rfs_create(d,2,&other)==FVM_R_OK);
    CHECK(fvm_rfs_stat(fs,root,&entry)==FVM_R_OK && entry.directory && !entry.qid_low);
    CHECK(fvm_rfs_open(fs,root,FVM_RFS_READ,&dir)==FVM_R_OK);
    CHECK(fvm_rfs_readdir(fs,dir,0,&object)==FVM_R_OK && !strcmp(object.name,"1.1"));
    CHECK(fvm_rfs_readdir(fs,dir,1,&entry)==FVM_R_PENDING);
    untouched = entry; CHECK(fvm_rfs_walk(fs,root,"01.1",&entry)==FVM_R_UNKNOWN && !memcmp(&entry,&untouched,sizeof(entry)));
    CHECK(fvm_rfs_walk(fs,root,"1.1/ops",&entry)==FVM_R_UNKNOWN);
    CHECK(fvm_rfs_walk(fs,root,"..",&entry)==FVM_R_OK && entry.node.kind==FVM_RFS_ROOT);
    CHECK(fvm_rfs_walk(fs,object.node,"ops",&opdir)==FVM_R_OK);
    CHECK(fvm_rfs_walk(fs,opdir.node,"7",&call)==FVM_R_OK);
    CHECK(fvm_rfs_walk(other,opdir.node,"7",&other_call)==FVM_R_OK && other_call.qid_low==call.qid_low && other_call.qid_high==call.qid_high);
    CHECK(fvm_rfs_walk(other,opdir.node,"4294967295",&entry)==FVM_R_DENIED);
    CHECK(fvm_rfs_walk(fs,object.node,"path",&path)==FVM_R_OK);
    CHECK(fvm_rfs_open(fs,path.node,FVM_RFS_READ,&file)==FVM_R_OK);
    CHECK(fvm_rfs_read(fs,file,7,bytes,sizeof(bytes),&used)==FVM_R_OK && used==4 && !memcmp(bytes,"root",4));
    CHECK(fvm_rfs_read(fs,file,FVM_SRECORD_U32,bytes,sizeof(bytes),&used)==FVM_R_OK && !used);
    old=file; CHECK(fvm_rfs_close(fs,file)==FVM_R_OK && fvm_rfs_close(fs,old)==FVM_R_UNKNOWN);
    CHECK(fvm_rfs_open(fs,call.node,FVM_RFS_READ,&file)==FVM_R_DENIED);
    CHECK(fvm_rfs_open(fs,call.node,FVM_RFS_READ|FVM_RFS_WRITE,&file)==FVM_R_OK);
    used=123; CHECK(fvm_rfs_read(fs,file,0,bytes,sizeof(bytes),&used)==FVM_R_PENDING && used==123);
    CHECK(fvm_rfs_write(fs,file,1,"abc",3,&used)==FVM_R_INVALID && used==123);
    CHECK(fvm_rfs_write(fs,file,0,"abc",3,&used)==FVM_R_OK && used==3 && host.calls==0);
    CHECK(fvm_rfs_write(fs,file,0,"abc",3,&used)==FVM_R_CONFLICT);
    CHECK(fvm_rfs_read(fs,file,0,bytes,sizeof(bytes),&used)==FVM_R_PENDING);
    host.fs=fs; host.file=file;
    CHECK(fvm_rdispatch_pump(d,1,&completed)==FVM_R_OK && completed==1 && host.calls==1 && host.close_result==FVM_R_BUSY);
    host.fs=0; total=0;
    do { CHECK(fvm_rfs_read(fs,file,(unsigned long)total,bytes+total,1,&used)==FVM_R_OK); total+=used; } while(used);
    CHECK(fvm_sbindings_init(&bindings,slots,1)==FVM_S_OK);
    CHECK(fvm_sbindings_apply(&bindings,bytes,total,&consumed)==FVM_S_OK);
    CHECK(fvm_rreply_record_read(&bindings,bytes+consumed,total-consumed,&request_id,&reply,&used)==FVM_S_OK && request_id==1 && reply.result==FVM_R_OK && reply.size==3 && !memcmp(reply.data,"abc",3));
    CHECK(fvm_rfs_close(fs,file)==FVM_R_OK);
    CHECK(fvm_rfs_open(fs,call.node,FVM_RFS_READ|FVM_RFS_WRITE,&file)==FVM_R_OK);
    CHECK(fvm_rfs_write(fs,file,0,0,0,&used)==FVM_R_OK && !used);
    CHECK(fvm_rfs_close(fs,file)==FVM_R_OK);
    CHECK(fvm_rdispatch_pump(d,1,&completed)==FVM_R_OK && !completed && host.calls==1);
    CHECK(fvm_rfs_walk(fs,object.node,"events",&event)==FVM_R_OK);
    CHECK(fvm_rfs_open(fs,event.node,FVM_RFS_READ,&event_file)==FVM_R_OK);
    CHECK(fvm_rfs_open(fs,event.node,FVM_RFS_READ,&second_event)==FVM_R_OK);
    CHECK(fvm_rfs_read(fs,event_file,0,bytes,sizeof(bytes),&used)==FVM_R_PENDING);
    CHECK(fvm_rdispatch_publish(d,resource,4,"data",4,&sequence)==FVM_R_OK);
    CHECK(fvm_rfs_read(fs,event_file,0,bytes,1,&used)==FVM_R_OK && used==1);
    CHECK(fvm_rfs_read(fs,second_event,0,encoded,sizeof(encoded),&total)==FVM_R_OK);
    CHECK(fvm_sbindings_init(&bindings,slots,1)==FVM_S_OK);
    CHECK(fvm_sbindings_apply(&bindings,encoded,total,&consumed)==FVM_S_OK);
    CHECK(fvm_revent_record_read(&bindings,encoded+consumed,total-consumed,&received,&used)==FVM_S_OK && received.sequence==sequence && received.size==4 && !received.missed);
    CHECK(fvm_rdispatch_grant(d,resource,1,FVM_R_READ)==FVM_R_OK);
    CHECK(fvm_rfs_readdir(fs,dir,0,&entry)==FVM_R_CONFLICT);
    CHECK(fvm_rdispatch_grant(d,resource,1,FVM_R_RIGHTS)==FVM_R_OK);
    used=99; CHECK(fvm_rfs_read(fs,event_file,1,bytes,sizeof(bytes),&used)==FVM_R_UNKNOWN && used==99);
    CHECK(fvm_rfs_close(fs,event_file)==FVM_R_OK && fvm_rfs_close(fs,second_event)==FVM_R_OK);
    CHECK(fvm_rfs_stat(fs,call.node,&other_call)==FVM_R_OK && other_call.qid_low==call.qid_low);
    CHECK(fvm_rfs_close(fs,dir)==FVM_R_OK);
    for(i=0;i<FVM_RFS_FILES;++i) CHECK(fvm_rfs_open(fs,path.node,FVM_RFS_READ,many+i)==FVM_R_OK);
    CHECK(fvm_rfs_open(fs,path.node,FVM_RFS_READ,&file)==FVM_R_LIMIT && fvm_rfs_destroy(&fs)==FVM_R_BUSY);
    for(i=0;i<FVM_RFS_FILES;++i) CHECK(fvm_rfs_close(fs,many[i])==FVM_R_OK);
    fs->files[0].generation=FVM_SRECORD_U32;
    CHECK(fvm_rfs_open(fs,path.node,FVM_RFS_READ,&file)==FVM_R_OK && file.id==1);
    CHECK(fvm_rfs_close(fs,file)==FVM_R_OK && !fs->files[0].generation);
    CHECK(fvm_rfs_open(fs,call.node,FVM_RFS_READ|FVM_RFS_WRITE,&file)==FVM_R_OK && file.id==2);
    fs->next=FVM_SRECORD_U32; used=88;
    CHECK(fvm_rfs_write(fs,file,0,0,0,&used)==FVM_R_LIMIT && used==88 && !fs->files[1].written);
    CHECK(fvm_rfs_close(fs,file)==FVM_R_OK);
    CHECK(fvm_rdispatch_unregister(d,resource)==FVM_R_OK);
    CHECK(fvm_rdispatch_register(d,"/scene/root",ops,2,handler,&host,&replacement)==FVM_R_OK);
    CHECK(fvm_rdispatch_grant(d,replacement,1,FVM_R_RIGHTS)==FVM_R_OK);
    CHECK(fvm_rfs_stat(fs,path.node,&entry)==FVM_R_UNKNOWN);
    memset(&old,0,sizeof(old)); CHECK(fvm_rfs_readdir(other,old,0,&entry)==FVM_R_UNKNOWN);
    CHECK(fvm_rfs_destroy(&fs)==FVM_R_OK && !fs && fvm_rfs_destroy(&other)==FVM_R_OK);
    CHECK(fvm_rdispatch_unregister(d,replacement)==FVM_R_OK && fvm_rdispatch_destroy(&d)==FVM_R_OK);
    puts("resource filesystem: OK"); return 0;
}
