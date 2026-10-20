/* C89/C++98 entrypoint of the example shared resource module. */
#ifndef EXAMPLE_EMBEDDED_MODULE_H /* Protect repeated inclusion. */
#define EXAMPLE_EMBEDDED_MODULE_H /* Module interface owns no VM object. */
#if defined(EXAMPLE_EMBEDDED_STATIC) /* Consume/build the ordinary archive variant. */
#define EXAMPLE_EMBEDDED_API /* Static linkage needs no DLL decoration. */
#elif defined(_WIN32) && defined(EXAMPLE_EMBEDDED_BUILD) /* Build the Windows DLL. */
#define EXAMPLE_EMBEDDED_API __declspec(dllexport) /* Export the module entry. */
#elif defined(_WIN32) /* Consume the Windows import library. */
#define EXAMPLE_EMBEDDED_API __declspec(dllimport) /* Import the module entry. */
#else /* ELF/Mach-O use the normal default symbol visibility. */
#define EXAMPLE_EMBEDDED_API /* No Windows decoration on other targets. */
#endif /* Finish native symbol decoration. */
#ifdef __cplusplus /* C++ applications link the same C symbol. */
extern "C" { /* Begin C linkage. */
#endif /* End language selection. */
/* Call on the native UI thread; return the player status after two muted ticks. */
EXAMPLE_EMBEDDED_API int embedded_movie_play(void); /* Synchronous, no retained caller state. */
#ifdef __cplusplus /* Close only the C++ linkage block. */
} /* End C linkage. */
#endif /* End language selection. */
#endif /* End the example module interface. */
