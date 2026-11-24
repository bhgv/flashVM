/* Copyright (c) 2026 bhgv
 * EN: Exercise a retained flash object image and a paired Dis module namespace.
 * RU: Проверка удерживаемого образа Flash и пространства модулей парной DisVM.
 */
#include "object-module.h"
#include "managed-test.h"
#include "channel-test.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/styxrpc_filesystem.h"
#include "flashvm/styxrpc_module.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <stdio.h>
#include <string.h>

#define LABEL_SIGNATURE UINT32_C(0xac26a778) /* fn(string): (real, string). */
#define PORT_SIGNATURE UINT32_C(0x09ef996d)  /* fn(int): chan of int. */
#define NUMBER_SIGNATURE UINT32_C(0xdf75d234) /* fn(real, string): (real, string). */
#define FRAME_LIMIT 65536U /* Shared carrier message boundary. */
#define MSIZE 4096U        /* Requested maximum Styx message. */
#define WAIT_MS 3000U      /* Mount and invocation time budget. */
#define TAGS 8U            /* Outstanding Styx request tags. */


/* EN: Access a shared script property on the exact AS receiver.
 * RU: Читать общее свойство сценария на точном объекте AS. */
static int count(FVM_Context *context, FVM_Object *self, FVM_Value *value)
{
    return self && fvm_object_get(context, self, "state", value) == FVM_OK && value->type == FVM_VALUE_NUMBER;
}


/* EN: Change the receiver's state and return the result.
 * RU: Изменить состояние объекта и вернуть результат. */
static FVM_Result add(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    (void)unused;
    if (argc != 1 || args->type != FVM_VALUE_NUMBER || !count(context, self, out))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(out, out->number + args->number);
    return fvm_object_set(context, self, "state", out);
}


/* EN: Return a managed tuple with the live receiver state and counted text.
 * RU: Вернуть управляемый кортеж с живым состоянием объекта и строкой с длиной. */
static FVM_Result label(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_Value fields[2];
    (void)unused;
    if (argc != 1 || !count(context, self, fields))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fields[1] = *args;
    return fvm_exec_literal_array(context, 2, fields, out);
}


/* EN: Seed one independently leased channel from the same live receiver.
 * RU: Заполнить отдельно арендованный канал по состоянию того же объекта. */
static FVM_Result seed(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    (void)unused;
    if (argc != 1 || args->type != FVM_VALUE_NUMBER || !count(context, self, out))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(out, out->number + args->number);
    return FVM_OK;
}


/* [GC:] EN: Publish a live image and verify that its native pin survives full collection.
 * RU: Опубликовать живой образ и проверить удержание объекта после полной сборки. */
int test_object_image_serve(FVM_Context *context, int fd)
{
    const FVM_StyxRpcObjectMethod methods[] = {
        {"label", "describe", STYXRPC_MODULE_MANAGED, LABEL_SIGNATURE, {"(s)", "((fs))"}, 0},
        {"port", "seed", STYXRPC_MODULE_CHANNEL, PORT_SIGNATURE, {"(i)", "(c[i])"}, 1},
        {"add", "accumulate", STYXRPC_MODULE_INT, STYXRPC_INT_SIGNATURE, {"", ""}, 0}
    };
    const FVM_StyxRpcObjectVariable variables[] = {{"count", "state", 1}, {"limit", "state", 0}};
    FVM_Object *object;
    FVM_Value number;
    FVM_StyxRpcObjectImage *image = NULL;
    StyxRpcModuleChannel leases[2];
    StyxRpcModuleChannels registry;
    FVM_GCRootScope *roots;
    if (fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return 0;
    }
    FVM_Value rooted;
    fvm_value_undefined(&rooted);
    fvm_value_number(&number, 40);
    int ready = fvm_object_create(context, &object) == FVM_OK;
    if (ready)
    {
        rooted.type = FVM_VALUE_OBJECT;
        rooted.object = object;
        const FVM_StyxRpcObjectSpec spec = {"flash-object.dis", object, methods, 3, variables, 2, &registry, NULL};
        ready = fvm_gc_roots_add(roots, &rooted) == FVM_OK &&
            fvm_object_set(context, object, "state", &number) == FVM_OK &&
            fvm_object_define_method_rooted(context, object, "accumulate", add, NULL, NULL) == FVM_OK &&
            fvm_object_define_method_rooted(context, object, "describe", label, NULL, NULL) == FVM_OK &&
            fvm_object_define_method_rooted(context, object, "seed", seed, NULL, NULL) == FVM_OK &&
            styxrpc_module_channels_init(&registry, leases, 2, test_channel_entropy, NULL) &&
            fvm_styxrpc_object_image_create(context, &spec, &image) == FVM_OK;
    }
    fvm_gc_roots_end(roots);
    if (!ready || fvm_gc_collect(context, NULL) != FVM_OK)
    {
        fvm_styxrpc_object_image_destroy(image);
        fputs("Could not publish FlashVM object image\n", stderr);
        return 0;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), FRAME_LIMIT);
    StyxClient *reverse = mux ? styxclientmountphy(phybidirchannel(mux, 1), MSIZE, "object", "", WAIT_MS, TAGS) : NULL;
    StyxRpcServer *server = reverse ? styxrpc_server_create(phybidirchannel(mux, 0),
        &styxrpc_module_tree, fvm_styxrpc_object_image_module(image)) : NULL;
    while (styxrpc_server_step(server))
    {
    }
    int ok = server != NULL;
    styxrpc_server_destroy(server);
    styxclientshutdown(reverse);
    styxclientfree(reverse);
    phybidirfree(mux);
    fvm_styxrpc_object_image_destroy(image);
    return ok;
}


