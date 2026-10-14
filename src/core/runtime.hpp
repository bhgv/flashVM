/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: runtime.
 * File: src/core/runtime.hpp.
 */

#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
/* Define FLASHVM INTERNAL RUNTIME HPP constant or compilation switch. */
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/value.h"
#include "flashvm/function.h"
#include "flashvm/exception.h"
#include "flashvm/swf.h"
#include "flashvm/input.h"
#include "flashvm/raster.h"
#include "flashvm/input_queue.h"
#include "../details/hash_table.hpp"
#include "../details/string.hpp"
#include "../details/vector.hpp"
struct FVM_FontSource;
struct FVM_Stack;
struct FVM_Worker;
struct FVM_Channel;
struct FVM_ResourceBroker;
typedef FVM_Result (*FVM_StackCallbackInternal)(FVM_Context *, FVM_Stack *, void *);

/* [GC:] Non-owning allocation descriptor; the enclosing context releases each registered buffer. */
struct FVM_ContextString
{
    char *data; /* [GC:] Context-owned terminated byte buffer, including counted embedded NULs. */
    size_t length; /* [GC:] Payload bytes before the separately allocated final terminator. */
    /* [GC:] Initialize an unused vector capacity entry without claiming a buffer. */
    FVM_ContextString() : data(0), length(0)
    {
    }
    /* [GC:] Record buffer ownership and exact byte length atomically in one vector element. */
    FVM_ContextString(char *bytes, size_t count) : data(bytes), length(count)
    {
    }
};

/* Define FVM_StackEntry struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_StackEntry
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    FVM_StackCallbackInternal
        callback;                  /* [GC:] callback: stored callback used by the enclosing API. */
    int (*cfunction)(FVM_Stack *); /* [GC:] Lua-style C function callback. */
    void *userdata;                /* [GC:] Opaque caller data passed to native callbacks. */
    FVM_Object *root;              /* [GC:] Managed object kept alive by this entry. */
    /* Initialize Stack Entry and its owned state. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_StackEntry() : callback(0), cfunction(0), userdata(0), root(0)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Define FVM_NativeEntry struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_NativeEntry
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    bool collector_entry; /* [GC:] Whether this native entry is a collector entry point. */
    FVM_Object *root;     /* [GC:] Managed object kept alive by this entry. */
    /* Initialize Native Entry and its owned state. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry() : collector_entry(false), root(0), callback(0), userdata(0)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    FVM_NativeFunction callback; /* [GC:] callback: stored callback used by the enclosing API. */
    void *userdata;              /* [GC:] Opaque caller data passed to native callbacks. */
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Define FVM_AccessorEntry struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_AccessorEntry
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    FVM_NativeFunction getter /* [GC:] Callback invoked when the property is read. */,
        setter;       /* [GC:] Callback invoked when the property is assigned. */
    void *userdata;   /* [GC:] Opaque caller data passed to native callbacks. */
    FVM_Object *root; /* [GC:] Managed object kept alive by this entry. */
    /* Initialize Accessor Entry and its owned state. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_AccessorEntry() : getter(0), setter(0), userdata(0), root(0)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* [GC:] Context-local generated body attachment; never included in graph serialization. */
struct FVM_ScriptBodyEntry
{
    FVM_ScriptBody callback; /* Host-owned code entry, null selects the interpreter. */
    void *userdata;          /* Host-owned opaque state. */
    FVM_Object *capture;     /* [GC:] Same-context object traced with the function. */
    /* Initialize fresh/deserialized objects without process-local code attachments. */
    FVM_ScriptBodyEntry() : callback(0), userdata(0), capture(0)
    {
    }
};

