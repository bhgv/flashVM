#include "flashvm/speech.h"
#include <assert.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
static int transport=-1;
static int connections=0;
extern "C" int __wrap_socket(int domain,int type,int protocol)
{
    assert(domain==AF_UNIX && type==SOCK_STREAM && protocol==0);
    return dup(transport);
}
extern "C" int __wrap_connect(int fd,const struct sockaddr *generic,socklen_t length)
{
    const unsigned char expected[]={0,'v','m',0,'t','t','s',0};
    const struct sockaddr_un *address=(const struct sockaddr_un *)generic;
    (void)fd;
    ++connections;
    if(length!=offsetof(struct sockaddr_un,sun_path)+sizeof(expected) || address->sun_family!=AF_UNIX || memcmp(address->sun_path,expected,sizeof(expected))){errno=EINVAL;return -1;}
    return 0;
}
static void expect(int fd,const char *text)
{
    for(size_t i=0;text[i];++i){char c=0;assert(read(fd,&c,1)==1 && c==text[i]);}
}
static void reply(int fd,const char *text)
{
    size_t left=strlen(text);while(left){ssize_t n=write(fd,text,left);assert(n>0);text+=n;left-=(size_t)n;}
}
#endif
int main()
{
#if defined(__linux__) && !defined(__ANDROID__)
    int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
    pid_t child=fork();assert(child>=0);
    if(child==0)
    {
        close(pair[0]);
        expect(pair[1],"SET SELF CLIENT_NAME flashvm:tts:default\r\n");reply(pair[1],"208 OK\r\n");
        expect(pair[1],"SPEAK\r\n");reply(pair[1],"230 OK\r\n");
        expect(pair[1],"hello\r\n.\r\n");reply(pair[1],"225 OK\r\n");
        expect(pair[1],"CANCEL SELF\r\n");reply(pair[1],"200 OK\r\n");
        expect(pair[1],"CANCEL SELF\r\n");reply(pair[1],"200 OK\r\n");
        close(pair[1]);_exit(0);
    }
    close(pair[1]);transport=pair[0];
    const unsigned char name[]={'v','m',0,'t','t','s',0};
    FVM_SpeechUnixEndpoint endpoint;endpoint.struct_size=sizeof(endpoint);
    endpoint.name_space=FVM_SPEECH_UNIX_ABSTRACT;endpoint.name=name;endpoint.length=sizeof(name);
    FVM_TtsBackend backend;FVM_TtsSession *session=0;
    assert(fvm_tts_unix_backend(&endpoint,&backend)==FVM_OK);
    assert(fvm_tts_open(&backend,&session)==FVM_OK && connections==1);
    close(transport);transport=-1;
    assert(fvm_tts_speak(session,"hello")==FVM_OK);
    assert(fvm_tts_stop(session)==FVM_OK);
    fvm_tts_close(session);
    int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
#endif
    return 0;
}
