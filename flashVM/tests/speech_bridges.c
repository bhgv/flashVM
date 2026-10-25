#include <stdio.h>
#include <stdlib.h>
#include "flashvm/speech.h"
#include <string.h>


typedef struct Provider
{
    int opens, speaks, stops, closes, pending;
} Provider;


static FVM_Result open_provider(void *host, void **out)
{
    Provider *p = (Provider *)host;
    ++p->opens;
    *out = p;
    return FVM_OK;
}


static FVM_Result speak_provider(void *host, const char *text)
{
    Provider *p = (Provider *)host;
    if (!(strcmp(text, "hello") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->speaks;
    return FVM_OK;
}


static FVM_Result stop_provider(void *host)
{
    Provider *p = (Provider *)host;
    ++p->stops;
    return FVM_OK;
}


static void close_provider(void *host)
{
    Provider *p = (Provider *)host;
    ++p->closes;
}


static FVM_Result poll_provider(void *host, char *text, size_t capacity, size_t *required, int *final)
{
    Provider *p = (Provider *)host;
    if (!p->pending)
    {
        return FVM_NOT_FOUND;
    }
    *required = 6;
    *final = 1;
    if (!capacity)
    {
        return FVM_OK;
    }
    if (capacity < 6)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memcpy(text, "hello", 6);
    p->pending = 0;
    return FVM_OK;
}


int main(void)
{
    Provider p;
    FVM_TtsBackend tts, native_tts;
    FVM_SttBackend stt, native_stt;
    FVM_TtsSession *output = 0;
    FVM_SttSession *input = 0;
    FVM_LinuxSttConfig linux_config;
    FVM_Result result;
    char text[8];
    size_t required;
    int final;
    memset(&p, 0, sizeof(p));
    memset(&tts, 0, sizeof(tts));
    memset(&stt, 0, sizeof(stt));
    tts.struct_size = sizeof(tts);
    tts.abi_version = FVM_SPEECH_ABI_VERSION;
    tts.userdata = &p;
    tts.open = open_provider;
    tts.speak = speak_provider;
    tts.stop = stop_provider;
    tts.close = close_provider;
    if (!(fvm_tts_open(0, &output) == FVM_INVALID_ARGUMENT && !output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_open(&tts, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&tts, 0, sizeof(tts));
    if (!(fvm_tts_speak(output, "") == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_speak(output, "\xc0\x80") == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_speak(output, "hello") == FVM_OK && p.speaks == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_stop(output) == FVM_OK && p.stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_tts_close(output);
    fvm_tts_close(0);
    if (!(p.closes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    stt.struct_size = sizeof(stt);
    stt.abi_version = FVM_SPEECH_ABI_VERSION;
    stt.userdata = &p;
    stt.open = open_provider;
    stt.poll = poll_provider;
    stt.close = close_provider;
    if (!(fvm_stt_open(&stt, &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&stt, 0, sizeof(stt));
    if (!(fvm_stt_poll(input, text, sizeof(text), &required, &final) == FVM_NOT_FOUND && !required && !*text))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p.pending = 1;
    if (!(fvm_stt_poll(input, 0, 0, &required, &final) == FVM_OK && required == 6 && final == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stt_poll(input, text, 2, &required, &final) == FVM_INVALID_ARGUMENT && p.pending && required == 6 &&
          !*text))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stt_poll(input, text, sizeof(text), &required, &final) == FVM_OK && !strcmp(text, "hello") && !p.pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_stt_close(input);
    fvm_stt_close(0);
    if (!(p.opens == 2 && p.closes == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stt_poll(0, text, sizeof(text), &required, &final) == FVM_INVALID_ARGUMENT && !*text))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_system_backend(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stt_system_backend(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_linux_backend(0, &native_tts) == FVM_INVALID_ARGUMENT && !native_tts.open))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    linux_config.struct_size = sizeof(linux_config);
    linux_config.model_path = "explicit-model";
    linux_config.alsa_device = "explicit-device";
    result = fvm_stt_linux_backend(&linux_config, &native_stt);
    if (!(result == FVM_UNSUPPORTED || result == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_tts_android_backend(0, &native_tts) == FVM_INVALID_ARGUMENT && !native_tts.open))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stt_android_backend(0, &native_stt) == FVM_INVALID_ARGUMENT && !native_stt.open))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
