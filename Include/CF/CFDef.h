//
// Created by james on 8/25/17.
//

#ifndef __CF_DEF_H__
#define __CF_DEF_H__

#include "Types.h"
#include "Revision.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !__Win32__ && !__MinGW__
#include <sys/uio.h>
#endif


//*******************************
// CONSTANTS

#define CF_API_VERSION                0x00050000
#define CF_MAX_MODULE_NAME_LENGTH     64
#define CF_MAX_SESSION_ID_LENGTH      64
#define CF_MAX_ATTRIBUTE_NAME_SIZE    64
#define CF_MAX_URL_LENGTH             512
#define CF_MAX_NAME_LENGTH            128
#define CF_MAX_ATTRIBUTE_NUMS         128


// Network Constants
#define CF_MAX_REQUEST_BUFFER_SIZE    2*1024  // 2k
#define CF_MAX_RESPONSE_BUFFER_SIZE   2048    // 2k


//*******************************
// ENUMERATED TYPES

// Error Codes
#if USE_ENUM
typedef enum {
#else
enum {
#endif
  CF_NoErr = 0,
  CF_RequestFailed = -1,
  CF_Unimplemented = -2,
  CF_RequestArrived = -3,
  CF_OutOfState = -4,
  CF_NotAModule = -5,
  CF_WrongVersion = -6,
  CF_IllegalService = -7,
  CF_BadIndex = -8,
  CF_ValueNotFound = -9,
  CF_BadArgument = -10,
  CF_ReadOnly = -11,
  CF_NotPreemptiveSafe = -12,
  CF_NotEnoughSpace = -13,
  CF_WouldBlock = -14,
  CF_NotConnected = -15,
  CF_FileNotFound = -16,
  CF_NoMoreData = -17,
  CF_AttrDoesntExist = -18,
  CF_AttrNameExists = -19,
  CF_InstanceAttrsNotAllowed = -20,
  CF_UnknowAudioCoder = -21,
#if USE_ENUM
  } CF_Error;
#else
};

typedef SInt32 CF_Error;
#endif

// Events
typedef enum {
  CF_ReadableEvent = 1,
  CF_WriteableEvent = 2
} CF_EventType;

// Flags for QTSS_Write when writing to a QTSS_ClientSessionObject.
typedef enum {
  qtssWriteFlagsNoFlags = 0x00000000,
  qtssWriteFlagsIsRTP = 0x00000001,
  qtssWriteFlagsIsRTCP = 0x00000002,
  qtssWriteFlagsWriteBurstBegin = 0x00000004,
  qtssWriteFlagsBufferData = 0x00000008
} CF_WriteFlags;

struct CF_NetAddr {
  char *ip;
  UInt16 port;
};
typedef struct CF_NetAddr CF_NetAddr;

#ifdef __cplusplus
}
#endif

#endif //__CF_DEF_H__
