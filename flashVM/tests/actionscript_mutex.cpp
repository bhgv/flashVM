#include <stdio.h>
#include <stdlib.h>
#include "flashvm/extensions.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/worker.h"
#include "flashvm/table.h"
#include "flashvm/mutex.h"
#include "flashvm/debug.h"
#include "flashvm/transfer.h"
#include "../ports/thread_port.h"
#include "../src/details/vector.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void byte(Bytes &b, unsigned int x)
{
    b.push_back((unsigned char)x);
}


static void word(Bytes &b, size_t n)
{
    byte(b, (unsigned int)n);
    byte(b, (unsigned int)(n >> 8));
}


static void text(Bytes &b, const char *s)
{
    do
    {
        byte(b, (unsigned char)*s);
    } while (*s++);
}


static void str(Bytes &b, const char *s)
{
    byte(b, 0x96);
    word(b, std::strlen(s) + 2);
    byte(b, 0);
    text(b, s);
}


static void number(Bytes &b, unsigned long n)
{
    byte(b, 0x96);
    word(b, 5);
    byte(b, 7);
    for (int i = 0; i < 4; ++i)
    {
        byte(b, (unsigned int)(n >> (i * 8)));
    }
}


static void var(Bytes &b, const char *s)
{
    str(b, s);
    byte(b, 0x1c);
}


static void call(Bytes &b, const char *receiver, const char *name, unsigned int n)
{
    number(b, n);
    var(b, receiver);
    str(b, name);
    byte(b, 0x52);
}


static void define(Bytes &b, const char *name, const char *param, const Bytes &body)
{
    byte(b, 0x9b);
    word(b, std::strlen(name) + 1 + 2 + (param ? std::strlen(param) + 1 : 0) + 2 + body.size());
    text(b, name);
    word(b, param ? 1 : 0);
    if (param)
    {
        text(b, param);
    }
    word(b, body.size());
    for (size_t i = 0; i < body.size(); ++i)
    {
        byte(b, body[i]);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value mutex, thread, lock, worker, result, value, args[3], id;
    FVM_Channel *ch = 0;
    FVM_Object *wrapper = 0;
    FVM_DebugChannelWait waits[8];
    size_t count = 0;
    Bytes body, defs, code;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "mutex", &mutex) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "thread", &thread) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    str(code, "lock");
    call(code, "mutex", "new", 0);
    byte(code, 0x1d);
    call(code, "lock", "set", 0);
    byte(code, 0x17);
    str(code, "held");
    call(code, "lock", "touch", 0);
    byte(code, 0x1d);
    call(code, "lock", "reset", 0);
    byte(code, 0x17);
    str(code, "locks");
    call(code, "mutex", "list", 0);
    byte(code, 0x1d);
    str(code, "firstLock");
    var(code, "locks");
    number(code, 0);
    byte(code, 0x4e);
    byte(code, 0x1d);
    var(code, "locks");
    number(code, 1);
    var(code, "lock");
    byte(code, 0x4f);
    str(code, "secondLock");
    var(code, "locks");
    number(code, 1);
    byte(code, 0x4e);
    byte(code, 0x1d);
    byte(code, 0);
    if (!(fvm_avm1_execute(c, &code[0], code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "held", &value) == FVM_OK && value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "lock", &lock) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "firstLock", &value) == FVM_OK && value.object == lock.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "secondLock", &value) == FVM_OK && value.object == lock.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(body, "input", "set", 0);
    byte(body, 0x17);
    call(body, "input", "reset", 0);
    byte(body, 0x17);
    var(body, "input");
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "roundtrip", "input", body);
    body.clear();
    call(body, "mutex", "new", 0);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "createMutex", 0, body);
    body.clear();
    call(body, "sharedChannel", "receive", 0);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "waitForPacket", 0, body);
    byte(defs, 0);
    if (!(fvm_avm1_execute(c, &defs[0], defs.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&args[0], "roundtrip");
    args[1] = lock;
    if (!(fvm_object_call(c, thread.object, "run", 2, args, &worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, worker.object, "join", 0, 0, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.object == lock.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&args[0], "createMutex");
    if (!(fvm_object_call(c, thread.object, "run", 1, args, &worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, worker.object, "id", &id) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, worker.object, "join", 0, 0, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, result.object, "origin", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(value.type == FVM_VALUE_NUMBER && value.number == id.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, result.object, "set", 0, 0, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, result.object, "reset", 0, 0, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_wrap_channel(c, ch, &wrapper) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_map_action(c, "waitForPacket", 0, "mutex-demo.as", 17) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&args[0], "waitForPacket");
    fvm_value_undefined(&args[1]);
    fvm_value_undefined(&args[2]);
    args[2].type = FVM_VALUE_OBJECT;
    args[2].object = wrapper;
    if (!(fvm_object_call(c, thread.object, "run", 3, args, &worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, worker.object, "id", &id) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 5000; ++i)
    {
        if (!(fvm_debug_channel_wait_snapshot(waits, 8, &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (count)
        {
            break;
        }
        fvm_port_thread_sleep_us(1000);
    }
    if (!(count == 1 && waits[0].context == id.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(waits[0].channel == fvm_channel_identity(ch) && !waits[0].sending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(waits[0].offset > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(waits[0].line == 17 && !std::strcmp(waits[0].source, "mutex-demo.as")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(waits[0].function, "waitForPacket")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_freeze(ch, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &lock, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_message(c, ch, 0, &result) == FVM_OK && result.object == lock.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_freeze(ch, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, worker.object, "join", 0, 0, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.object == lock.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_channel_wait_snapshot(waits, 8, &count) == FVM_OK && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    fvm_channel_destroy(ch);
    return 0;
}
