/* Copyright (c) 2026 bhgv
 * EN: Launch real DisVM jobs from ActionScript and verify reverse resources.
 * RU: Запуск настоящих программ DisVM из ActionScript с проверкой встречных ресурсов.
 */
#define _POSIX_C_SOURCE 200809L
#include "session-test.h"
#include "flashvm/avm1.h"
#include "styxrpc/vfs_client.h"
#include <stdio.h>
#include <time.h>

#define CONTEXT_PID 42U         /* Test-scoped reverse authority. */
#define CONTEXT_INITIAL 41      /* Initial caller-owned variable value. */
#define CONTEXT_GUEST_FD 7U     /* Write-only descriptor delegated by the host. */
#define LAUNCH_POLL_TRIES 300U   /* Finite receipt poll attempts. */

#define ACTION_END 0x00         /* End checked top-level bytecode. */
#define ACTION_GET_VARIABLE 0x1c /* Read a checked global by its stack name. */
#define ACTION_SET_VARIABLE 0x1d /* Save the receipt token in the script global. */
#define ACTION_CALL_METHOD 0x52  /* Call launch on the script session object. */
#define ACTION_PUSH 0x96         /* Push typed operands. */
#define PUSH_STRING 0x00         /* NUL-terminated bytecode string. */
#define PUSH_INTEGER 0x07        /* Little-endian ActionScript integer. */


/* EN: Return the test count owned by the reverse server thread.
 * RU: Выдать тестовую переменную из потока встречного сервера. */
static int read_count(void *state, int32_t *value)
{
    *value = ((TestLaunchContext *)state)->value;
    return 1;
}


/* EN: Store the checked signed value on the reverse server thread.
 * RU: Записать проверенное целое в потоке встречного сервера. */
static int write_count(void *state, int32_t value)
{
    ((TestLaunchContext *)state)->value = value;
    return 1;
}


/* EN: Provide the single fixed VFS marker byte at offset zero.
 * RU: Выдать один фиксированный байт VFS с нулевого смещения. */
static int read_marker(void *state, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    (void)state;
    *size = !offset && capacity ? 1 : 0;
    if (*size)
    {
        *(unsigned char *)bytes = 'C';
    }
    return 1;
}


/* EN: Accept the one-byte guest descriptor report.
 * RU: Принять однобайтовый отчёт гостя по выданному дескриптору. */
static int write_output(void *state, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    if (offset || count != 1)
    {
        return 0;
    }
    ((TestLaunchContext *)state)->output = *(const unsigned char *)bytes;
    *size = count;
    return 1;
}


/* EN: Notify the AS test thread after the guest enters its cancellation checkpoint.
 * RU: Уведомить поток AS после входа гостя в контрольную точку отмены. */
static int write_parked(void *state, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    if (offset || count != 1 || *(const unsigned char *)bytes != 'P')
    {
        return 0;
    }
    __atomic_store_n(&((TestLaunchContext *)state)->parked, 1, __ATOMIC_RELEASE);
    *size = count;
    return 1;
}


/* EN/RU: See session-test.h. / См. session-test.h. */
int test_launch_resources(StyxRpcPrograms *programs, TestLaunchContext *state)
{
    const StyxIntVariable variable = {state, read_count, write_count};
    const StyxRpcResource marker = {NULL, NULL, read_marker, NULL, NULL};
    const StyxRpcResource output = {state, NULL, NULL, write_output, NULL};
    const StyxRpcResource parked = {state, NULL, NULL, write_parked, NULL};
    state->value = CONTEXT_INITIAL;
    return styxrpc_program_add(programs, CONTEXT_PID) &&
        styxrpc_program_int(programs, CONTEXT_PID, "count", &variable) &&
        styxrpc_program_file(programs, CONTEXT_PID, "marker", STYXRPC_READ, 1, &marker) &&
        styxrpc_program_file(programs, CONTEXT_PID, "parked", STYXRPC_WRITE, 0, &parked) &&
        styxrpc_program_fd(programs, CONTEXT_PID, CONTEXT_GUEST_FD, STYXRPC_WRITE, 0, &output);
}


/* EN: Start a named job by executing an actual call-method action in FlashVM.
 * RU: Запустить именованную программу через настоящую инструкцию вызова FlashVM. */
static int launch_script(FVM_Context *context, const char *name, const char *text, size_t length, FVM_Value *token)
{
    static const unsigned char actions[] = {
        ACTION_PUSH, 6, 0, PUSH_STRING, 't', 0, PUSH_STRING, 'a', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 3, 0, PUSH_STRING, 'n', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 8, 0, PUSH_INTEGER, 2, 0, 0, 0, PUSH_STRING, 'r', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 8, 0, PUSH_STRING, 'l', 'a', 'u', 'n', 'c', 'h', 0,
        ACTION_CALL_METHOD, ACTION_SET_VARIABLE, ACTION_END
    };
    FVM_Value arg, program;
    return fvm_value_string_bytes(&arg, text, length) == FVM_OK &&
        fvm_value_string(&program, name) == FVM_OK &&
        fvm_set_global(context, "a", &arg) == FVM_OK &&
        fvm_set_global(context, "n", &program) == FVM_OK &&
        fvm_avm1_execute(context, actions, sizeof(actions)) == FVM_OK &&
        fvm_get_global(context, "t", token) == FVM_OK && token->type == FVM_VALUE_NUMBER;
}


