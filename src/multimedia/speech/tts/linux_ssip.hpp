/* OS: Linux. Speak through an explicitly selected real Speech Dispatcher Unix socket. */
#ifndef FVM_TTS_LINUX_SSIP_HPP /* Protect the private SSIP interface. */
#define FVM_TTS_LINUX_SSIP_HPP /* Define the private include guard. */
#include "../session.hpp" /* Share the public TTS callback contract. */
#if defined(__linux__) /* Isolate Unix-domain speech service access. */
#define FVM_SSIP_IO_TIMEOUT_SECONDS 5 /* Bound each blocking socket read/write wait. */
#define FVM_SSIP_REPLY_LINE_LIMIT 1024 /* Bound one server reply line including its terminator. */
#define FVM_SSIP_REPLY_LINES_LIMIT 64 /* Bound multiline replies instead of trusting an endless service. */
#define FVM_SSIP_RECEIVING_DATA 230 /* Native SSIP readiness code after SPEAK. */
#define FVM_SSIP_MESSAGE_QUEUED 225 /* Native SSIP acceptance code after the final dot line. */
#define FVM_SSIP_SUCCESS_MIN 200 /* First successful SSIP reply class value. */
#define FVM_SSIP_SUCCESS_LIMIT 300 /* First non-success value after the SSIP success class. */
struct FVM_LinuxTts /* Own one synchronous SSIP client connection. */
{ /* Own one synchronous SSIP client connection. */
    int fd; /* Own the connected Unix-domain socket until native close. */
    bool failed; /* Poison malformed/disconnected protocol state instead of reusing it. */
}; /* Finish native connection state. */
/* Send all bytes with EINTR retry and SIGPIPE suppression. */
bool fvm_ssip_send(FVM_LinuxTts *, const char *, size_t); /* OS: Linux send. */
/* Read a bounded multiline reply and return its numeric code, or zero on failure. */
int fvm_ssip_reply(FVM_LinuxTts *); /* OS: Linux recv. */
#include <sys/socket.h> /* Share the actual target socklen_t representation. */
#include <sys/un.h> /* Share the actual native sockaddr_un layout. */
/* Build exact native lengths; abstract names are binary, not C strings. */
FVM_Result fvm_ssip_address(const FVM_SpeechUnixEndpoint *, struct sockaddr_un *, socklen_t *); /* OS: Linux/Android. */
/* Connect the explicitly represented native Unix endpoint to a real SSIP service. */
FVM_Result fvm_unix_tts_open(void *, void **); /* OS: Linux/Android. */
/* Connect explicitly and identify this client to the real speech service. */
FVM_Result fvm_linux_tts_open(void *, void **); /* OS: Linux Speech Dispatcher. */
/* Dot-stuff plain UTF-8 text and queue real synthesis/playback through SSIP. */
FVM_Result fvm_linux_tts_speak(void *, const char *); /* OS: Linux Speech Dispatcher. */
/* Cancel this SSIP client's pending/current speech. */
FVM_Result fvm_linux_tts_stop(void *); /* OS: Linux Speech Dispatcher. */
/* Cancel and disconnect the native service. */
void fvm_linux_tts_close(void *); /* OS: Linux Speech Dispatcher. */
#endif /* Finish Linux-only declarations. */
#endif /* Finish private header protection. */
