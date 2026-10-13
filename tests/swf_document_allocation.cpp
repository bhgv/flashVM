#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdlib>
#include <new>
static long allowance = -1;
/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
        throw std::bad_alloc();
    if (allowance > 0)
        --allowance;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete(void *p) throw()
{
    std::free(p);
}
void *operator new[](std::size_t size) throw(std::bad_alloc)
{
    return ::operator new(size);
}
void operator delete[](void *p) throw()
{
    ::operator delete(p);
}
void *operator new(std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete(void *p, const std::nothrow_t &) throw()
{
    ::operator delete(p);
}
void *operator new[](std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return 0;
    }
}
void operator delete[](void *p, const std::nothrow_t &) throw()
{
    ::operator delete[](p);
}
/* SetBackgroundColor is a replayable, graph-transported timeline control with stage-wide first win. */
#include "flashvm/movieclip.h"
#include "flashvm/stage.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
typedef fvm_detail::vector<unsigned char> Bytes;
static void word(Bytes &b,unsigned n){b.push_back((unsigned char)n);b.push_back((unsigned char)(n>>8));}
static void tag(Bytes &b,unsigned code,const Bytes &p){assert(p.size()<63);word(b,(code<<6)|(unsigned)p.size());for(size_t i=0;i<p.size();++i)b.push_back(p[i]);}
static Bytes movie(const Bytes &tags)
{
    Bytes b;b.push_back('F');b.push_back('W');b.push_back('S');b.push_back(8);unsigned long n=(unsigned long)tags.size()+14;for(unsigned i=0;i<4;++i){b.push_back((unsigned char)n);n>>=8;}b.push_back(8);b.push_back(0);word(b,12*256);word(b,2);for(size_t i=0;i<tags.size();++i)b.push_back(tags[i]);return b;
}
#include "flashvm/swf.h"
#include <cstring>
static Bytes document(unsigned long flags, const char *xml, bool duplicate = false)
{
    Bytes tags, attributes, metadata, empty;
    for (unsigned i = 0; i < 4; ++i) { attributes.push_back((unsigned char)flags); flags >>= 8; }
    tag(tags, 69, attributes);
    if (duplicate) tag(tags, 69, attributes);
    if (xml) { for (size_t i = 0; i <= std::strlen(xml); ++i) metadata.push_back((unsigned char)xml[i]); tag(tags, 77, metadata); }
    tag(tags, 1, empty); tag(tags, 1, empty); tag(tags, 0, empty);
    return movie(tags);
}
static void inspect_metadata(FVM_Context *c, FVM_Object *clip, const char *xml)
{
    FVM_SWFDocumentInfo info; info.struct_size = sizeof(info);
    assert(fvm_movieclip_swf_document_info(c, clip, &info) == FVM_OK);
    assert(info.file_attributes_present && info.file_attributes == 16);
    assert(info.metadata.type == FVM_VALUE_STRING && fvm_value_string_size(&info.metadata) == std::strlen(xml));
    assert(!std::memcmp(info.metadata.string, xml, std::strlen(xml)));
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    Bytes source = document(16, "<test/>");
    bool succeeded = false;
    for (long failure = 0; failure < 4096 && !succeeded; ++failure) {
        FVM_Object *clip = 0;
        allowance = failure;
        FVM_Result r = fvm_movieclip_load_swf(c, source.begin(), source.size(), "root", &clip);
        allowance = -1;
        assert(r == FVM_OK || r == FVM_OUT_OF_MEMORY);
        assert(r == FVM_OK || !clip);
        succeeded = r == FVM_OK;
        if (succeeded) {
            assert(fvm_gc_pin(c, clip) == FVM_OK);
            inspect_metadata(c, clip, "<test/>");
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (succeeded) {
            inspect_metadata(c, clip, "<test/>");
            assert(fvm_gc_unpin(c, clip) == FVM_OK);
        }
    }
    assert(succeeded);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
