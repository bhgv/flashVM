#include <stdio.h>
#include <stdlib.h>
#include "../src/multimedia/speech/tts/linux_ssip.hpp"
#include <string.h>
#include <stddef.h>


int main()
{
#if defined(__linux__)
    struct sockaddr_un address;
    socklen_t length = 0;
    const unsigned char binary[] = {'v', 'm', 0, 't', 't', 's', 0};
    FVM_SpeechUnixEndpoint endpoint;
    FVM_TtsBackend backend;
    endpoint.struct_size = sizeof(endpoint);
    endpoint.name_space = FVM_SPEECH_UNIX_ABSTRACT;
    endpoint.name = binary;
    endpoint.length = sizeof(binary);
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(address.sun_family == AF_UNIX && address.sun_path[0] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(address.sun_path + 1, binary, sizeof(binary))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(length == offsetof(struct sockaddr_un, sun_path) + 1 + sizeof(binary)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_unix_backend(&endpoint, &backend) == FVM_OK && backend.open))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    endpoint.name_space = FVM_SPEECH_UNIX_FILESYSTEM;
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_INVALID_ARGUMENT && !length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char path[] = {'/', 't', 'm', 'p', '/', 's'};
    endpoint.name = path;
    endpoint.length = sizeof(path);
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(address.sun_path, path, sizeof(path)) && address.sun_path[sizeof(path)] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(length == offsetof(struct sockaddr_un, sun_path) + sizeof(path) + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    endpoint.length = sizeof(address.sun_path);
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_INVALID_ARGUMENT && !length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char maximum[sizeof(address.sun_path) - 1];
    memset(maximum, 'x', sizeof(maximum));
    endpoint.length = sizeof(maximum);
    endpoint.name = maximum;
    endpoint.name_space = FVM_SPEECH_UNIX_ABSTRACT;
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_OK && length == sizeof(address)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    endpoint.name_space = 99;
    if (!(fvm_tts_unix_backend(&endpoint, &backend) == FVM_INVALID_ARGUMENT && !backend.open))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    endpoint.name_space = FVM_SPEECH_UNIX_ABSTRACT;
    endpoint.length = 0;
    if (!(fvm_ssip_address(&endpoint, &address, &length) == FVM_INVALID_ARGUMENT && !length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    FVM_TtsBackend backend;
    if (!(fvm_tts_unix_backend(0, &backend) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
