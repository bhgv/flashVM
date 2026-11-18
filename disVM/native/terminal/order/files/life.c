/* Copyright (c) 2026 bhgv
 * EN: Explicit native file-set owner, nonjoining provider pins and final lifecycle-only broker disposal.
 * RU: Явный native owner file-set, pins provider без join и освобождение broker только lifecycle-owner.
 */
#include "internal.h"


/* EN/RU: See files.h; preserve ordinary owned broker rollback on allocation/lock failure.
 * См. files.h; сохранить rollback owned broker при отказе allocation/lock.
 */
StyxOrderedFiles *styxorderedfiles(StyxOrdered *peer)
{
    StyxOrderedFiles *files = peer ? calloc(1, sizeof(*files)) : NULL; /* Private native owner. */
    if (!files || pthread_mutex_init(&files->lock, NULL))
    {
        free(files);
        styxorderedfree(peer);
        return NULL;
    }
    files->peer = peer;
    return files;
}


/* EN/RU: See files.h; no provider/namespace lock held while notifying native broker consumers.
 * См. files.h; provider/namespace locks не удерживаются при уведомлении native-потребителей broker.
 */
void styxorderedfilesstop(StyxOrderedFiles *files)
{
    if (files)
    {
        styxorderedstop(files->peer);
    }
}


/* EN/RU: See files.h; failed early disposal never cancels a live export.
 * См. files.h; преждевременный free не отменяет живой export.
 */
int styxorderedfilesfree(StyxOrderedFiles *files)
{
    if (!files)
    {
        return 1;
    }
    pthread_mutex_lock(&files->lock);
    int busy = files->factories != 0; /* Factory pins include every open wrapper. */
    pthread_mutex_unlock(&files->lock);
    if (busy)
    {
        return 0;
    }
    styxorderedfree(files->peer);
    pthread_mutex_destroy(&files->lock);
    free(files);
    return 1;
}
