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

#include <string.h>
#include <CF/ConfParser.h>
#include <CF/GetWord.h>
#include <CF/Trim.h>

#if TEST_CONF_PARSER

static bool SampleConfigSetter(char const *paramName,
                               char const *paramValue[],
                               void * /*userData*/) {
  s_printf("param: %s", paramName);

  int x = 0;

  while (paramValue[x]) {
    s_printf(" value(%" _S32BITARG_ "): %s ",
                (SInt32) x,
                (char *) paramValue[x]);
    x++;
  }

  s_printf("\n");

  return false;
}

void TestParseConfigFile() {
  ParseConfigFile(false, "qtss.conf", SampleConfigSetter, nullptr);

}

#endif

static void DisplayConfigErr(char const *fname,
                             int lineCount,
                             char const *lineBuff,
                             char const *errMessage) {

  s_printf("- Configuration file error:\n");

  if (lineCount)
    s_printf("  file: %s, line# %i\n", fname, lineCount);
  else
    s_printf("  file: %s\n", fname);

  if (lineBuff)
    s_printf("  text: %s", lineBuff); // lineBuff already includes a \n

  if (errMessage)
    s_printf("  reason: %s\n", errMessage); // lineBuff already includes a \n
}

int ParseConfigFile(bool allowNullValues, char const *fname, CFConfigSetter setter, void *userData) {
  int error = -1;

  Assert(fname);
  Assert(setter);

  if (!fname) return error;
  if (!setter) return error;

  FILE *configFile = fopen(fname, "r");

  //  Assert( configFile );

  if (configFile) {
    SInt32 lineBuffSize = kConfParserMaxLineSize;
    SInt32 wordBuffSize = kConfParserMaxParamSize;

    char lineBuff[kConfParserMaxLineSize];
    char wordBuff[kConfParserMaxParamSize];

    char *next;

    int lineCount = 0;
    do {
//      next = lineBuff;

      // get a line ( fgets adds \n+ 0x00 )
      if (fgets(lineBuff, lineBuffSize, configFile) == nullptr)
        break;

      ++lineCount;
      error = 0; // allow empty lines at beginning of file.

      // trim the leading whitespace
      next = TrimLeft(lineBuff);

      if (*next) { // non-0

        if (*next == '#') {
          // it's a comment
          // probably do nothing in release version?

          //s_printf( "comment: %s" , &lineBuff[1] );

          error = 0;

        } else {
          char *param;

          // grab the param name, quoted or not
          if (*next == '"')
            next = GetQuotedWord(wordBuff, next, wordBuffSize);
          else
            next = GetWord(wordBuff, next, wordBuffSize);

          Assert(*wordBuff);

          param = new char[::strlen(wordBuff) + 1];

          Assert(param);

          if (param) {
            char const *values[kConfParserMaxParamValues + 1];
            int maxValues = 0;

            ::strcpy(param, wordBuff);

            values[maxValues] = nullptr;

            while (maxValues < kConfParserMaxParamValues && *next) {
              // ace
              next = TrimLeft(next);

              if (*next) {
                if (*next == '"')
                  next = GetQuotedWord(wordBuff, next, wordBuffSize);
                else
                  next = GetWord(wordBuff, next, wordBuffSize);

                char *value = new char[::strlen(wordBuff) + 1];

                Assert(value);

                if (value) {
                  ::strcpy(value, wordBuff);

                  values[maxValues++] = value;
                  values[maxValues] = 0;
                }
              }
            }

            if ((maxValues > 0 || allowNullValues) && !(*setter)(param, values, userData)) {
              error = 0;
            } else {
              error = -1;
              if (maxValues > 0)
                DisplayConfigErr(fname, lineCount, lineBuff, "Parameter could not be set.");
              else
                DisplayConfigErr(fname, lineCount, lineBuff, "No value to set.");
            }

            delete[] param;

            maxValues = 0;

            while (values[maxValues]) {
              char **tempValues = (char **) values; // Need to do this to delete a const
              delete[] tempValues[maxValues];
              maxValues++;
            }
          }
        }
      }
    } while (feof(configFile) == 0 && error == 0);

    (void) fclose(configFile);
  }
  //  else {
  //      s_printf("Couldn't open config file at: %s\n", fname);
  //  }

  return error;
}
