/* Copyright (c) 2026 bhgv
 * EN: Native Inferno signature snapshots and typed request admission before any provider preparation I/O.
 * RU: Native snapshots сигнатур Inferno и typed request admission до provider preparation I/O.
 */
#include "../backend.h"
#include "../../../../runtime/vfs/launch/ticket.h"
#include <limits.h>


/* EN/RU: Copy one named unsigned32 signature into an owned contract; explicit failure rather than fabricated metadata.
 * Скопировать одно имя/unsigned32 signature в свой contract; явный отказ вместо fabricated metadata.
 */
static DVM_LaunchContract *one(const char *name, unsigned long signature)
{
    DVM_LaunchSymbol symbol = {name, signature};
    DVM_LaunchContract *contract = dvm_launch_contract_create(&symbol, 1);
    launch_check(contract != NULL, "single contract construction");
    return contract;
}


/* EN/RU: Validate copied storage, exact bits, required-subset matching, bounds and output preservation.
 * Проверить copied storage, exact bits, required-subset matching, bounds и неизменность output при отказе.
 */
static void snapshots(void)
{
    char name[] = "value";
    DVM_LaunchSymbol symbols[] = {{"z", 0xffffffffUL}, {name, 0x80000000UL}, {"a", 0}};
    DVM_LaunchContract *available = dvm_launch_contract_create(symbols, 3);
    DVM_LaunchContract *required = one(name, 0x80000000UL);
    DVM_LaunchContract *wrong = one(name, 1);
    DVM_LaunchContract *missing = one("absent", 0);
    DVM_LaunchContract *empty = dvm_launch_contract_create(NULL, 0);
    DVM_LaunchContract *copy = dvm_launch_contract_clone(available);
    launch_check(available && empty && copy, "native contract snapshots");
    memset(name, 'x', sizeof(name) - 1);
    symbols[0].signature = 1;
    dvm_launch_contract_free(available);
    launch_check(dvm_launch_contract_matches(required, copy) && !dvm_launch_contract_matches(wrong, copy) &&
        !dvm_launch_contract_matches(missing, copy) && dvm_launch_contract_matches(empty, copy) &&
        !dvm_launch_contract_matches(required, empty) && !dvm_launch_contract_matches(NULL, copy) &&
        !dvm_launch_contract_matches(empty, NULL), "native contract subset/signature semantics");
    DVM_LaunchSymbol output = {"unchanged", 123};
    launch_check(dvm_launch_contract_count(copy) == 3 && !dvm_launch_contract_count(NULL) &&
        !dvm_launch_contract_symbol(copy, 3, &output) && !strcmp(output.name, "unchanged") &&
        output.signature == 123 && !dvm_launch_contract_symbol(copy, 0, NULL) &&
        dvm_launch_contract_symbol(copy, 2, &output) && !strcmp(output.name, "z") && output.signature == 0xffffffffUL,
        "native contract output or unsigned32 bits changed");
    DVM_LaunchSymbol duplicate[] = {{"same", 1}, {"same", 2}};
    char oversized[DVM_LAUNCH_SYMBOL_NAME];
    memset(oversized, 'x', sizeof(oversized));
    DVM_LaunchSymbol invalid = {oversized, 1};
    launch_check(!dvm_launch_contract_create(NULL, 1) &&
        !dvm_launch_contract_create(symbols, DVM_LAUNCH_SYMBOL_LIMIT + 1) &&
        !dvm_launch_contract_create(duplicate, 2) && !dvm_launch_contract_create(&invalid, 1), "invalid contract accepted");
    invalid.name = "";
    launch_check(!dvm_launch_contract_create(&invalid, 1), "empty contract member accepted");
    invalid.name = NULL;
    launch_check(!dvm_launch_contract_create(&invalid, 1) && !dvm_launch_contract_clone(NULL), "null contract member accepted");
#if ULONG_MAX > 0xffffffffUL
    invalid.name = "wide";
    invalid.signature = 0x100000000UL;
    launch_check(!dvm_launch_contract_create(&invalid, 1), "signature silently narrowed to unsigned32");
#endif
    dvm_launch_contract_free(copy);
    dvm_launch_contract_free(empty);
    dvm_launch_contract_free(required);
    dvm_launch_contract_free(wrong);
    dvm_launch_contract_free(missing);
}


/* EN/RU: Typed requests remain pending until actual matching exports; copied client storage may retire before owner review.
 * Typed requests сохраняют PENDING до matching exports; copied client storage может завершиться до owner review.
 */
int main(void)
{
    snapshots();
    LaunchState terminal = {.terminal = 1};
    LaunchState provider = {0};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.module, "owner-module");
    strcpy(spec.expected_provider, options.provider_origin.system);
    DVM_LaunchContract *required = one("value", 0x80000000UL);
    DVM_LaunchContract *exports = one("value", 0x80000000UL);
    DVM_LaunchContract *wrong = one("value", 0xffffffffUL);
    spec.contract = required;
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    dvm_launch_contract_free(required);
    spec.contract = exports;
    DVM_LaunchContract *review_copy = dvm_launch_request_contract(request);
    launch_check(review_copy && dvm_launch_contract_matches(review_copy, exports) &&
        !dvm_launch_request_contract(NULL), "owner could not independently inspect copied requirements");
    DVM_LaunchDescription description;
    unsigned roots = provider.roots + terminal.roots;
    launch_check(ticket && request && dvm_launch_describe(request, &description) && description.typed &&
        !dvm_launch_request_prepare(request, options.provider, &options.provider_origin) &&
        provider.roots + terminal.roots == roots && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING,
        "unvalidated typed request prepared an environment");
    launch_check(!dvm_launch_request_check(request, NULL) && !dvm_launch_request_check(request, wrong) &&
        dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING && !dvm_launch_request_take(request),
        "mismatched interface advanced a typed request");
    launch_check(dvm_launch_request_check(request, exports) &&
        dvm_launch_request_prepare(request, options.provider, &options.provider_origin) &&
        !dvm_launch_request_check(request, exports), "matching interface did not gate preparation exactly once");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && handoff->description.typed, "typed request lost copied handoff description");
    dvm_launch_handoff_free(handoff);
    dvm_launch_request_free(request);
    dvm_launch_ticket_free(ticket);
    ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_ticket_cancel(ticket) && !dvm_launch_request_check(request, exports) &&
        !dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "cancelled interface request revived");
    dvm_launch_request_free(request);
    dvm_launch_ticket_free(ticket);
    launch_check(dvm_launch_contract_matches(review_copy, exports), "owner review snapshot died with request");
    dvm_launch_contract_free(review_copy);
    spec.contract = NULL;
    ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_describe(request, &description) && !description.typed &&
        dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "legacy untyped admission changed");
    dvm_launch_request_free(request);
    dvm_launch_ticket_free(ticket);
    dvm_launch_contract_free(exports);
    dvm_launch_contract_free(wrong);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "typed request retained native grants");
    puts("PASS copied Inferno interface requirements gate native launch preparation");
    return 0;
}
