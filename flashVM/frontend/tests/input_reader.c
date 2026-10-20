/* SDL memory-backed streams exercise file growth/short read/EOF error/close cleanup. */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>
static const unsigned char data[2]={65,66};
static Sint64 advertised;
static int available, closes, reads, probe_error, close_error, open_error;
static SDL_RWops *test_open(const char *path,const char *mode)
{ (void)path;(void)mode;return open_error?NULL:SDL_RWFromConstMem(data,available); }
static Sint64 test_size(SDL_RWops *file)
{ (void)file;return advertised; }
static size_t test_read(SDL_RWops *file,void *out,size_t width,size_t count)
{
    ++reads;
    if(probe_error && reads>1){SDL_SetError("injected EOF read error");return 0;}
    return SDL_RWread(file,out,width,count);
}
static int test_close(SDL_RWops *file)
{
    int result=SDL_RWclose(file);++closes;
    if(close_error){SDL_SetError("injected close error");return -1;}
    return result;
}
#define SDL_RWFromFile test_open
#define SDL_RWsize test_size
#define SDL_RWread test_read
#define SDL_RWclose test_close
#include "../common/player.c"
#undef SDL_RWFromFile
#undef SDL_RWsize
#undef SDL_RWread
#undef SDL_RWclose
static void check(int success)
{
    size_t size=91;unsigned char *bytes;
    closes=reads=0;bytes=read_movie("memory-fixture",&size);
    assert(closes==(open_error?0:1));
    if(success){assert(bytes && size==(size_t)advertised && bytes[0]==65);free(bytes);}
    else assert(!bytes && size==91);
}
int main(void)
{
    FVM_DesktopBuildInfo build_info;
    char *null_program[] = {NULL, "--help"};
    char *null_file[] = {"player", NULL};
    char *null_operand[] = {"player", "--fps", NULL};
    assert(fvm_desktop_build_info(NULL) == FVM_INVALID_ARGUMENT);
    assert(fvm_desktop_build_info(&build_info) == FVM_OK);
    assert(build_info.version && build_info.abi_version == fvm_abi_version());
    assert(build_info.pointer_bits == sizeof(void *) * CHAR_BIT);
    assert(fvm_desktop_main(-1, NULL) == 2);
    assert(fvm_desktop_main(0, NULL) == 2);
    assert(fvm_desktop_main(1, NULL) == 2);
    assert(fvm_desktop_main(2, null_program) == 2);
    assert(fvm_desktop_main(2, null_file) == 2);
    assert(fvm_desktop_main(3, null_operand) == 2);
    assert(reads == 0 && closes == 0);
    available=2;advertised=2;check(1);
    SDL_SetError("stale unrelated error");check(1);
    advertised=1;check(0);
    advertised=2;available=1;check(0);
    advertised=0;check(0);
    advertised=-1;check(0);
    advertised=(Sint64)FVM_SWF_DEFAULT_LIMIT+1;check(0);
    available=2;advertised=2;probe_error=1;check(0);probe_error=0;
    close_error=1;check(0);close_error=0;
    open_error=1;check(0);
    return 0;
}
