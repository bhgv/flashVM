/* Copyright (c) 2026 bhgv
 * EN: Native state shared by reverse Styx and resident module owners.
 * RU: Нативное состояние для встречного Styx и владельцев модулей.
 */
#ifndef DVM_LIVE_MODULES_PRIVATE_H
#define DVM_LIVE_MODULES_PRIVATE_H
#include "live_modules.h"
#include "unibus_server.h"
#include "styxrpc/vfs_client.h"
#include "styxrpc/live_module.h"
#include <pthread.h>
#include <stdint.h>

#define DVM_LIVE_MODULE_LIMIT 32U /* Maximum concurrent owners or pending loads. */
#define DVM_LIVE_MODULE_OWNER "/.dis-owner" /* Private mailboxes inside a worker namespace. */
#define DVM_LIVE_MODULE_WIRE "mounted/live.dis" /* Private internal Styx directory. */

typedef enum LiveModulePhase
{
    LIVE_UNUSED,  /* Unallocated slot. */
    LIVE_QUEUED,  /* Request received, awaiting the managed controller. */
    LIVE_BOOTING, /* Controller scheduled the resident. */
    LIVE_STAGING, /* Private routes are ready for the resident's offer writes. */
    LIVE_READY,   /* Exact callable directory published. */
    LIVE_FAILED   /* Terminal refusal or retirement. */
} LiveModulePhase;

typedef struct LiveModule
{
    LiveModulePhase phase; /* Guarded by session lock. */
    int pid;               /* Exact resident PID assigned on startup. */
    uint32_t generation;   /* Monotonic ID, zero only in a free slot. */
    int leased;            /* Control fid still owns this generation. */
    int worker;            /* A managed launch or owner still borrows this slot. */
    int breaker;           /* Owned shutdown descriptor or -1. */
    StyxUnibusServer *server; /* Owned internal dispatcher through joined cleanup. */
    DVM_Provider *root;       /* Owned module directory. */
    char path[STYXRPC_VFS_PATH_MAX + 2]; /* Absolute guest path, fixed at admission. */
} LiveModule;

typedef struct LiveModules
{
    pthread_mutex_t lock; /* Protects published slots and stops. */
    pthread_cond_t changed; /* Signals request completion. */
    DVM_Namespace *group; /* Borrowed from the mount command. */
    DVM_Provider *reverse; /* Owned forward-lane binding for scoped calls. */
    struct StyxClient *client; /* Owned transport pin. */
    DVM_Provider *control; /* Owned request file provider. */
    DVM_Provider *empty; /* Owned empty hidden directory. */
    uint32_t last; /* Last assigned generation, never reused. */
    int stopped; /* Admission closed. */
    int primary; /* Initial guest PID, set before dispatch. */
    int listener; /* Dedicated listener PID, guarded by lock. */
    LiveModule entries[DVM_LIVE_MODULE_LIMIT]; /* Stable native storage through joined cleanup. */
} LiveModules;

/* EN: Borrow the published session until the runtime and dispatcher join.
 * RU: Заимствовать опубликованную сессию до завершения VM и диспетчера. */
LiveModules *dvm_live_modules_get(void);

/* EN: Close native routes for a detached slot after all managed callbacks retire.
 * RU: Закрыть нативные маршруты снятого слота после завершения колбэков VM. */
void dvm_live_module_release(LiveModule *entry);


/* EN/RU: Create the control provider borrowing session. / Создать провайдер запросов. */
DVM_Provider *dvm_live_module_control(LiveModules *session);
/* EN/RU: Validate canonical visible relative path. / Проверить канонический путь. */
int dvm_live_module_path(const char *path);
/* EN/RU: Serialize the fixed-width ID. / Записать ID фиксированной ширины. */
void dvm_live_module_id(char *output, uint32_t id);
/* EN/RU: Resolve an exact generation from one component. / Разрешить точное поколение. */
uint32_t dvm_live_module_parse(const char *input);

/* EN: Prepare the calling resident's routes from its copied offer sequence.
 * RU: Подготовить маршруты вызывающего владельца из копии его предложений. */
int dvm_live_module_prepare(DVM_Namespace *group, int pid, const void *offers, size_t size);

/* EN: Commit the calling resident's directory after all matched offers arrive.
 * RU: Опубликовать каталог вызывающего после получения всех совпавших предложений. */
int dvm_live_module_commit(int pid);

/* EN/RU: Internal resident loop. / Цикл приёмника. */
void dvm_live_modules_listen(int pid);

#endif
