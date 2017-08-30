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

#ifndef __CONF_PARSER_H__
#define __CONF_PARSER_H__

#define TEST_CONF_PARSER 0

#include <CF/Types.h>

// the maximum size + 1 of a parameter
#define kConfParserMaxParamSize 512

// the maximum size + 1 of single line in the file 
#define kConfParserMaxLineSize 1024

// the maximum number of values per config parameter
#define kConfParserMaxParamValues 10

#if TEST_CONF_PARSER
void TestParseConfigFile();
#endif

typedef bool(*CFConfigSetter)(const char *paramName,
                            const char *paramValue[],
                            void *userData);

int ParseConfigFile(bool allowNullValues,
                    const char *fname,
                    CFConfigSetter setter,
                    void *userData);

#endif // __CONF_PARSER_H__