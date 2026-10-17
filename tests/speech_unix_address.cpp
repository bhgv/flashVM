#include "../src/multimedia/speech/tts/linux_ssip.hpp"
#include <assert.h>
#include <string.h>
#include <stddef.h>
int main()
{
#if defined(__linux__)
    struct sockaddr_un address;
    socklen_t length=0;
    const unsigned char binary[]={'v','m',0,'t','t','s',0};
    FVM_SpeechUnixEndpoint endpoint;
    FVM_TtsBackend backend;
    endpoint.struct_size=sizeof(endpoint);endpoint.name_space=FVM_SPEECH_UNIX_ABSTRACT;
    endpoint.name=binary;endpoint.length=sizeof(binary);
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_OK);
    assert(address.sun_family==AF_UNIX && address.sun_path[0]==0);
    assert(!memcmp(address.sun_path+1,binary,sizeof(binary)));
    assert(length==offsetof(struct sockaddr_un,sun_path)+1+sizeof(binary));
    assert(fvm_tts_unix_backend(&endpoint,&backend)==FVM_OK && backend.open);
    endpoint.name_space=FVM_SPEECH_UNIX_FILESYSTEM;
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_INVALID_ARGUMENT && !length);
    const char path[]={'/','t','m','p','/','s'};
    endpoint.name=path;endpoint.length=sizeof(path);
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_OK);
    assert(!memcmp(address.sun_path,path,sizeof(path)) && address.sun_path[sizeof(path)]==0);
    assert(length==offsetof(struct sockaddr_un,sun_path)+sizeof(path)+1);
    endpoint.length=sizeof(address.sun_path);
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_INVALID_ARGUMENT && !length);
    unsigned char maximum[sizeof(address.sun_path)-1];memset(maximum,'x',sizeof(maximum));
    endpoint.length=sizeof(maximum);endpoint.name=maximum;endpoint.name_space=FVM_SPEECH_UNIX_ABSTRACT;
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_OK && length==sizeof(address));
    endpoint.name_space=99;
    assert(fvm_tts_unix_backend(&endpoint,&backend)==FVM_INVALID_ARGUMENT && !backend.open);
    endpoint.name_space=FVM_SPEECH_UNIX_ABSTRACT;endpoint.length=0;
    assert(fvm_ssip_address(&endpoint,&address,&length)==FVM_INVALID_ARGUMENT && !length);
#else
    FVM_TtsBackend backend;
    assert(fvm_tts_unix_backend(0,&backend)==FVM_UNSUPPORTED);
#endif
    return 0;
}
