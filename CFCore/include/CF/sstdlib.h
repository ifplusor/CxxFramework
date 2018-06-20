/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2008 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
    File:       SafeStdLib.h

    Contains:   Thread safe std lib calls

*/

#ifndef __INTERNAL_STDLIB_H__
#define __INTERNAL_STDLIB_H__

#include <time.h>

#include <CF/Types.h>

#define kTimeStrSize 32
#define kErrorStrSize 256

#ifdef __cplusplus
extern "C" {
#endif

extern int s_maxprintf(char const *fmt, ...);
extern void s_setmaxprintfcharsinK(UInt32 newMaxCharsInK);
extern UInt32 s_getmaxprintfcharsinK();

#ifndef USE_DEFAULT_STD_LIB

#include <stdio.h>

#ifdef __USE_MAX_PRINTF__
#define s_printf s_maxprintf
#else
extern int s_printf(char const *fmt, ...);

#endif

extern int s_sprintf(char *buffer, char const *fmt, ...);
extern int s_fprintf(FILE *file, char const *fmt, ...);
extern int s_snprintf(char *str, size_t size, char const *format, ...);
extern size_t s_strftime(char *buf,
                         size_t maxsize,
                         char const *format,
                         const struct tm *timeptr);

// These calls return the pointer passed into the call as the result.

extern char *s_strerror(int errnum, char *buffer, int buffLen);
extern char *s_ctime(const time_t *timep, char *buffer, int buffLen);
extern char *s_asctime(const struct tm *timeptr, char *buffer, int buffLen);
extern struct tm *s_gmtime(const time_t *, struct tm *result);
extern struct tm *s_localtime(const time_t *, struct tm *result);

#else // !USE_DEFAULT_STD_LIB

#include <stdio.h>

#define s_sprintf sprintf
#define s_fprintf fprintf

#ifdef __USE_MAX_PRINTF__
#define s_printf s_maxprintf
#else
#define s_printf printf
#endif

#if __Win32__
#define s_snprintf _snprintf
#else
#define s_snprintf snprintf
#endif

#define s_strftime strftime

// Use our calls for the following.
// These calls return the pointer passed into the call as the result.

extern char* s_strerror(int errnum, char* buffer, int buffLen);
extern char* s_ctime(const time_t* timep, char* buffer, int buffLen);
extern char* s_asctime(const struct tm* timeptr, char* buffer, int buffLen);
extern struct tm* s_gmtime(const time_t*, struct tm* result);
extern struct tm* s_localtime(const time_t*, struct tm* result);

#endif // !USE_DEFAULT_STD_LIB

#ifdef __cplusplus
};
#endif

#endif // __INTERNAL_STDLIB_H__
