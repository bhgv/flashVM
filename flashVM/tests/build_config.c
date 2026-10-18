#include "flashvm_build_config.h"
#include <assert.h>

#if (defined(GRAPHICS_AGG) + defined(GRAPHICS_CAIRO) + defined(NO_GRAPHICS)) != 1
#error Rasterizer keys must be exclusive
#endif
#if (defined(WINDOWS) + defined(LINUX) + defined(MAC) + defined(ANDROID)) != 1
#error Resolved platform keys must be exclusive
#endif
#if (defined(SQLITE) + defined(NO_SQLITE)) != 1
#error SQLite keys must be exclusive
#endif
#if defined(GRAPHICS_AGG) && !defined(FVM_HAS_AGG)
#error AGG key must select the actual library
#endif
#if defined(GRAPHICS_CAIRO) && !defined(FVM_HAS_CAIRO)
#error Cairo key must select the actual library
#endif
#if defined(SQLITE) && !defined(FVM_HAS_SQLITE)
#error SQLite key must select the actual library
#endif
int main(void)
{
    assert(HEADLESS == 1);
    return 0;
}
