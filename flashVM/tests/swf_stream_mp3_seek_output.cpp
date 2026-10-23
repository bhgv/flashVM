/* Aligned seek transactions preserve pending owners through failures, reentry and exceptions. */
#define main original_decode_main
#include "swf_stream_mp3_decode.cpp"
#undef main
#include "flashvm/swf_stream_output.h"
#include <cstring>
struct SeekQueue { size_t queued,clears,pauses;int reject,pause_fail,clear_fail,throws;FVM_SWFStreamOutput *output; };
static FVM_Result seek_write(void *raw,const short *,size_t frames){SeekQueue *q=(SeekQueue*)raw;if(q->reject)return FVM_ERROR;q->queued+=frames;return FVM_OK;}
static FVM_Result seek_count(void *raw,size_t *n){*n=((SeekQueue*)raw)->queued;return FVM_OK;}
static FVM_Result seek_pause(void *raw,int){SeekQueue *q=(SeekQueue*)raw;++q->pauses;return q->pause_fail?FVM_ERROR:FVM_OK;}
static FVM_Result seek_clear(void *raw){SeekQueue *q=(SeekQueue*)raw;++q->clears;if(q->output)assert(fvm_swf_stream_output_seek_frame_aligned(q->output,1)==FVM_INVALID_ARGUMENT);if(q->throws)throw 1;if(q->clear_fail)return FVM_ERROR;q->queued=0;return FVM_OK;}
int main(int argc,char **)
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    const mp3_fixture::Fixture &fixture=mp3_fixture::fixtures[0];Bytes swf=stream_movie(fixture,37);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    FVM_SWFStreamConverter converter,target;assert(fvm_swf_stream_converter_init(&converter,source,fixture.rate,fixture.channels)==FVM_OK);target=converter;
    SeekQueue queue;std::memset(&queue,0,sizeof(queue));FVM_AudioQueueBackend backend;std::memset(&backend,0,sizeof(backend));
    backend.struct_size=sizeof(backend);backend.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;backend.sample_rate=fixture.rate;backend.channels=fixture.channels;backend.userdata=&queue;
    backend.write=seek_write;backend.queued=seek_count;backend.pause=seek_pause;backend.clear=seek_clear;
    short pcm[16];FVM_SWFStreamOutput output;assert(fvm_swf_stream_output_init(&output,&converter,&backend,pcm,16,4,8)==FVM_OK);queue.output=&output;
    queue.reject=1;size_t n=999;assert(fvm_swf_stream_output_pump(&output,&n)==FVM_ERROR && !n && output.info.pending_frames==4);
    FVM_SWFStreamConverter before=converter,target_before=target;short pending[16];std::memcpy(pending,pcm,sizeof(pending));
    FVM_SWFStreamOutputInfo info=output.info;
    for(unsigned failure=0;failure<3;++failure)
    {
        if(failure==2 && argc>1)continue;
        queue.pause_fail=failure==0;queue.clear_fail=failure==1;queue.throws=failure==2;
        assert(fvm_swf_stream_output_bind_frame_aligned(&output,&target,3)==FVM_ERROR && !output.busy);
        assert(output.converter==&converter && !std::memcmp(&before,&converter,sizeof(before)) && !std::memcmp(&target_before,&target,sizeof(target)));
        assert(output.info.pending_frames==info.pending_frames && output.info.submitted_frames==info.submitted_frames && !std::memcmp(pending,pcm,sizeof(pending)));
    }
    queue.pause_fail=queue.clear_fail=queue.throws=0;
    size_t pauses=queue.pauses,clears=queue.clears;
    assert(fvm_swf_stream_output_bind_frame_aligned(&output,&target,0)==FVM_INVALID_ARGUMENT && queue.pauses==pauses && queue.clears==clears);
    FVM_SWFStreamConverter wrong=target;++wrong.output_rate;
    assert(fvm_swf_stream_output_bind_frame_aligned(&output,&wrong,3)==FVM_INVALID_ARGUMENT && queue.pauses==pauses);
    assert(fvm_swf_stream_output_bind_frame_aligned(&output,&target,3)==FVM_OK && output.converter==&target && output.info.paused && !output.info.pending_frames && !output.info.submitted_frames);
    FVM_SWFStreamSoundSeekPoint point;point.struct_size=sizeof(point);assert(fvm_swf_stream_sound_snapshot_seek_point(source,3,&point)==FVM_OK);
    assert(target.block_index==point.block_index && target.first==point.first && target.phase==0);
    assert(!std::memcmp(&before,&converter,sizeof(before)));
    queue.reject=0;assert(fvm_swf_stream_output_pump(&output,&n)==FVM_OK && n==4);
    short expected[8];size_t written=0;assert(fvm_swf_stream_sound_snapshot_read_pcm(source,point.block_index,point.first,4,expected,8,&written)==FVM_OK && !std::memcmp(expected,pcm,written*sizeof(short)));
    assert(fvm_swf_stream_output_seek_frame(&output,3)==FVM_OK && target.block_index==2 && target.first==0);
    assert(fvm_swf_stream_output_seek_frame_aligned(&output,65535)==FVM_OK && target.block_index==target.block_count && target.first==0);
    assert(fvm_swf_stream_output_pump(&output,&n)==FVM_NOT_FOUND && !n && output.info.eof);
    fvm_swf_stream_sound_snapshot_close(source);return 0;
}
