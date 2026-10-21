/* Independent integer grid oracle across tiny blocks, gaps and empty markers; no float oracle. */
#define main stream_fixture_main
#include "swf_stream_sound.cpp"
#undef main
#include "flashvm/swf_stream_convert.h"
/* Signed division toward zero, independent of pre-C++11 negative division conventions. */
static long quotient(long n, long d) { return n < 0 ? -((-n)/d) : n/d; }
/* Compute signed interpolation with bounded integer long division, independently of doubles. */
static long reference(long a,long b,unsigned long phase,unsigned long rate)
{
    long delta=b-a;unsigned long magnitude=(unsigned long)(delta<0?-delta:delta);
    unsigned long q=0,r=0;
    for(unsigned long bit=32768UL;bit;bit>>=1) {
        unsigned long sum=r*2+((magnitude&bit)?phase:0);
        q=q*2+sum/rate;r=sum%rate;
    }
    long value=delta<0?a-(long)q:a+(long)q;
    if(r && delta<0 && value>0)--value;
    if(r && delta>=0 && value<0)++value;
    return value;
}
/* Compare continuous rational sample positions and legacy clamping and continuous neighbors with one-frame/native chunks. */
int main()
{
    FVM_Config config;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    const unsigned long rates[]={5512,11025,22050,44100,48000,96000,192000};
    const unsigned long source_rates[]={5512,11025,22050,44100};
    for(unsigned source_index=0;source_index<4;++source_index) for(unsigned source_channels=1;source_channels<=2;++source_channels) {
        unsigned long source_rate=source_rates[source_index];
        Bytes tags,empty;
        tag(tags,45,header(0x32|(source_index<<2)|(source_channels-1)));
        short samples[14];
        for(unsigned i=0;i<14;++i)samples[i]=(short)(i%3==0?-32768:i%3==1?32767:-1234);
        const unsigned sizes[]={1,0,2,1,3};size_t at=0;
        for(unsigned b=0;b<5;++b) {
            Bytes data;
            for(unsigned i=0;i<sizes[b]*source_channels;++i)word(data,(unsigned short)samples[at*source_channels+i]);
            tag(tags,19,data);if(b<4)tag(tags,1,empty);at+=sizes[b];
        }
        load(c,stream_movie(tags,8,5));FVM_SWFStreamSoundSnapshot *snapshot=0;
        assert(fvm_swf_stream_sound_snapshot_open(c,0,&snapshot)==FVM_OK);
        load(c,movie(empty));
        for(unsigned rate_index=0;rate_index<7;++rate_index)for(unsigned channels=1;channels<=2;++channels)for(unsigned continuous=0;continuous<2;++continuous)for(unsigned chunk_size=1;chunk_size<=11;chunk_size+=10) {
            unsigned long rate=rates[rate_index];
            FVM_SWFStreamConverter state;
            assert((continuous ? fvm_swf_stream_converter_init_continuous(&state,snapshot,rate,channels) : fvm_swf_stream_converter_init(&state,snapshot,rate,channels))==FVM_OK);
            FVM_SWFStreamConvertedChunk chunk;std::memset(&chunk,0,sizeof(chunk));chunk.struct_size=sizeof(chunk);
            short output[22];size_t written=999;
            FVM_SWFStreamConverter saved=state;FVM_SWFStreamConvertedChunk old=chunk;output[0]=42;
            assert(fvm_swf_stream_converter_next(&state,chunk_size,output,0,&chunk,&written)==FVM_INVALID_ARGUMENT);
            assert(!std::memcmp(&state,&saved,sizeof(state)) && !std::memcmp(&chunk,&old,sizeof(chunk)) && output[0]==42 && written==999);
            size_t ordinal=0,base=0,number=0,total_output=0;
            while(ordinal<5) {
                assert(fvm_swf_stream_converter_next(&state,chunk_size,output,22,&chunk,&written)==FVM_OK);
                assert(chunk.block_index==ordinal && chunk.frame==ordinal+1 && chunk.sample_rate==rate && chunk.channels==channels && written==chunk.frames*channels);
                assert(((chunk.flags&FVM_STREAM_CONVERT_EMPTY)!=0) == (sizes[ordinal]==0));
                for(size_t i=0;i<chunk.frames;++i,++number) {
                    /* Small fixture keeps products within long; grid derives from absolute output index. */
                    size_t absolute=(number*source_rate)/rate;
                    unsigned long phase=(number*source_rate)%rate;
                    assert(absolute>=base && absolute<base+sizes[ordinal]);
                    size_t limit=continuous?7:base+sizes[ordinal];
                    size_t right=absolute+1<limit?absolute+1:absolute;
                    for(unsigned channel=0;channel<channels;++channel) {
                        long a,b;
                        if(channels==1 && source_channels==2) {
                            a=quotient((long)samples[absolute*2]+samples[absolute*2+1],2);
                            b=quotient((long)samples[right*2]+samples[right*2+1],2);
                        } else {
                            unsigned index=source_channels==1?0:channel;
                            a=samples[absolute*source_channels+index];b=samples[right*source_channels+index];
                        }
                        long expected=reference(a,b,phase,rate);
                        assert(output[i*channels+channel]==expected);
                    }
                    ++total_output;
                }
                if(chunk.flags&FVM_STREAM_CONVERT_END) { base+=sizes[ordinal];++ordinal; }
            }
            assert(total_output==(7*rate+source_rate-1)/source_rate);
            saved=state;old=chunk;written=999;
            assert(fvm_swf_stream_converter_next(&state,1,output,22,&chunk,&written)==FVM_NOT_FOUND && written==999);
            assert(!std::memcmp(&state,&saved,sizeof(state)) && !std::memcmp(&chunk,&old,sizeof(chunk)));
            assert(fvm_swf_stream_converter_seek(&state,2,1)==FVM_OK && state.phase==0 && state.continuous==(int)continuous);
            assert(fvm_swf_stream_converter_next(&state,1,output,22,&chunk,&written)==FVM_OK && chunk.block_index==2 && chunk.first_source==1 && written==channels);
            assert(fvm_swf_stream_converter_seek(&state,0,0)==FVM_OK && state.phase==0);
            assert(fvm_swf_stream_converter_seek(&state,5,1)==FVM_INVALID_ARGUMENT);
        }
        fvm_swf_stream_sound_snapshot_close(snapshot);
    }
    Bytes tags,data,empty;tag(tags,45,header(12));for(unsigned i=0;i<2000;++i)data.push_back(128);
    tag(tags,19,data);load(c,movie(tags));FVM_SWFStreamSoundSnapshot *large=0;
    assert(fvm_swf_stream_sound_snapshot_open(c,0,&large)==FVM_OK);
    FVM_SWFStreamConverter converter,saved;std::memset(&converter,0xa5,sizeof(converter));saved=converter;
    assert(fvm_swf_stream_converter_init(&converter,large,1,1)==FVM_INVALID_ARGUMENT && !std::memcmp(&converter,&saved,sizeof(saved)));
    assert(fvm_swf_stream_converter_init(&converter,large,44100,1)==FVM_OK);
    FVM_SWFStreamConvertedChunk chunk;chunk.struct_size=sizeof(chunk);short buffer[1024];size_t written;
    assert(fvm_swf_stream_converter_next(&converter,(size_t)-1,buffer,1024,&chunk,&written)==FVM_OK && written==1024 && chunk.frames==1024);
    assert(fvm_swf_stream_converter_next(&converter,(size_t)-1,buffer,1024,&chunk,&written)==FVM_OK && written==976 && (chunk.flags&FVM_STREAM_CONVERT_END));
    fvm_swf_stream_sound_snapshot_close(large);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
