/* Production ARM64 arena against controlled SDK calls; not native Apple OS evidence. */
#include "flashvm/flashvm.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#if !defined(_WIN32)
#include <pthread.h>
#define FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_ARM64 1
#define FVM_MAC_CODE_SUPPORTED 1
#define FVM_MAC_CODE_ARM64_SDK_HPP
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANON 4096
#define MAP_JIT 2048
#define MAP_FAILED ((void *)-1)
static unsigned failure, allocations, deny_allocation, maps, writes, caches;
static void *region;
static size_t region_size;
static bool protection = true;
static pthread_key_t writable_key;
static int fixture_supported(void) { return protection; }
static bool writable(void) { return pthread_getspecific(writable_key) != 0; }
static void *fixture_allocate(size_t size)
{
    assert(!writable());
    if (++allocations == deny_allocation) return 0;
    return std::malloc(size);
}
static void fixture_free(void *value) { assert(!writable());std::free(value); }
static void *fixture_mmap(void *base,size_t size,int prot,int flags,int fd,long offset)
{
    assert(!writable() && !region && !base && prot==(PROT_READ|PROT_WRITE|PROT_EXEC));
    assert(flags==(MAP_PRIVATE|MAP_ANON|MAP_JIT) && fd==-1 && !offset);
    ++maps;
    if(failure==1 || failure==2){errno=failure==1?ENOMEM:EACCES;return MAP_FAILED;}
    region_size=size;region=std::calloc(size,1);assert(region);return region;
}
static void fixture_cache(void *memory,size_t size)
{
    assert(!writable() && region && (unsigned char *)memory>=(unsigned char *)region);
    assert((unsigned char *)memory+size<=(unsigned char *)region+region_size);
    ++caches;
}
static int (*allowed)(void *);
static int fixture_write(int (*callback)(void *),void *opaque)
{
    assert(!writable() && callback==allowed);
    ++writes;
    if(failure==3)return FVM_ERROR;
    assert(!pthread_setspecific(writable_key,(void *)1));
    int result=callback(opaque);
    assert(!pthread_setspecific(writable_key,0));
    return result;
}
#define FVM_MAC_JIT_ALLOW_CALLBACK(callback) static int register_callback = (allowed=callback,0)
#define FVM_MAC_JIT_ALLOCATE(size) fixture_allocate(size)
#define FVM_MAC_JIT_FREE(memory) fixture_free(memory)
#define mmap fixture_mmap
#define pthread_jit_write_protect_supported_np fixture_supported
#define pthread_jit_write_with_callback_np fixture_write
#define sys_icache_invalidate fixture_cache
#define fvm_mac_arm64_code_supported fixture_code_supported
#define fvm_mac_arm64_code_create fixture_code_create
#define fvm_mac_arm64_code_release fixture_code_release
#include "../ports/mac/code_arm64.cpp"
#undef mmap
#undef pthread_jit_write_protect_supported_np
#undef pthread_jit_write_with_callback_np
#undef sys_icache_invalidate
#undef fvm_mac_arm64_code_supported
#undef fvm_mac_arm64_code_create
#undef fvm_mac_arm64_code_release
/* Each worker keeps a slot live while another allocation/release exercises the shared list. */
static void *worker(void *opaque)
{
    unsigned char bytes[37];std::memset(bytes,(int)(size_t)opaque,sizeof(bytes));
    for(unsigned i=0;i<200;++i)
    {
        void *memory=0;size_t cookie=0;
        assert(fixture_code_create(bytes,sizeof(bytes),&memory,&cookie)==FVM_OK);
        assert(cookie==48 && !std::memcmp(memory,bytes,sizeof(bytes)));
        fixture_code_release(memory,cookie);
    }
    return 0;
}
#endif
int main()
{
#if !defined(_WIN32)
    assert(!pthread_key_create(&writable_key,0));
    unsigned char bytes[33];std::memset(bytes,0xa5,sizeof(bytes));
    void *out=(void *)1;size_t cookie=77;
    assert(fixture_code_create(0,1,&out,&cookie)==FVM_INVALID_ARGUMENT);
    assert(fixture_code_create(bytes,0,&out,&cookie)==FVM_INVALID_ARGUMENT);
    assert(fixture_code_create(bytes,1,0,&cookie)==FVM_INVALID_ARGUMENT);
    assert(fixture_code_create(bytes,1,&out,0)==FVM_INVALID_ARGUMENT);
    assert(!maps && !allocations && !writes);
    protection=false;
    assert(!fixture_code_supported());
    assert(fixture_code_create(bytes,1,&out,&cookie)==FVM_UNSUPPORTED && !maps);
    protection=true;
    assert(fixture_code_create(bytes,(size_t)-1,&out,&cookie)==FVM_UNSUPPORTED && !maps);
    deny_allocation=allocations+1;
    assert(fixture_code_create(bytes,1,&out,&cookie)==FVM_OUT_OF_MEMORY && !maps);
    deny_allocation=0;
    for(failure=1;failure<=2;++failure)
    {
        assert(fixture_code_create(bytes,1,&out,&cookie)==(failure==1?FVM_OUT_OF_MEMORY:FVM_UNSUPPORTED));
        assert(!region && !arena_slots && !arena_memory && out==(void *)1 && cookie==77);
    }
    failure=0;
    /* Fail the split metadata allocation after the sole region was reserved. */
    deny_allocation=allocations+2;
    assert(fixture_code_create(bytes,1,&out,&cookie)==FVM_OUT_OF_MEMORY);
    assert(region && arena_slots && !arena_slots->used && !arena_slots->next);
    assert(out==(void *)1 && cookie==77 && !writes);
    deny_allocation=0;
    failure=3;
    assert(fixture_code_create(bytes,1,&out,&cookie)==FVM_UNSUPPORTED);
    assert(!arena_slots->used && !arena_slots->next && !active_write && !caches);
    assert(out==(void *)1 && cookie==77);
    failure=0;
    unsigned expected_maps=maps;
    assert(fixture_code_create(bytes,sizeof(bytes),&out,&cookie)==FVM_OK && cookie==48);
    assert(out==region && !std::memcmp(out,bytes,sizeof(bytes)) && caches==1 && !active_write);
    assert(write_code(0)==FVM_INVALID_ARGUMENT && write_code((void *)1)==FVM_INVALID_ARGUMENT);
    /* Slot ownership and wrong release cookies cannot invalidate a second live body. */
    void *second=0;size_t second_cookie=0;
    assert(fixture_code_create(bytes,1,&second,&second_cookie)==FVM_OK && second!=out && second_cookie==16);
    fixture_code_release(out,cookie+16);assert(arena_slots->used);
    fixture_code_release((unsigned char *)out+1,cookie);assert(arena_slots->used);
    fixture_code_release(out,cookie);fixture_code_release(out,cookie);
    assert(!std::memcmp(second,bytes,1));
    fixture_code_release(second,second_cookie);
    assert(!arena_slots->used && !arena_slots->next && arena_slots->size==FVM_MAC_JIT_ARENA_BYTES);
    /* Exact-capacity request needs no split metadata and exhaustion leaves outputs unchanged. */
    void *whole=0;size_t whole_cookie=0;
    deny_allocation=allocations+1;
    /* Source bytes must cover the entire copy, not the short pattern above. */
    unsigned char *large=(unsigned char *)std::calloc(FVM_MAC_JIT_ARENA_BYTES,1);assert(large);
    assert(fixture_code_create(large,FVM_MAC_JIT_ARENA_BYTES,&whole,&whole_cookie)==FVM_OK);
    assert(whole==region && whole_cookie==FVM_MAC_JIT_ARENA_BYTES);
    assert(fixture_code_create(bytes,1,&out,&cookie)==FVM_OUT_OF_MEMORY);
    fixture_code_release(whole,whole_cookie);std::free(large);deny_allocation=0;
    pthread_t threads[4];
    for(size_t i=0;i<4;++i)assert(!pthread_create(&threads[i],0,worker,(void *)(i+1)));
    for(unsigned i=0;i<4;++i)assert(!pthread_join(threads[i],0));
    assert(maps==expected_maps && !active_write && !arena_slots->used && !arena_slots->next);
    assert(arena_slots->size==FVM_MAC_JIT_ARENA_BYTES && !writable());
    fixture_code_release(0,0);
    /* Test-only teardown; production deliberately keeps its one process-lifetime mapping. */
    std::free(arena_slots);arena_slots=0;std::free(region);region=0;arena_memory=0;
    assert(!pthread_key_delete(writable_key));
#else
    return 77;
#endif
    return 0;
}