/* Define FVM_Object struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_Object
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    fvm_detail::map<fvm_detail::string, FVM_Value>
        properties; /* [GC:] Named own property values. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, unsigned int>
        attributes; /* [GC:] Named property attribute flags. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>
        accessors;                                     /* [GC:] Named native getter/setter pairs. */
    fvm_detail::map<unsigned long, FVM_Value> indexed; /* [GC:] Numeric own property values. */
    FVM_Object *metatable; /* [GC:] Metadata object used for metamethod dispatch. */
    FVM_Context *owner;    /* [GC:] Owner responsible for the lifetime of this resource. */
    FVM_Object *prototype; /* [GC:] Prototype used for inherited property lookup. */
    fvm_detail::vector<FVM_Object *>
        implemented_interfaces; /* [GC:] Interface constructors retained by this prototype. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>
        metamethods; /* [GC:] Native metamethod callbacks keyed by name. */
    unsigned long swf_initializer_next; /* Context-local next init block; new graph copies start at zero. */
    bool swf_initializer_running; /* Prevent recursive initialization of the same source library. */
    bool finalized;  /* [GC:] Whether finalization has already run. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    bool gc_marked; /* Reachability mark for the current collection. */
    /* [GC:] Проверяем или инициализируем количество явных корней объекта. */
    unsigned long gc_pins;   /* Explicit host roots retaining this object. */
    FVM_ScriptBodyEntry script_body; /* [GC:] Optional native body and retained capture. */
    FVM_StackEntry callable; /* [GC:] Stack API callable entry. */
    FVM_NativeEntry invocable /* [GC:] Native function invocation entry. */,
        constructable; /* [GC:] Native constructor invocation entry. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Object *>
        method_values; /* [GC:] Cached Function objects representing native methods. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Object *>
        watchers;               /* [GC:] Script functions observing property changes. */
    bool resource_result_valid; /* [GC:] Whether a cached resource operation result is available. */
    FVM_Value resource_result;  /* [GC:] Cached result of a resource operation. */
    unsigned long resource_origin;    /* [GC:] Identity of the originating resource owner. */
    fvm_detail::string resource_path; /* [GC:] File path associated with a transferred resource. */
    int resource_kind;                /* [GC:] Kind of backing resource. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>
        methods;    /* [GC:] Named native methods of this object. */
    void *resource; /* [GC:] Opaque shared or platform resource handle. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    void (*release_resource)(void *); /* [GC:] Callback releasing the attached resource. */
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Define FVM_Runtime struct and its representation. */
struct FVM_Runtime
{
    FVM_Config config; /* Configuration used by this runtime. */
};

/* Define FVM_ScriptFunction struct and its representation. */
struct FVM_ScriptFunction
{
    fvm_detail::string name; /* Name used to identify or look up this entry. */
    fvm_detail::vector<fvm_detail::string>
        constants;                          /* Constant pool captured by the script function. */
    fvm_detail::vector<unsigned char> body; /* Bytecode storage or view for this record. */
    fvm_detail::vector<fvm_detail::string>
        parameters; /* Formal parameter names in declaration order. */

    fvm_detail::vector<unsigned int>
        parameter_registers;     /* Register per parameter; zero selects a named binding. */
    bool function2;              /* Whether this function has private Function2 registers. */
    unsigned int register_count; /* Number of allocated Function2 registers. */
    unsigned int flags; /* Preload/suppression flags retained for calls and graph transfer. */
    /* Initialize legacy function metadata without Function2 flags or registers. */
    FVM_ScriptFunction() : function2(false), register_count(0), flags(0)
    {
    }
};

/* Define FVM_QueuedInput struct and its representation. */
struct FVM_QueuedInput
{
    FVM_InputEvent event; /* Queued input event payload. */
    FVM_InputPhase phase; /* Input event delivery phase. */
};

/* [GC:] Define FVM_GCRootFrame struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_GCRootFrame
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_GCRootFrame *previous;            /* [GC:] Previous frame in the root chain. */
    fvm_detail::vector<FVM_Value> *stack; /* [GC:] Execution stack or stack API handle. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Value>
        *locals;          /* [GC:] Local variables traced as collector roots. */
    FVM_Object *receiver; /* [GC:] Receiver retained during the call. */
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Define FVM_SourceLocation struct and its representation. */
struct FVM_SourceLocation
{
    fvm_detail::string function /* Function name associated with this source location. */,
        source; /* Source name associated with this entry. */
    unsigned long offset /* Byte offset identifying the instruction or data. */,
        line; /* Source line number associated with this entry. */
};

