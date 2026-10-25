#include <stdio.h>
#include "../src/multimedia/speech/tts/linux_ssip.hpp"

#if defined(__linux__) && !defined(__ANDROID__)
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


static void send_bytes(int fd, const char *text)
{
    size_t size = strlen(text);
    while (size)
    {
        ssize_t sent = write(fd, text, size);
        if (!(sent > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        text += sent;
        size -= (size_t)sent;
    }
}


static void expect(int fd, const char *text)
{
    for (size_t i = 0; text[i]; ++i)
    {
        char byte = 0;
        if (!(read(fd, &byte, 1) == 1 && byte == text[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


static void serve(int fd, int mode)
{
    expect(fd, "SET SELF CLIENT_NAME flashvm:tts:default\r\n");
    if (mode == 1)
    {
        send_bytes(fd, "bad-status\r\n");
        return;
    }
    if (mode == 2)
    {
        send_bytes(fd, "500 REJECTED\r\n");
        return;
    }
    send_bytes(fd, "208-CLIENT\r\n208 OK\r\n");
    expect(fd, "SPEAK\r\n");
    send_bytes(fd, "230 OK RECEIVING DATA\r\n");
    expect(fd,
           "first\r\n..\r\nCANCEL SELF\r\n\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xf0\x9f\x98\x80\r\n.\r\n");
    send_bytes(fd, "225-17\r\n225 OK MESSAGE QUEUED\r\n");
    if (mode == 3)
    {
        return;
    }
    expect(fd, "CANCEL SELF\r\n");
    send_bytes(fd, "200 OK CANCELED\r\n");
    expect(fd, "CANCEL SELF\r\n");
    send_bytes(fd, "200 OK CANCELED\r\n");
}
#endif


int main()
{
#if defined(__linux__) && !defined(__ANDROID__)
    for (int mode = 0; mode < 4; ++mode)
    {
        int pair[2];
        if (!(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        pid_t child = fork();
        if (!(child >= 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (child == 0)
        {
            close(pair[0]);
            serve(pair[1], mode);
            close(pair[1]);
            _exit(0);
        }
        close(pair[1]);
        FVM_LinuxTts *state = new FVM_LinuxTts;
        state->fd = pair[0];
        state->failed = false;
        const char identity[] = "SET SELF CLIENT_NAME flashvm:tts:default\r\n";
        if (!(fvm_ssip_send(state, identity, sizeof(identity) - 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int code = fvm_ssip_reply(state);
        if (mode == 1)
        {
            if (!(code == 0 && state->failed))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (mode == 2)
        {
            if (!(code == 500))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            state->failed = true;
        }
        else
        {
            if (!(code == 208))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_linux_tts_speak(
                      state,
                      "first\n.\nCANCEL SELF\r\n\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xf0\x9f\x98\x80") ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_linux_tts_stop(state) == (mode == 3 ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_linux_tts_close(state);
        int status = 0;
        if (!(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
