/* Copyright (c) 2026 bhgv
 * EN: Isolated Styx asset provider for the native FlashVM playback fixture.
 * RU: Отдельный провайдер ресурсов Styx для нативной проверки плеера.
 */
#ifndef FVM_TEST_MOVIE_ASSETS_H
#define FVM_TEST_MOVIE_ASSETS_H
#include "styxrpc/serve.h"
#include "client.h"
#include <pthread.h>

typedef struct MovieAssets
{
    pthread_t thread;      /* Joins the fixture VFS server. */
    StyxRpcServer *server; /* Consumed by the thread. */
    StyxClient *client;    /* Borrowed by FlashVM until stop. */
    int running;          /* Thread started successfully. */
} MovieAssets;


/* EN: Start a dedicated Styx server exposing fixture files from share/.
 * RU: Запустить отдельный сервер Styx для файлов проверки из share/. */
int test_movie_assets_start(MovieAssets *provider);


/* EN: Close the client and join the asset server after playback teardown.
 * RU: Закрыть клиента и дождаться сервера после остановки плеера. */
void test_movie_assets_stop(MovieAssets *provider);

#endif