/* Define FVM_Context struct and its representation. */
/* [GC:] Объявляем представление состояния сборщика или корней. */
struct FVM_Context
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
    FVM_Object *calling_constructor; /* [GC:] Constructor identity of the current call mode. */
    FVM_Object *calling_function;    /* [GC:] Function whose selected method owner is recorded. */
    FVM_Object *calling_owner; /* [GC:] Prototype or object that supplied the current method. */
    FVM_Object *calling_setter_owner; /* [GC:] Accessor lookup owner retained by the active setter scope. */
    const fvm_detail::string *calling_setter_name; /* Borrowed immutable key owned by the active setter scope. */
    bool exception_pending /* Whether an ActionScript exception is awaiting handling. */,
        /* [GC:] Работаем с очередью объектов для итеративного обхода графа. */
        fault_pending;         /* Whether structured resource fault information is pending. */
    FVM_Value exception_value; /* [GC:] Pending ActionScript thrown value. */
    FVM_ExceptionInfo fault;   /* [GC:] Structured pending resource fault information. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::string
        resource_receive_path;  /* [GC:] File origin retained while restoring received resources. */
    FVM_ResourceBroker *broker; /* [GC:] Broker serving distributed shared resources. */
    unsigned long identity;     /* [GC:] Stable identity of this shared resource. */
    unsigned long debug_line /* [GC:] Current debugger source line. */,
        debug_offset; /* [GC:] Current debugger bytecode offset. */
    fvm_detail::string debug_source /* [GC:] Current debugger source name. */,
        debug_function; /* [GC:] Current debugger function name. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<FVM_SourceLocation>
        debug_locations;      /* [GC:] Bytecode-to-source mappings registered by the host. */
    void *transfer_resources; /* [GC:] Resource identity registry used by graph transfer. */
    FVM_Worker *worker;       /* [GC:] Worker executing this context, when present. */
    FVM_Object *array_builtin_prototype; /* [GC:] Original Array prototype retained for native result builders. */
    FVM_Object *object_builtin_prototype; /* [GC:] Original Object prototype retained for script constructor prototypes. */
    bool standard_library;    /* [GC:] Whether core standard objects have been installed. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    unsigned int
        avm1_execution_depth; /* [GC:] avm  execution depth: stored state of the enclosing type. */
    unsigned long avm1_steps; /* [GC:] avm  steps: stored state of the enclosing type. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_Object *>
        function_objects; /* [GC:] Registered script Function instances. */
    fvm_detail::vector<FVM_Object *> lexical_scopes; /* [GC:] Captured lexical scope chain. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<FVM_Object *>
        watch_running;          /* [GC:] Watch callbacks currently executing; prevents recursion. */
    bool standard_text_library; /* [GC:] Whether standard text objects have been installed. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_FontSource *>
        fonts;                       /* [GC:] Font sources registered by name. */
    fvm_detail::string default_font; /* [GC:] Default registered font name. */
    unsigned long random_state;      /* [GC:] State of the context-local pseudo-random generator. */
    FVM_Runtime *runtime;            /* [GC:] Runtime owning this context. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_StackEntry>
        stack_functions; /* [GC:] Registered stack API functions. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<fvm_detail::vector<FVM_Value> *>
        /* [GC:] Регистрируем временный стек значений среди корней контекста. */
        gc_stacks;      /* Temporary value stacks exposed to the collector. */
    FVM_Object *global; /* [GC:] Global ActionScript object of this context. */
    fvm_detail::vector<FVM_Object *> objects; /* [GC:] Managed objects owned by the context. */
    fvm_detail::map<size_t, size_t>
        string_allocations; /* [GC:] Buffer address to descriptor index. */
    fvm_detail::vector<FVM_ContextString>
        strings; /* [GC:] Owned strings with exact payload lengths. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>
        functions; /* [GC:] Registered native global functions. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>
        constructors; /* [GC:] Registered native constructors. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_NativeFunction>
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        serialization_callbacks; /* serialization callbacks: stored callback used by the enclosing
                                   API. */
    fvm_detail::vector<unsigned char> swf_bytes; /* [GC:] Stored bytes of the loaded SWF. */
    FVM_SWFInfo swf_info;                        /* [GC:] Parsed metadata of the loaded SWF. */
    bool swf_loaded; /* [GC:] Whether the context has loaded SWF metadata. */
    bool stage_background_set; /* Whether a host or encountered SWF control selected a color. */
    FVM_RasterColor stage_background; /* Straight-alpha RGBA stage color, containing no managed edges. */
    bool avm1_target_active; /* Whether an AVM1 activation has published display target state. */
    FVM_Object *avm1_base_clip; /* [GC:] Base clip of the active AVM1 activation. */
    FVM_Object *avm1_target_clip; /* [GC:] Selected display target; null represents invalid target. */
    FVM_NativeEntry avm1_url; /* [GC:] Host URL request sink retaining its optional capture. */
    FVM_NativeEntry avm1_trace; /* [GC:] Host ActionTrace sink retaining its optional capture. */
    fvm_detail::vector<FVM_InputRule>
        input_rules; /* [GC:] Rules mapping input events to callbacks. */
    fvm_detail::vector<FVM_QueuedInput> queued_input; /* [GC:] Input events awaiting delivery. */
    bool pumping_input; /* [GC:] Whether input delivery is already in progress. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_InputCallback
        input_callback;   /* [GC:] input callback: stored callback used by the enclosing API. */
    void *input_userdata; /* [GC:] Host data passed to input callbacks. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>
        script_functions;         /* [GC:] Named script function bytecode records. */
    unsigned int script_depth;    /* [GC:] Current nested script invocation depth. */
    FVM_Object *script_callee;    /* [GC:] Active script function identity for arguments.caller. */
    FVM_Object *script_this;      /* [GC:] Current object-valued implicit this, separate from call receiver. */
    unsigned int string_depth;    /* [GC:] Recursion guard for string coercion. */
    unsigned int primitive_depth; /* [GC:] Recursion guard for primitive coercion. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::vector<FVM_Object *>
        joining_arrays; /* [GC:] Arrays currently being joined; prevents cycles. */
    /* [GC:] Работаем с выбранным режимом планирования сборки. */
    int gc_mode; /* Collector scheduling mode. */
    /* [GC:] Учитываем выделения для планирования следующего прохода сборки. */
    unsigned long gc_allocations; /* Allocations charged since the last collection. */
    /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
    bool gc_collecting;     /* Whether collection is currently running. */
    bool gc_strings_traced; /* [GC:] Opt-in reclamation of unreachable context string buffers. */
    bool destroying;        /* [GC:] Whether context destruction is in progress. */
    /* [GC:] Учитываем фазу освобождения ресурсов сборщиком. */
    bool gc_releasing; /* Whether collector resource release is in progress. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    FVM_GCRootFrame *gc_frames; /* [GC:] Linked active collector root frames. */
    /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
    unsigned int gc_native_depth;  /* Current native-call collector barrier depth. */
    FVM_Object *pending_finalizer; /* [GC:] Resolved finalizer whose user-code entry is not yet
                                      committed. */
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
};

