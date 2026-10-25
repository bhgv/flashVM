#include <stdio.h>
#include <stdlib.h>
#include "../src/multimedia/speech/stt/linux_vosk.hpp"
#include <string.h>


int main()
{
    char *text = 0;
    if (!(fvm_vosk_text("{\"text\":\"hello\"}", true, &text) == FVM_OK && !strcmp(text, "hello")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    delete[] text;
    if (!(fvm_vosk_text("{\"partial\":\"\\u041f\\u0440\\u0438\\u0432\\u0435\\u0442 \\ud83d\\ude00\"}", false, &text) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(text, "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xf0\x9f\x98\x80")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    delete[] text;
    if (!(fvm_vosk_text(" \n { \"text\" : \"quote \\\" slash \\\\ newline\\n\" }\t", true, &text) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(text, "quote \" slash \\ newline\n")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    delete[] text;
    if (!(fvm_vosk_text("{\"text\":\"\"}", true, &text) == FVM_NOT_FOUND && !text))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *bad[] = {"",
                         "{",
                         "{}",
                         "{\"text\":",
                         "{\"text\":\"bad\\u0000\"}",
                         "{\"text\":\"bad\\ud800\"}",
                         "{\"text\":\"bad\\udc00\"}",
                         "{\"text\":\"bad\\x\"}",
                         "{\"text\":\"ok\",\"extra\":true}",
                         "{\"text\":\"ok\"}extra",
                         "{\"partial\":\"wrong-key\"}",
                         "{\"text\":\"\xc0\x80\"}"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
    {
        if (!(fvm_vosk_text(bad[i], true, &text) == FVM_ERROR && !text))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const char valid[] = "{\"text\":\"abc\\u0410\\ud83d\\ude00\"}";
    for (size_t n = 0; n < strlen(valid); ++n)
    {
        char truncated[128];
        memcpy(truncated, valid, n);
        truncated[n] = 0;
        if (!(fvm_vosk_text(truncated, true, &text) == FVM_ERROR && !text))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
