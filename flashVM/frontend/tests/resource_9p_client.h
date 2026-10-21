/* Test-only standard-wire client; no private FS/server structure access. */
#ifndef FVM_TEST_RESOURCE_9P_CLIENT_H
#define FVM_TEST_RESOURCE_9P_CLIENT_H
#include "flashvm/r9p.h"
typedef struct Test9PClient
{
    FVM_RFS *fs;
    FVM_R9P *server;
    FVM_SBindings bindings;
    FVM_SBinding slot;
    unsigned long offset;
    int pending;
    unsigned char output[FVM_R9P_MSIZE];
    size_t size;
} Test9PClient;
int test_9p_create(Test9PClient *, FVM_RDispatch *, unsigned long, FVM_RHandle);
int test_9p_command(Test9PClient *, FVM_RHandle, unsigned long);
int test_9p_reply(Test9PClient *);
int test_9p_query(Test9PClient *, FVM_RDispatch *, FVM_RHandle, FVM_RReply *);
FVM_RResult test_9p_event(Test9PClient *, FVM_REvent *);
int test_9p_destroy(Test9PClient *);
#endif
