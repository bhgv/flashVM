/* FlashVM: C embedding API: flashvm.
 * File: src/capi/flashvm_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/capability_internal.hpp"
#include "../swf/decode.hpp"
#include <new>
#include <cstring>
#include "flashvm/metamethod.h"
extern "C"
{
    /* Compute and return abi version. */
    unsigned long fvm_abi_version(void)
    {
        return FVM_ABI_VERSION;
    }
    /* Initialize config. */
    FVM_Result fvm_config_init(FVM_Config *config)
    {
        if (!config)
            return FVM_INVALID_ARGUMENT;

        /* Computation / Вычисление и обновление состояния. */
        std::memset(config, 0, sizeof(*config));
        config->struct_size = (unsigned long)sizeof(*config);
        return FVM_OK;
    }
    /* Create runtime. */
    FVM_Result fvm_runtime_create(const FVM_Config *config, FVM_Runtime **out_runtime)
    {
        FVM_Runtime *runtime;
        if (!out_runtime)
            return FVM_INVALID_ARGUMENT;
        *out_runtime = 0;
        if (!config || config->struct_size != sizeof(FVM_Config))
            return FVM_INVALID_ARGUMENT;
        if (config->features != 0)
            return FVM_UNSUPPORTED;
        runtime = new (std::nothrow) FVM_Runtime;
        if (!runtime)
            return FVM_OUT_OF_MEMORY;

        /* Computation / Вычисление и обновление состояния. */
        runtime->config = *config;
        *out_runtime = runtime;
        return FVM_OK;
    }
    /* Destroy runtime. */
    void fvm_runtime_destroy(FVM_Runtime *runtime)
    {
        delete runtime;
    }
    /* Create context. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result fvm_context_create(FVM_Runtime *runtime, FVM_Context **out_context)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *context;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!out_context)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out_context = 0;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!runtime)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        try
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            context = new (std::nothrow) FVM_Context;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
        catch (...)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!context)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
        context->exception_pending = context->fault_pending = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        std::memset(&context->fault, 0, sizeof(context->fault));
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_value_undefined(&context->exception_value);

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->broker = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->identity = fvm_sync_next_identity();
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!context->identity)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete context;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_ERROR;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->debug_line = context->debug_offset = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->transfer_resources = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->worker = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->array_builtin_prototype = 0;
        context->standard_library = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->standard_text_library = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->random_state = 1;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->runtime = runtime;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->global = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->swf_loaded = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->script_depth = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->string_depth = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->primitive_depth = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->avm1_execution_depth = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->avm1_steps = 0;
        /* [GC:] Работаем с выбранным режимом планирования сборки. */
        /* [GC:] No constructor/method invocation is active in a new context. */
        context->calling_constructor = context->calling_function = context->calling_owner = 0;
        context->calling_setter_owner = 0;
        context->calling_setter_name = 0;
        /* [GC:] Start with the default collector scheduling mode. */
        context->gc_mode = 0;
        /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
        context->gc_allocations = 0;
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        context->gc_collecting = false;
        /* [GC:] Preserve historical context-long string ownership unless explicitly opted in. */
        context->gc_strings_traced = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->destroying = false;
        /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
        context->gc_releasing = false;
        /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
        context->gc_frames = 0;
        /* [GC:] A new context has no suspended or active display-target activation. */
        context->avm1_target_active = false;
        context->avm1_base_clip = context->avm1_target_clip = 0;
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        context->gc_native_depth = 0;
        /* [GC:] New contexts have no armed finalizer preparation. */
        context->pending_finalizer = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->pumping_input = false;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->input_callback = 0;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->input_userdata = 0;
        /* Initialize stage selection before the new context becomes visible to host callbacks. */
        context->stage_background_set = false;
        context->stage_background.r = context->stage_background.g =
            context->stage_background.b = context->stage_background.a = 0;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_object_create(context, &context->global) != FVM_OK)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete context;
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_OUT_OF_MEMORY;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out_context = context;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Destroy context. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    void fvm_context_destroy(FVM_Context *context)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        if (!context || context->destroying || context->gc_collecting || context->gc_native_depth)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return;

        /* Computation / Вычисление и обновление состояния. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        context->destroying = true;
        /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
        context->gc_collecting = true;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_mutex_context_cleanup(context);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < context->objects.size(); ++i)
            /* [GC:] Вызываем финализатор; он может сделать объект снова достижимым. */
            fvm_meta_finalize(context, context->objects[i]);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < context->objects.size(); ++i)
            /* [GC:] Освобождаем ресурс объекта до удаления его записи из кучи. */
            fvm_release_object_resource(context, context->objects[i]);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < context->objects.size(); ++i)
            /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
            delete context->objects[i];
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (size_t i = 0; i < context->strings.size(); ++i)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            delete[] context->strings[i].data;

        /* Cleanup / Освобождение ресурсов. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_standard_text_cleanup(context);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_capability_context_cleanup(context);
        /* [GC:] Удаляем запись или область корней после завершения работы с ней. */
        delete context;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* Load memory. */
    FVM_Result fvm_load_memory(FVM_Context *context, const void *data, size_t length)
    {
        if (!context || (!data && length))
            return FVM_INVALID_ARGUMENT;
        FVM_SWFInfo info;
        info.struct_size = sizeof(info);

        /* Initialization / Инициализация локального состояния. */
        fvm_detail::vector<unsigned char> decoded;
        unsigned long configured = context->runtime->config.memory_limit_bytes;
        size_t limit = configured ? (size_t)configured : (size_t)FVM_SWF_DEFAULT_LIMIT;
        FVM_Result result = fvm_swf_decode(data, length, limit, decoded, &info);
        if (result != FVM_OK)
            return result;

        /* Computation / Вычисление и обновление состояния. */
        context->swf_bytes.swap(decoded);
        context->swf_info = info;
        context->swf_loaded = true;
        return FVM_OK;
    }
    /* Compute and return advance. */
    FVM_Result fvm_advance(FVM_Context *context, double milliseconds)
    {
        if (!context || milliseconds < 0.0)
            return FVM_INVALID_ARGUMENT;
        return FVM_UNSUPPORTED;
    }
}
