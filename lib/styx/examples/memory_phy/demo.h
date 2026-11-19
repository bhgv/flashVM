/* Copyright (c) 2026 bhgv
 * EN: Native owner startup contract for the original libstyx PHY server example.
 * RU: Контракт запуска native owner исходного libstyx PHY server примера.
 * standalone-reviewed: callers never touch an active server or its fids directly.
 */
#ifndef EXAMPLE_MEMORY_SERVER_H
#define EXAMPLE_MEMORY_SERVER_H
#include "memory.h"
#include <pthread.h>


typedef struct DemoServer
{
    RemotePhy *phy;       /* Main owns before pthread_create; thread consumes it on successful startup. */
    pthread_mutex_t lock; /* Protect only the one-time startup publication. */
    pthread_cond_t ready; /* Main waits for actual initialization, not a guessed scheduling delay. */
    int initialized;      /* Set even on failed setup, so main cannot wait forever for readiness. */
    int success;          /* One-time startup result, published under lock. */
    int clean;            /* Terminal cleanup result; main reads only after join. */
} DemoServer;


/* EN: state borrows DemoServer through join; consumes its phy on every outcome.
 * Return NULL after peer disconnect and original server cleanup; no socket/listener/VM.
 * RU: state заимствует DemoServer до join; phy поглощается при любом исходе.
 * Вернуть NULL после peer disconnect и cleanup исходного сервера; без socket/listener/VM.
 */
void *memory_server(void *state);
#endif
