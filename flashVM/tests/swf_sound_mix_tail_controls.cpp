/* Callback reentry/errors preserve pending tail bytes until acknowledged clear or acceptance. */
#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"
#undef main
struct ControlQueue : TailQueue {FVM_SWFSoundMixQueue *owner;FVM_AudioQueueBackend *adapted;int observe_error,clear_error,pause_error,throwing;unsigned observations;};
static void recursive(ControlQueue *q)
{size_t accepted=91;FVM_SWFSoundMixQueuePending p;p.struct_size=sizeof(p);p.frames=91;p.origin=91;assert(fvm_swf_sound_mix_queue_pump_tail(q->owner,2,4,&accepted)==FVM_INVALID_ARGUMENT && !accepted);assert(fvm_swf_sound_mix_queue_pending(q->owner,&p)==FVM_INVALID_ARGUMENT && p.frames==91 && p.origin==91);assert(q->adapted->clear(q->adapted->userdata)==FVM_INVALID_ARGUMENT);}
static FVM_Result control_write(void *raw,const short *pcm,size_t frames){ControlQueue *q=(ControlQueue*)raw;recursive(q);if(q->throwing)throw 7;return tail_write(q,pcm,frames);}
static FVM_Result control_count(void *raw,size_t *out){ControlQueue *q=(ControlQueue*)raw;recursive(q);++q->observations;*out=q->count;return q->observe_error?FVM_ERROR:FVM_OK;}
static FVM_Result control_clear(void *raw){ControlQueue *q=(ControlQueue*)raw;recursive(q);return q->clear_error?FVM_ERROR:tail_clear(q);}
static FVM_Result control_pause(void *raw,int){ControlQueue *q=(ControlQueue*)raw;recursive(q);return q->pause_error?FVM_ERROR:FVM_OK;}
int main(int argc,char **argv)
{
 bool exceptions=argc==1;assert(argc==1 || (argc==2 && !std::strcmp(argv[1],"--no-throw")));
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);Samples samples;for(unsigned i=0;i<8;++i)samples.push_back((short)(100+i));Bytes swf=movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFSoundMixer *mixer=create(44100,1);assert(fvm_swf_sound_mixer_submit(mixer,c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 ControlQueue q;std::memset(&q,0,sizeof(q));FVM_AudioQueueBackend b;std::memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=44100;b.channels=1;b.userdata=&q;b.write=control_write;b.queued=control_count;b.pause=control_pause;b.clear=control_clear;FVM_AudioQueueBackend adapted;assert(fvm_swf_sound_mix_queue_create(mixer,&b,2,&q.owner,&adapted)==FVM_OK);q.adapted=&adapted;size_t accepted=91;
 q.observe_error=1;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_ERROR && !accepted && !q.calls);q.observe_error=0;q.fail=1;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_ERROR && !accepted);q.clear_error=q.pause_error=1;assert(adapted.clear(adapted.userdata)==FVM_ERROR && adapted.pause(adapted.userdata,1)==FVM_ERROR);q.clear_error=q.pause_error=0;assert(adapted.pause(adapted.userdata,1)==FVM_OK);if(exceptions){q.throwing=1;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_ERROR && !accepted);q.throwing=0;}
 q.fail=0;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_OK && accepted==2 && q.saved[0]==100 && q.saved[1]==101);
 q.count=0;q.fail=1;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_ERROR);assert(adapted.clear(adapted.userdata)==FVM_OK);q.fail=0;assert(fvm_swf_sound_mix_queue_pump_tail(q.owner,2,4,&accepted)==FVM_OK && q.saved[2]==104 && q.saved[3]==105);
 FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(q.owner,&pending)==FVM_OK && !pending.frames && pending.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_NONE);fvm_swf_sound_mix_queue_close(q.owner);fvm_swf_sound_mixer_close(mixer);
 return 0;
}
