/* Copyright (c) 2026 bhgv
 * EN: Concurrent per-PID caller grants shared by the mount and reverse dispatcher.
 * RU: Параллельные выдачи контекста по PID для маунта и встречного сервера.
 */
#ifndef STYX_CALLER_SCOPE_H
#define STYX_CALLER_SCOPE_H
#include "tree.h"
#include "provider.h"
#include <pthread.h>
#include <stdint.h>

#define STYX_CALLER_SLOTS 16U /* Per-session active process limit. */
#define STYX_CALLER_NODES 512U /* Number of Qids reserved per process generation. */

typedef struct StyxCallerGrant
{
    DVM_Provider *tree; /* Borrowed through the caller lease. */
    uint32_t pid; /* Nonzero session-local process ID. */
    uint64_t generation; /* Never reused during the session. */
} StyxCallerGrant;

typedef struct StyxCallerScopes
{
    pthread_mutex_t lock; /* Protects grants, generation and stopped. */
    StyxCallerGrant active[STYX_CALLER_SLOTS]; /* Sparse concurrently active publications. */
    uint64_t generation; /* Last assigned generation. */
    int stopped; /* Refuse further publication. */
} StyxCallerScopes;


/* EN: Publish a borrowed tree until its matching release. One grant per PID.
 * RU: Опубликовать заимствованное дерево до освобождения. Один контекст на PID. */
int styxcallerscopeadd(StyxCallerScopes *scopes, DVM_Provider *tree, uint32_t pid);

/* EN: Revoke the supplied tree lease.
 * RU: Отозвать указанную выдачу дерева. */
void styxcallerscopedrop(StyxCallerScopes *scopes, DVM_Provider *tree);

/* EN: Find a live grant by PID or generation and optionally pin a copy of its tree.
 * RU: Найти живой контекст по PID или поколению и при необходимости удержать дерево. */
int styxcallerscopefind(StyxCallerScopes *scopes, uint32_t pid, uint64_t generation, StyxCallerGrant *result);

/* EN: Return a snapshot view for listing the portal directories.
 * RU: Вернуть снимок дерева для чтения общих каталогов. */
DVM_Provider *styxcallerscopeview(StyxCallerScopes *scopes);
/* EN/RU: Encode metadata and complete directory records. / Кодировать метаданные и целые записи каталога. */
void styxcallerattributes(const StyxMeta *meta, Qid qid, Dir *dir);
char *styxcallerlist(StyxOpen *opened, char *bytes, ulong *count, vlong offset);
#endif
