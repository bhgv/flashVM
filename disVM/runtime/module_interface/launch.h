/* Copyright (c) 2026 bhgv
 * EN: Recheck the actual loaded target against inherited launch requirements before returning its Module to linking.
 * RU: Повторно сверить actual loaded target с inherited launch requirements до передачи Module в linking.
 */
#ifndef DVM_MODULE_INTERFACE_LAUNCH_H
#define DVM_MODULE_INTERFACE_LAUNCH_H
struct Module;


/* [GC:] EN/RU: Consume owned module from readmod, return same owner/NULL on a required interface mismatch or snapshot OOM.
 * path is borrowed caller-visible load name. On ACTIVE owner, only exact launch-label matches have this extra check;
 * ordinary dependencies/aliases/builtins retain normal Inferno link checking. No path/provider lookup or guest execution.
 * Caller owns returned Module; failure unloads input and sets native error, never retries another source.
 * Поглотить owned Module от readmod; вернуть тот же owner/NULL при required mismatch либо snapshot OOM.
 * path — borrowed load name; на ACTIVE owner дополнительная проверка только для exact launch-label match.
 * Dependencies/aliases/builtins сохраняют обычную Inferno linking проверку. Без lookup/guest execution/retry;
 * failure выполняет unload input и устанавливает error; caller владеет returned Module.
 */
struct Module *dvm_launch_module_checked(const char *path, struct Module *module);
#endif
