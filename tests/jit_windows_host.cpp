/* Compile actual backend selection under controlled Windows macros on an x64 build. */
#include "../src/jit/internal.hpp"
#include <cassert>
#include <cstring>
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(__ILP32__) && !defined(_M_ARM64EC)
#ifdef __linux__
#define TEST_RESTORE_LINUX
#undef __linux__
#endif
#ifndef _WIN32
#define TEST_RESTORE_WIN32
#define _WIN32 1
#endif
#define fvm_jit_host_target fixture_windows_host_target
#define fvm_jit_target_width fixture_windows_target_width
#define fvm_jit_emit fixture_windows_emit
#define fvm_jit_backend_name fixture_windows_backend_name
#include "../src/jit/backend.cpp"
#undef fvm_jit_host_target
#undef fvm_jit_target_width
#undef fvm_jit_emit
#undef fvm_jit_backend_name
#ifdef TEST_RESTORE_WIN32
#undef _WIN32
#endif
#ifdef TEST_RESTORE_LINUX
#define __linux__ 1
#endif
#endif
int main()
{
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(__ILP32__) && !defined(_M_ARM64EC)
 FVM_JitTarget selected=fixture_windows_host_target();
 assert(selected==FVM_JIT_TARGET_WINDOWS_X64);
 assert(!std::strcmp(fixture_windows_backend_name(),"windows-x64"));
 assert(fixture_windows_target_width(selected)==8);
 /* Check the linked production function too, not only the macro-specialized copy. */
 assert(fvm_jit_target_width(selected)==sizeof(void *) && sizeof(FVM_JitStep)==sizeof(void *));
#endif
 return 0;
}
