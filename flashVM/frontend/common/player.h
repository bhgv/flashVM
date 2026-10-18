/* Shared desktop application entry; OS adapters supply process arguments only. */
#ifndef FVM_DESKTOP_PLAYER_H
#define FVM_DESKTOP_PLAYER_H
/* Return 0 on help/normal close, 1 on playback failure, 2 on invalid command line. */
int fvm_desktop_main(int argc, char **argv);
#endif
