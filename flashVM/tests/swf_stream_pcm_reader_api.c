/* C89 header/link contract for the native prepared streaming PCM reader. */
#include "flashvm/swf_stream_pcm.h"
#include "flashvm/swf_stream_convert.h"
/* Invalid input must remain callable without initializing a VM or audio device. */
int main(void)
{
    FVM_SWFStreamConverter converter;
    FVM_SWFStreamPCMReader reader;
    FVM_SWFStreamPCMChunk chunk;
    size_t written = 999;
    if (fvm_swf_stream_converter_init(&converter, 0, 48000, 2) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_converter_next(0, 1, 0, 0, 0, 0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_converter_seek(0, 0, 0) != FVM_INVALID_ARGUMENT) return 1;
    chunk.struct_size = sizeof(chunk);
    if (fvm_swf_stream_pcm_reader_init(&reader, 0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_pcm_reader_next(0, 1, 0, 0, &chunk, &written) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_pcm_reader_seek(0, 0, 0) != FVM_INVALID_ARGUMENT || written != 999)
        return 1;
    return 0;
}
