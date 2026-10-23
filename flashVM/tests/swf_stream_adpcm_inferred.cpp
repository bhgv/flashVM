/* Unique stereo five-bit partial ADPCM works without counts; legacy APIs retain their policy. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
static int export_inferred_file(const char *input, const char *output)
{
    FILE *file = std::fopen(input, "rb");
    if (!file)
        return 2;
    Bytes bytes;
    int ch;
    while ((ch = std::fgetc(file)) != EOF)
        bytes.push_back((unsigned char)ch);
    std::fclose(file);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_load_memory(c, bytes.begin(), bytes.size()) == FVM_OK);
    FVM_Result result = fvm_swf_stream_sound_snapshot_open_pcm_inferred(c, 0, (size_t)-1, &snapshot);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    if (result != FVM_OK)
    {
        assert(!snapshot);
        return result == FVM_UNSUPPORTED ? 3 : 4;
    }
    FVM_SWFStreamPCMReader reader;
    assert(fvm_swf_stream_pcm_reader_init(&reader, snapshot) == FVM_OK);
    file = std::fopen(output, "wb");
    assert(file);
    short pcm[62];
    size_t written;
    FVM_SWFStreamPCMChunk chunk;
    for (;;)
    {
        chunk.struct_size = sizeof(chunk);
        result = fvm_swf_stream_pcm_reader_next(&reader, 31, pcm, 62, &chunk, &written);
        if (result == FVM_NOT_FOUND)
            break;
        assert(result == FVM_OK);
        assert(std::fwrite(pcm, sizeof(short), written, file) == written);
    }
    assert(!std::fclose(file));
    fvm_swf_stream_sound_snapshot_close(snapshot);
    return 0;
}
int main(int argc,char **argv)
{
 if(argc==3)return export_inferred_file(argv[1],argv[2]);assert(argc==1);
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 Bits bits=partial_bits(5,2,17);Bytes swf=swf_block(bits.bytes,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 FVM_SWFStreamSoundSnapshot *s=0;assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c,0,(size_t)-1,&s)==FVM_UNSUPPORTED&&!s);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&s)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);verify_partial(s,17,2,5);fvm_swf_stream_sound_snapshot_close(s);return 0;
}
