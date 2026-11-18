/* Copyright (c) 2026 bhgv
 * EN: Private per-fetch native scope combines receiver lifetime and caller cancellation without sharing effects.
 * RU: Private per-fetch scope соединяет receiver lifetime и caller cancellation без общих effects.
 */
#ifndef STYX_RESOURCE_FETCH_SCOPE_H
#define STYX_RESOURCE_FETCH_SCOPE_H
#include "cancel.h"


/* EN/RU: Caller owns zeroed storage until leave on the same native thread and LIFO scope order.
 * Caller владеет zeroed storage до leave на том же native thread с LIFO порядком scopes.
 */
typedef struct MovieFetchScope
{
    DVM_Cancel *cancel;      /* Owned independent sticky source for this fetch only. */
    DVM_CancelTap *lifetime; /* Owned receiver-lifetime forwarding tap. */
    DVM_CancelTap *caller;   /* Owned optional outer-caller forwarding tap. */
    DVM_CancelScope current; /* Current native scope owns another pin while entered. */
} MovieFetchScope;


/* EN/RU: Enter an independent source/1, or rollback/0; borrow lifetime and existing caller scope only for setup.
 * Forwarding callbacks retain only the new source; cancellation never flows back into either parent.
 * Войти с independent source/1 либо rollback/0; callbacks держат только новый source, без обратной отмены.
 */
int movie_fetch_scope_enter(MovieFetchScope *scope, DVM_Cancel *lifetime);


/* EN/RU: Consume partially/fully entered scope on original native owner; late callbacks retain their source.
 * Поглотить partially/fully entered scope на исходном native owner; late callbacks удерживают source.
 */
void movie_fetch_scope_leave(MovieFetchScope *scope);
#endif