/* [GC:] EN: Use only the attached Dis filesystem and load two verified methods as one object.
 * RU: Использовать только файловую систему Dis и загрузить два проверенных метода одним объектом. */
int test_object_module_call(FVM_Context *context, StyxClient *session)
{
    static const StyxRpcModuleImport imports[] = {
        {"next", STYXRPC_MODULE_INT, STYXRPC_INT_SIGNATURE, {"", ""}},
        {"number", STYXRPC_MODULE_MANAGED, NUMBER_SIGNATURE, {"(fs)", "((fs))"}}
    };
    static FVM_StyxRpcLoadSpec spec;
    static const FVM_StyxRpcModuleCatalog catalog = {&spec, 1};
    spec = (FVM_StyxRpcLoadSpec){session, "mounted/limbo.dis", imports, 2, TEST_MANAGED_PID, WAIT_MS, NULL};
    FVM_Value fs, args[2], result, value, module;
    if (fvm_styxrpc_filesystem_mount(context, "/", session) != FVM_OK ||
        fvm_filesystem_unmount(context, "/") != FVM_UNSUPPORTED ||
        fvm_styxrpc_filesystem_mount(context, "/other", session) != FVM_INVALID_ARGUMENT ||
        fvm_filesystem_install(context, "fs") != FVM_OK ||
        fvm_get_global(context, "fs", &fs) != FVM_OK || fs.type != FVM_VALUE_OBJECT ||
        fvm_value_string(args, "local-only") != FVM_OK ||
        fvm_object_call(context, fs.object, "open", 1, args, &result) == FVM_OK ||
        fvm_value_string(args, "/mounted/limbo.dis/.interface") != FVM_OK ||
        fvm_object_call(context, fs.object, "stat", 1, args, &result) != FVM_OK)
    {
        fputs("Could not mount paired filesystem\n", stderr);
        return 0;
    }
    if (fvm_styxrpc_module_loader_install(context, "loadDis", &catalog) != FVM_OK ||
        fvm_value_string(args, "/.prog/42") != FVM_OK ||
        fvm_call(context, "loadDis", 1, args, &module) != FVM_NOT_FOUND ||
        fvm_value_string(args, "/mounted/limbo.dis") != FVM_OK)
    {
        return 0;
    }
    FVM_Result loaded = fvm_call(context, "loadDis", 1, args, &module);
    if (loaded != FVM_OK || fvm_set_global(context, "limbo", &module) != FVM_OK)
    {
        fprintf(stderr, "Could not load the Dis module (%d), next=%d, number=%d\n", loaded,
            styxrpc_module_check(session, spec.path, imports, 1, spec.pid),
            styxrpc_module_check(session, spec.path, imports + 1, 1, spec.pid));
        return 0;
    }
    fvm_value_number(args, 41);
    if (fvm_object_call(context, module.object, "next", 1, args, &result) != FVM_OK ||
        result.type != FVM_VALUE_NUMBER || result.number != 42)
    {
        fputs("Could not call the module's scalar method\n", stderr);
        return 0;
    }
    fvm_value_number(args, -12.5);
    return fvm_value_string(args + 1, "native") == FVM_OK &&
        fvm_object_call(context, module.object, "number", 2, args, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && fvm_object_get(context, result.object, "0", &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == -12.5 &&
        fvm_object_get(context, result.object, "1", &value) == FVM_OK &&
        value.type == FVM_VALUE_STRING && !strcmp(value.string, "native");
}
