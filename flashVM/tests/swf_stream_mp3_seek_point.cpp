/* Signed authored targets across all six MPEG vectors, two latencies and exact bounds. */
#define main original_decode_main
#include "swf_stream_mp3_decode.cpp"
#undef main
#include <cstring>
#include <climits>
/* Build valid movie lengths/frame counts while changing only later signed seek metadata. */
static Bytes seek_movie(const mp3_fixture::Fixture &fixture,unsigned latency,int adjustment)
{
    Bytes swf=stream_movie(fixture,latency);
    size_t at=14;unsigned blocks=0;
    while(at<swf.size())
    {
        unsigned header=swf[at]|((unsigned)swf[at+1]<<8);at+=2;
        unsigned code=header>>6;size_t length=header&63;
        if(length==63){length=0;for(unsigned b=0;b<4;++b)length|=(size_t)swf[at+b]<<(8*b);at+=4;}
        if(code==19 && ++blocks==3){unsigned value=(unsigned)adjustment&65535;swf[at+2]=(unsigned char)value;swf[at+3]=(unsigned char)(value>>8);}
        at+=length;
    }
    return swf;
}
/* Independently sum public normalized block lengths rather than reuse private offset search. */
static size_t absolute_point(FVM_SWFStreamSoundSnapshot *source,const FVM_SWFStreamSoundSeekPoint &point)
{
    size_t result=point.first;
    for(size_t i=0;i<point.block_index;++i){FVM_SWFStreamSoundBlock b;b.struct_size=sizeof(b);assert(fvm_swf_stream_sound_snapshot_block_at(source,i,&b)==FVM_OK);result+=b.sample_count;}
    return result;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    for(size_t f=0;f<sizeof(mp3_fixture::fixtures)/sizeof(mp3_fixture::fixtures[0]);++f)
    {
        const mp3_fixture::Fixture &fixture=mp3_fixture::fixtures[f];
        FVM_SWFMP3Frame first;assert(fvm_swf_mp3_probe(fixture.data,fixture.size,&first)==FVM_OK);
        for(unsigned latency=0;latency<=1105;latency+=1105)
        {
            int offsets[]={0,37,-37,-32768,32767,-(int)first.samples,(int)(fixture.frames-latency-first.samples),(int)(fixture.frames-latency-first.samples+1)};
            for(size_t k=0;k<sizeof(offsets)/sizeof(offsets[0]);++k)
            {
                Bytes swf=seek_movie(fixture,latency,offsets[k]);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
                FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source)==FVM_OK);
                FVM_SWFStreamSoundSeekPoint p;std::memset(&p,0,sizeof(p));p.struct_size=sizeof(p);
                assert(fvm_swf_stream_sound_snapshot_seek_point(source,1,&p)==FVM_OK && p.source_frame==1 && p.mp3_aligned && absolute_point(source,p)==0);
                long expected=(long)first.samples+offsets[k];
                FVM_SWFStreamSoundSeekPoint old=p;
                FVM_Result result=fvm_swf_stream_sound_snapshot_seek_point(source,3,&p);
                if(expected<0 || (size_t)expected>fixture.frames-latency){assert(result==FVM_INVALID_ARGUMENT && std::memcmp(&p,&old,sizeof(p))==0);}
                else{assert(result==FVM_OK && p.source_frame==3 && absolute_point(source,p)==(size_t)expected);}
                old=p;assert(fvm_swf_stream_sound_snapshot_seek_point(source,0,&p)==FVM_INVALID_ARGUMENT && std::memcmp(&p,&old,sizeof(p))==0);
                assert(fvm_swf_stream_sound_snapshot_seek_point(source,UINT_MAX,&p)==FVM_OK && p.source_frame==FVM_SWF_STREAM_SEEK_EOF_FRAME && p.first==0 && absolute_point(source,p)==fixture.frames-latency);
                --p.struct_size;old=p;assert(fvm_swf_stream_sound_snapshot_seek_point(source,1,&p)==FVM_INVALID_ARGUMENT && std::memcmp(&p,&old,sizeof(p))==0);
                fvm_swf_stream_sound_snapshot_close(source);
            }
        }
        Bytes swf=stream_movie(fixture,0);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
        FVM_SWFStreamSoundSnapshot *encoded=0;assert(fvm_swf_stream_sound_snapshot_open(c,0,&encoded)==FVM_OK);
        FVM_SWFStreamSoundSeekPoint p;std::memset(&p,0,sizeof(p));p.struct_size=sizeof(p);FVM_SWFStreamSoundSeekPoint old=p;
        assert(fvm_swf_stream_sound_snapshot_seek_point(encoded,3,&p)==FVM_UNSUPPORTED && std::memcmp(&p,&old,sizeof(p))==0);
        fvm_swf_stream_sound_snapshot_close(encoded);
    }
    /* Ordinary PCM keeps authored block-start semantics, including empty blocks and gaps. */
    Bytes tags,head,data,empty;head.push_back(14);head.push_back(0x3e);word(head,1);
    tag(tags,45,head);word(data,100);word(data,200);tag(tags,19,data);tag(tags,1,empty);tag(tags,1,empty);tag(tags,19,empty);
    Bytes swf=movie(tags);swf[12]=3;assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    for(unsigned prepare=0;prepare<2;++prepare)
    {
        FVM_SWFStreamSoundSnapshot *source=0;
        assert((prepare?fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source):fvm_swf_stream_sound_snapshot_open(c,0,&source))==FVM_OK);
        FVM_SWFStreamSoundSeekPoint p;p.struct_size=sizeof(p);
        assert(fvm_swf_stream_sound_snapshot_seek_point(source,1,&p)==FVM_OK && !p.mp3_aligned && !p.block_index && !p.first && p.source_frame==1);
        assert(fvm_swf_stream_sound_snapshot_seek_point(source,2,&p)==FVM_OK && !p.mp3_aligned && p.block_index==1 && !p.first && p.source_frame==3);
        assert(fvm_swf_stream_sound_snapshot_seek_point(source,4,&p)==FVM_OK && !p.mp3_aligned && p.block_index==2 && !p.first && p.source_frame==FVM_SWF_STREAM_SEEK_EOF_FRAME);
        fvm_swf_stream_sound_snapshot_close(source);
    }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
