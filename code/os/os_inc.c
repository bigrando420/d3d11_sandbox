#include "os_main.c"
#if defined(OS_FEATURE_GFX)
#include "os_gfx.c"
#endif

#if OS_WINDOWS
#include "win32/win32_main.c"
#if defined(OS_FEATURE_GFX)
#include "win32/win32_gfx.c"
#endif
#else
#error OS layer not implemented.
#endif
