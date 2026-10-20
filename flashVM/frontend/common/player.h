/* Shared desktop application entry; OS adapters supply process arguments only. */
#ifndef FVM_DESKTOP_PLAYER_H
#define FVM_DESKTOP_PLAYER_H
/* Return 0 on help/normal close, 1 on playback failure, 2 on invalid command line.
 * argc must be positive; argv must contain argc non-null pointers to NUL-terminated strings.
 * Invalid pointer entries/counts fail before SDL/file/VM initialization. The caller
 * owns accessible storage throughout the synchronous call; no argv[argc] is read. */
int fvm_desktop_main(int argc, char **argv);
#endif