/* EN: Wait for a non-pending receipt or a finite timeout, leaving the ticket open.
 * RU: Дождаться конечной квитанции или ограниченного тайм-аута, сохраняя токен. */
static int completion(FVM_Context *context, FVM_Object *object, const FVM_Value *token, StyxRpcLaunchStatus expected)
{
    const struct timespec pause = {0, 10000000};
    FVM_Value result;
    for (unsigned i = 0; i < LAUNCH_POLL_TRIES; ++i)
    {
        if (fvm_object_call(context, object, "status", 1, token, &result) != FVM_OK ||
            result.type != FVM_VALUE_NUMBER)
        {
            return 0;
        }
        if (result.number != STYXRPC_LAUNCH_PENDING)
        {
            return result.number == expected;
        }
        nanosleep(&pause, NULL);
    }
    return 0;
}


/* EN: Check exact cancellation with another successful launch while the first is pending.
 * RU: Проверить отмену точного запуска, выполнив второй, пока первый ожидает. */
static int exercise_jobs(FVM_Context *context, FVM_Object *object, TestLaunchContext *state)
{
    const struct timespec pause = {0, 10000000};
    const char args[] = {'A', 0, 'B'};
    FVM_Value parked, complete, failed, result;
    if (!launch_script(context, "park.dis", "park", 4, &parked) || parked.number <= 0)
    {
        fputs("Parked launch admission failed\n", stderr);
        return 0;
    }
    for (unsigned i = 0; i < LAUNCH_POLL_TRIES && !__atomic_load_n(&state->parked, __ATOMIC_ACQUIRE); ++i)
    {
        nanosleep(&pause, NULL);
    }
    if (!__atomic_load_n(&state->parked, __ATOMIC_ACQUIRE) ||
        !launch_script(context, "context.dis", args, sizeof(args), &complete) || complete.number <= parked.number ||
        !completion(context, object, &complete, STYXRPC_LAUNCH_OK) ||
        fvm_object_call(context, object, "status", 1, &parked, &result) != FVM_OK ||
        result.type != FVM_VALUE_NUMBER || result.number != STYXRPC_LAUNCH_PENDING ||
        fvm_object_call(context, object, "close", 1, &parked, &result) != FVM_OK ||
        fvm_object_call(context, object, "status", 1, &parked, &result) != FVM_INVALID_ARGUMENT ||
        fvm_object_call(context, object, "close", 1, &complete, &result) != FVM_OK)
    {
        fputs("Concurrent named launch or exact cancellation failed\n", stderr);
        return 0;
    }
    return launch_script(context, "context.dis", "invalid", 7, &failed) && failed.number > complete.number &&
        completion(context, object, &failed, STYXRPC_LAUNCH_FAILED) &&
        fvm_object_call(context, object, "close", 1, &failed, &result) == FVM_OK &&
        launch_script(context, "../context.dis", "", 0, &result) && result.number == STYXRPC_LAUNCH_NOT_SENT;
}


/* EN: Leave one admitted pending job for teardown, then check its stale token.
 * RU: Оставить принятый незавершённый запуск для закрытия и проверить старый токен. */
static int exercise_teardown(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding,
    TestLaunchContext *state)
{
    const struct timespec pause = {0, 10000000};
    FVM_Value token, status;
    __atomic_store_n(&state->parked, 0, __ATOMIC_RELEASE);
    if (!launch_script(context, "park.dis", "park", 4, &token) || token.number <= 0)
    {
        return 0;
    }
    for (unsigned i = 0; i < LAUNCH_POLL_TRIES && !__atomic_load_n(&state->parked, __ATOMIC_ACQUIRE); ++i)
    {
        nanosleep(&pause, NULL);
    }
    int pending = __atomic_load_n(&state->parked, __ATOMIC_ACQUIRE) &&
        fvm_object_call(context, object, "status", 1, &token, &status) == FVM_OK &&
        status.type == FVM_VALUE_NUMBER && status.number == STYXRPC_LAUNCH_PENDING;
    fvm_styxrpc_session_close(binding);
    return pending && fvm_object_call(context, object, "status", 1, &token, &status) == FVM_INVALID_ARGUMENT;
}


/* EN/RU: See session-test.h. / См. session-test.h. */
int test_launch_call(FVM_Context *context, FVM_StyxRpcSession *binding, struct StyxClient *client,
    TestLaunchContext *state)
{
    const StyxRpcProgram program = {CONTEXT_PID, 0, 0};
    FVM_Value object, path, metadata, directory;
    unsigned char profile[64];
    size_t size = 0;
    int ok = fvm_styxrpc_session_bind(context, binding, client, &program, &object) == FVM_OK &&
        fvm_set_global(context, "r", &object) == FVM_OK &&
        fvm_value_string(&path, "programs") == FVM_OK &&
        fvm_object_call(context, object.object, "stat", 1, &path, &metadata) == FVM_OK &&
        metadata.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, metadata.object, "4", &directory) == FVM_OK &&
        directory.type == FVM_VALUE_BOOLEAN && directory.boolean &&
        styxrpc_vfs_read_at(client, ".interface", 0, profile, sizeof(profile), &size) == STYXRPC_IO_OK &&
        size > 0 && profile[0] == 'd' &&
        exercise_jobs(context, object.object, state) && exercise_teardown(context, object.object, binding, state);
    fvm_styxrpc_session_close(binding);
    return ok;
}
