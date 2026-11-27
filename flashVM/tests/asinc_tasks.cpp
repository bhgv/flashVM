/* Copyright (c) 2026 bhgv
 * EN: Check script launch, cooperative cancellation and optional compiled AS2 SWF.
 * RU: Проверка запуска из AS, кооперативной отмены и необязательного SWF из AS2.
 */
#include "flashvm/async.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "../ports/thread_port.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


/* EN: Fail once with the violated case name.
 * RU: Завершиться с именем нарушенного сценария. */
static void check(bool valid, const char *message)
{
    if (!valid)
    {
        std::fprintf(stderr, "asinc task: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}


/* EN: Borrow an own value from a still-rooted object.
 * RU: Прочитать значение из удерживаемого объекта. */
static FVM_Value get(FVM_Context *context, FVM_Object *object, const char *name)
{
    FVM_Value result;
    check(fvm_object_get(context, object, name, &result) == FVM_OK, name);
    return result;
}


/* EN: Ask the script handle for a completion snapshot with a bounded test deadline.
 * RU: Получить итоговый снимок задачи с ограничением времени прогона. */
static FVM_Value finish(FVM_Context *context, FVM_Object *task, FVM_Result expected)
{
    for (unsigned n = 0; n < 3000; ++n)
    {
        FVM_Value result;
        check(fvm_object_call(context, task, "poll", 0, 0, &result) == FVM_OK &&
              result.type == FVM_VALUE_OBJECT, "poll result");
        FVM_Value done = get(context, result.object, "done");
        if (done.type == FVM_VALUE_BOOLEAN && done.boolean)
        {
            FVM_Value status = get(context, result.object, "status");
            check(status.type == FVM_VALUE_NUMBER && status.number == expected, "completion status");
            return get(context, result.object, "value");
        }
        fvm_port_thread_sleep_us(1000);
    }
    check(false, "worker did not complete");
    FVM_Value result;
    fvm_value_undefined(&result);
    return result;
}


/* EN: Snapshot a closure at launch and then cancel a native worker loop.
 * RU: Скопировать захват замыкания при запуске, затем остановить цикл потока. */
static void check_tasks(FVM_Context *context)
{
    const unsigned char actions[] = {0x96, 3, 0, 0, 'n', 0, 0x1c, 0x3e, 0};
    const unsigned char loop[] = {0x99, 2, 0, 0xfb, 0xff, 0};
    FVM_Object *capture = 0;
    FVM_Object *entry = 0;
    FVM_Value space, argument, task, value;

    check(fvm_get_global(context, "Async", &space) == FVM_OK && space.type == FVM_VALUE_OBJECT, "space");
    check(fvm_object_create(context, &capture) == FVM_OK &&
          fvm_gc_pin(context, capture) == FVM_OK, "capture");
    check(fvm_value_string(&value, "before") == FVM_OK &&
          fvm_object_set(context, capture, "n", &value) == FVM_OK, "initial capture");
    check(fvm_function_create_avm1(context, actions, sizeof(actions), 0, 0, capture, &entry) == FVM_OK,
          "closure");
    fvm_value_undefined(&argument);
    argument.type = FVM_VALUE_OBJECT;
    argument.object = entry;

    const char counted[] = {'o', 's', '\0', 'f', 'v', 'm'};
    check(fvm_value_string_bytes(&value, counted, sizeof(counted)) == FVM_OK &&
          fvm_object_set(context, capture, "n", &value) == FVM_OK, "change before call");
    check(fvm_object_call(context, space.object, "run", 1, &argument, &task) == FVM_OK &&
          fvm_gc_pin(context, task.object) == FVM_OK, "start on call");
    check(fvm_value_string(&value, "after") == FVM_OK &&
          fvm_object_set(context, capture, "n", &value) == FVM_OK, "mutate after launch");
    value = finish(context, task.object, FVM_OK);
    check(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == sizeof(counted) &&
          !std::memcmp(value.string, counted, sizeof(counted)), "owned counted result of copied capture");
    fvm_gc_unpin(context, task.object);
    fvm_gc_unpin(context, capture);

    check(fvm_function_create_avm1(context, loop, sizeof(loop), 0, 0, 0, &entry) == FVM_OK,
          "worker loop");
    argument.object = entry;
    check(fvm_object_call(context, space.object, "run", 1, &argument, &task) == FVM_OK &&
          fvm_gc_pin(context, task.object) == FVM_OK, "run loop");
    check(fvm_object_call(context, task.object, "poll", 0, 0, &value) == FVM_OK &&
          !get(context, value.object, "done").boolean, "owner polls while loop runs");
    check(fvm_object_call(context, task.object, "cancel", 0, 0, &value) == FVM_OK,
          "request stop");
    check(finish(context, task.object, FVM_ERROR).type == FVM_VALUE_UNDEFINED,
          "cancelled worker has no result");
    fvm_gc_unpin(context, task.object);
    check(fvm_gc_collect(context, 0) == FVM_OK, "release stopped workers");
}


/* EN: Execute the actual SWF from the AS2/FIR/SWL pipeline without frame advances after launch.
 * RU: Исполнить SWF из цепочки AS2/FIR/SWL без тактов кадров после запуска задачи. */
static void check_compiled(FVM_Context *context, const char *path)
{
    unsigned char bytes[4096];
    std::FILE *file = std::fopen(path, "rb");
    check(file != 0, "open compiled SWF");
    const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
    check(std::feof(file) && !std::ferror(file), "small complete SWF");
    std::fclose(file);
    FVM_Object *root = 0;
    check(fvm_load_memory(context, bytes, size) == FVM_OK &&
          fvm_movieclip_load_swf(context, bytes, size, "_root", &root) == FVM_OK &&
          fvm_gc_pin(context, root) == FVM_OK &&
          fvm_movieclip_advance(context, root, 1) == FVM_OK, "install compiled function");
    FVM_Value task;
    FVM_Value method = get(context, root, "compute");
    check(method.type == FVM_VALUE_OBJECT, "installed compiled entry");
    FVM_Value extra;
    fvm_value_number(&extra, 1);
    check(fvm_function_invoke(context, method.object, root, 1, &extra, &task) == FVM_INVALID_ARGUMENT,
          "compiled task rejects arguments");
    FVM_Result started = fvm_function_invoke(context, method.object, root, 0, 0, &task);
    if (started != FVM_OK)
    {
        std::fprintf(stderr, "invoke compute: %d\n", (int)started);
    }
    check(started == FVM_OK && fvm_gc_pin(context, task.object) == FVM_OK, "invoke compiled entry");
    FVM_Value result = finish(context, task.object, FVM_OK);
    check(result.type == FVM_VALUE_NUMBER && result.number == 42, "compiled detached worker result");
    fvm_gc_unpin(context, task.object);
    fvm_gc_unpin(context, root);
}


/* EN: Own the test context. With a path argument, also load the linked fixture.
 * RU: Владеть контекстом теста. При аргументе-пути также загрузить слинкованный SWF. */
int main(int argc, char **argv)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    check(fvm_config_init(&config) == FVM_OK &&
          fvm_runtime_create(&config, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK &&
          fvm_standard_install(context) == FVM_OK &&
          fvm_async_install(context) == FVM_OK, "install harness");
    check_tasks(context);
    if (argc == 2)
    {
        check_compiled(context, argv[1]);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    std::puts("PASS deferred asinc launch, detached capture, nonblocking status and cancellation");
    return EXIT_SUCCESS;
}
