/* C89 document inspection reports neutral flags and preserves output on errors. */
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include <assert.h>
int main(void)
{
    static const unsigned char swf[] = {
        'F', 'W', 'S', 8, 24, 0, 0, 0, 8, 0, 0, 12, 1, 0,
        0x44, 0x11, 0, 0, 0, 0, 0x40, 0, 0, 0
    };
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    FVM_SWFDocumentInfo info;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_create(c, "plain", 1, &clip) == FVM_OK);
    info.struct_size = sizeof(info);
    assert(fvm_movieclip_swf_document_info(c, clip, &info) == FVM_OK);
    assert(!info.file_attributes_present && !info.file_attributes && info.metadata.type == FVM_VALUE_UNDEFINED);
    assert(fvm_movieclip_load_swf(c, swf, sizeof(swf), "root", &clip) == FVM_OK);
    assert(fvm_movieclip_swf_document_info(c, clip, &info) == FVM_OK);
    assert(info.file_attributes_present && !info.file_attributes && info.metadata.type == FVM_VALUE_UNDEFINED);
    info.struct_size = 0;
    info.file_attributes = 999;
    assert(fvm_movieclip_swf_document_info(c, clip, &info) == FVM_INVALID_ARGUMENT);
    assert(info.file_attributes == 999);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
