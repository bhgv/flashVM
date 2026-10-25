#include <stdio.h>
#include <stdlib.h>
#include "../src/core/fault_wire.hpp"
#include <cstring>


int main()
{
    FVM_ExceptionInfo sender, caller, result, before;
    unsigned char valid[FVM_FAULT_WIRE_SIZE], bad[FVM_FAULT_WIRE_SIZE];
    std::memset(&sender, 0, sizeof(sender));
    std::memset(&caller, 0, sizeof(caller));
    sender.code = caller.code = FVM_RESOURCE_CLOSED;
    sender.resource = 0xf1234567UL;
    sender.origin_thread = 0x81234567UL;
    sender.flags = FVM_EXCEPTION_FILE | FVM_EXCEPTION_THREAD;
    std::strcpy(sender.kind, "mutex");
    std::strcpy(sender.path, "remote-file.bin");
    std::strcpy(sender.operation, "capability.receive");
    std::strcpy(sender.message, "closed nested mutex");
    caller.thread = 33;
    caller.line = 29;
    caller.offset = 17;
    std::strcpy(caller.source, "caller.as");
    std::strcpy(caller.function, "receive");
    std::strcpy(caller.address, "fvm://outer/authority");
    fvm_fault_wire_encode(valid, sender);
    if (!(valid[8] == 0xf1 && valid[9] == 0x23 && valid[12] == 0x81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    result = caller;
    if (!(fvm_fault_wire_decode(&result, valid, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.resource == sender.resource && result.origin_thread == sender.origin_thread))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.thread == 33 && result.line == 29 && result.offset == 17 && !std::strcmp(result.source, "caller.as")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(result.kind, "mutex") && !std::strcmp(result.path, "remote-file.bin")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(result.address, caller.address) && !std::strcmp(result.operation, sender.operation)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((result.flags &
           (FVM_EXCEPTION_FILE | FVM_EXCEPTION_THREAD | FVM_EXCEPTION_NETWORK | FVM_EXCEPTION_REMOTE_CONTEXT)) == 15))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::strcpy(sender.address, "fvm://nested/authority");
    fvm_fault_wire_encode(valid, sender);
    result = caller;
    std::strcpy(result.path, "carrier-file.bin");
    if (!(fvm_fault_wire_decode(&result, valid, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(result.path, "carrier-file.bin") && !std::strcmp(result.address, sender.address)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 10; ++mode)
    {
        std::memcpy(bad, valid, sizeof(bad));
        size_t size = sizeof(bad);
        if (mode == 0)
        {
            --size;
        }
        else if (mode == 1)
        {
            ++size;
        }
        else if (mode == 2)
        {
            bad[0] = 'X';
        }
        else if (mode == 3)
        {
            bad[7] = 2;
        }
        else if (mode == 4)
        {
            bad[19] = 8;
        }
        else if (mode == 5)
        {
            std::memset(bad + 20, 'x', 16);
        }
        else if (mode == 6)
        {
            std::memset(bad + 36, 'x', 512);
        }
        else if (mode == 7)
        {
            std::memset(bad + 548, 'x', 32);
        }
        else if (mode == 8)
        {
            std::memset(bad + 580, 'x', 128);
        }
        else
        {
            std::memset(bad + 708, 'x', 512);
        }
        result = caller;
        before = result;
        if (!(!fvm_fault_wire_decode(&result, bad, size)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(&result, &before, sizeof(result))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    result = caller;
    before = result;
    if (!(!fvm_fault_wire_decode(&result, 0, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&result, &before, sizeof(result))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_fault_wire_decode(0, valid, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::strcpy(sender.kind, "unknown");
    fvm_fault_wire_encode(valid, sender);
    if (!(!fvm_fault_wire_decode(&result, valid, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    result.code = 0;
    if (!(!fvm_fault_wire_decode(&result, valid, sizeof(valid))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
