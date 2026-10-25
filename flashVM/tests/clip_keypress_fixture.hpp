/* Independent retained KeyPress fixtures reuse the strict placement-wire builder. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_fixture.hpp"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"


/* Load a pinned scene whose one sprite has a selected raw KeyPress record. */
static FVM_Object *keypress_root(FVM_Context *c, unsigned version = 8, unsigned key = 1, const Bytes &code = action(42))
{
    unsigned long flag = version < 6 ? 64 : 0x20000;
    FVM_Object *root = clip_load(c, clip_movie(version, clip_event_table(version, flag, code, key)));
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}


/* Encode a no-argument native global call from retained AVM1 event bytecode. */
static Bytes keypress_call(const char *name)
{
    Bytes code;
    integer(code, 0);
    text(code, name);
    code.push_back(0x3d);
    code.push_back(0x17);
    code.push_back(0);
    return code;
}
