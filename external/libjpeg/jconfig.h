#ifndef FVM_JPEG_CONFIG_H
#define FVM_JPEG_CONFIG_H
/* ANSI C89 profile, independent of autoconf and the host OS. */
#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#define HAVE_LOCALE_H
#ifdef _WIN32
#ifndef __RPCNDR_H__
typedef unsigned char boolean;
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#define HAVE_BOOLEAN
#endif
#ifdef JPEG_INTERNALS
/* Leave INLINE unset: public and implementation source stay in C89. */
#define DEFAULT_MAX_MEM 67108864L
#endif
#endif
