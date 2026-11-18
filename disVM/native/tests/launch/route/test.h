/* Copyright (c) 2026 bhgv
 * EN: Native VFS-route test capabilities and copied request checks, without code-image reads.
 * RU: Native VFS-route test capabilities и проверки copied request, без чтения code image.
 */
#ifndef LAUNCH_ROUTE_TEST_H
#define LAUNCH_ROUTE_TEST_H
#include "../backend.h"
#include "../../../../runtime/vfs/launch/route.h"


/* EN/RU: Two independent source-owner admission services; all wrappers live until the fixture joins their users.
 * Два независимых admission services source-owners; wrappers живут до join users.
 */
typedef struct Routes
{
    DVM_Provider *terminal; /* Borrowed known source capability, not an identity-string lookup. */
    DVM_Provider *provider; /* Borrowed opposite source capability. */
    DVM_LaunchEndpoint *client; /* Borrowed terminal-side submit capability. */
    DVM_LaunchEndpoint *server; /* Borrowed provider-side submit capability. */
    DVM_Cancel *cancel; /* Optional borrowed source requested at resolver boundary. */
    unsigned calls; /* Resolver invocation count, main-thread-only in basic tests. */
    int refuse; /* Explicit final refusal after authoritative source selection. */
} Routes;


/* EN/RU: Construct valid caller spec but deliberately unusable module/destination fields; target must replace both.
 * Создать valid caller spec с намеренно unusable module/destination fields; target заменяет оба.
 */
static DVM_LaunchSpec route_input(DVM_LaunchOptions *options)
{
    DVM_LaunchSpec spec = {0};
    spec.policy = options->policy;
    spec.terminal = options->terminal;
    spec.terminal_origin = options->terminal_origin;
    memset(spec.module, 'x', sizeof(spec.module));
    memset(spec.expected_provider, 'x', sizeof(spec.expected_provider));
    return spec;
}


/* EN/RU: Inspect one actual queued review and copied binary arguments, release review/ticket afterward.
 * Прочитать actual queued review и copied binary arguments, затем освободить review/ticket.
 */
static void received(DVM_LaunchService *owner, DVM_LaunchTicket *ticket, const char *module, const char *system)
{
    DVM_LaunchReview *review = dvm_launch_service_next(owner);
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    DVM_LaunchDescription description;
    unsigned char bytes[4] = {0};
    size_t copied = 0;
    launch_check(ticket && review && dvm_launch_describe(request, &description) &&
        !strcmp(description.module, module) && !strcmp(description.expected_provider, system),
        "route did not preserve capability destination or owner module label");
    launch_check(dvm_launch_arguments(request, 0, bytes, sizeof(bytes), &copied) &&
        copied == 4 && bytes[0] == 0 && bytes[1] == 255 && bytes[2] == 128 && bytes[3] == 1,
        "route did not copy caller binary startup bytes");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(ticket);
}
#endif