/* Allocate a process-local shared resource identity under the registry lock. */
extern "C" unsigned long fvm_sync_next_identity();

/* Update debug location. */
void fvm_debug_update_location(FVM_Context *, unsigned long);

/* Register a channel wait and retain its debugger source location. */
void *fvm_debug_channel_wait_begin(FVM_Context *, FVM_Channel *, int);

/* Wait for debug channel end. */
void fvm_debug_channel_wait_end(void *);

/* Drop mutex registry entries belonging to the destroyed context. */
void fvm_mutex_context_cleanup(FVM_Context *);

/* Release standard text state owned by the destroyed context. */
void fvm_standard_text_cleanup(FVM_Context *);
/* Shared bounded method lookup for C API and AVM1. Noncallable own properties
 * shadow inherited callbacks; NOT_FOUND permits legacy named-function fallback. */
/* Look up native method. */
FVM_Result fvm_lookup_native_method(FVM_Object *, const fvm_detail::string &, FVM_NativeEntry *);

/* Map a native numeric callback to its stable serialization identifier. */
unsigned long fvm_numeric_callback_id(FVM_NativeFunction);

/* Restore a native numeric callback from its stable serialization identifier. */
FVM_NativeFunction fvm_numeric_callback_from_id(unsigned long);

/* Map a native text callback to its stable serialization identifier. */
unsigned long fvm_text_callback_id(FVM_NativeFunction);

/* Restore a native text callback from its stable serialization identifier. */
FVM_NativeFunction fvm_text_callback_from_id(unsigned long);

/* Release object resource. */
/* [GC:] Освобождаем ресурс объекта до удаления его записи из кучи. */
FVM_Result fvm_release_object_resource(FVM_Context *, FVM_Object *);

/* Validate value. */
FVM_Result fvm_validate_value(FVM_Context *, const FVM_Value *);
/* Named lookup without the canonical-index bridge prevents recursive aliasing
 * when indexed access encounters a named getter/setter or metatable hook. */
/* Read object no index. */
FVM_Result fvm_object_get_no_index(FVM_Context *, FVM_Object *, const fvm_detail::string &, FVM_Value *);

/* Update object no index. */
FVM_Result fvm_object_set_no_index(FVM_Context *, FVM_Object *, const fvm_detail::string &, const FVM_Value *);

/* [GC:] Adopt a freshly allocated terminated buffer and exact byte count; caller frees on failure.
 */
void fvm_context_adopt_string(FVM_Context *, char *, size_t);
/* [GC:] Remove a buffer ownership index before explicit rollback frees its allocation. */
void fvm_context_forget_string(FVM_Context *, const char *);
/* [GC:] Preserve successful native string results in context storage before receiver roots unwind.
 */
FVM_Result fvm_native_result(FVM_Context *, FVM_Result, FVM_Value *);
#endif
