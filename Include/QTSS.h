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

#ifndef QTSS_H
#define QTSS_H
#include "CF/Types.h"
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __Win32__
#include <sys/uio.h>
#endif

#define QTSS_API_VERSION                0x00050000
#define QTSS_MAX_MODULE_NAME_LENGTH     64
#define QTSS_MAX_SESSION_ID_LENGTH      64
#define QTSS_MAX_ATTRIBUTE_NAME_SIZE    64
#define QTSS_MAX_URL_LENGTH                512
#define QTSS_MAX_NAME_LENGTH            128
#define QTSS_MAX_REQUEST_BUFFER_SIZE    2*1024
#define EASY_ACCENCODER_BUFFER_SIZE_LEN    16*1024*4
#define QTSS_MAX_ATTRIBUTE_NUMS            128

#define EASY_KEY_SPLITER                "-"

//*******************************
// ENUMERATED TYPES

/**********************************/
// Error Codes

enum {
  QTSS_NoErr = 0,
  QTSS_RequestFailed = -1,
  QTSS_Unimplemented = -2,
  QTSS_RequestArrived = -3,
  QTSS_OutOfState = -4,
  QTSS_NotAModule = -5,
  QTSS_WrongVersion = -6,
  QTSS_IllegalService = -7,
  QTSS_BadIndex = -8,
  QTSS_ValueNotFound = -9,
  QTSS_BadArgument = -10,
  QTSS_ReadOnly = -11,
  QTSS_NotPreemptiveSafe = -12,
  QTSS_NotEnoughSpace = -13,
  QTSS_WouldBlock = -14,
  QTSS_NotConnected = -15,
  QTSS_FileNotFound = -16,
  QTSS_NoMoreData = -17,
  QTSS_AttrDoesntExist = -18,
  QTSS_AttrNameExists = -19,
  QTSS_InstanceAttrsNotAllowed = -20,
  QTSS_UnknowAudioCoder = -21
};
typedef SInt32 QTSS_Error;

// QTSS_AddStreamFlags used in the QTSS_AddStream Callback function
enum {
  qtssASFlagsNoFlags = 0x00000000,
  qtssASFlagsAllowDestination = 0x00000001,
  qtssASFlagsForceInterleave = 0x00000002,
  qtssASFlagsDontUseSlowStart = 0x00000004,
  qtssASFlagsForceUDPTransport = 0x00000008
};
typedef UInt32 QTSS_AddStreamFlags;

// QTSS_PlayFlags used in the QTSS_Play Callback function.
enum {
  qtssPlayFlagsSendRTCP =
  0x00000010,   // have the server generate RTCP Sender Reports
  qtssPlayFlagsAppendServerInfo =
  0x00000020    // have the server append the server info APP packet to your RTCP Sender Reports
};
typedef UInt32 QTSS_PlayFlags;

// Flags for QTSS_Write when writing to a QTSS_ClientSessionObject.
enum {
  qtssWriteFlagsNoFlags = 0x00000000,
  qtssWriteFlagsIsRTP = 0x00000001,
  qtssWriteFlagsIsRTCP = 0x00000002,
  qtssWriteFlagsWriteBurstBegin = 0x00000004,
  qtssWriteFlagsBufferData = 0x00000008
};
typedef UInt32 QTSS_WriteFlags;

// Flags for QTSS_SendStandardRTSPResponse
enum {
  qtssPlayRespWriteTrackInfo = 0x00000001,
  qtssSetupRespDontWriteSSRC = 0x00000002
};

// Flags for the qtssRTSPReqAction attribute in a QTSS_RTSPRequestObject.
enum {
  qtssActionFlagsNoFlags = 0x00000000,
  qtssActionFlagsRead = 0x00000001,
  qtssActionFlagsWrite = 0x00000002,
  qtssActionFlagsAdmin = 0x00000004,
  qtssActionFlagsExtended = 0x40000000,
  qtssActionQTSSExtended = 0x80000000,
};
typedef UInt32 QTSS_ActionFlags;

enum {
  easyRedisActionDelete = 0,
  easyRedisActionSet = 1
};
typedef UInt32 Easy_RedisAction;

/**********************************/
// RTP SESSION STATES
//
// Is this session playing, paused, or what?
enum {
  qtssPausedState = 0,
  qtssPlayingState = 1
};
typedef UInt32 QTSS_RTPSessionState;

//*********************************/
// CLIENT SESSION CLOSING REASON
//
// Why is this Client going away?
enum {
  qtssCliSesCloseClientTeardown = 0, // QTSS_Teardown was called on this session
  qtssCliSesCloseTimeout = 1, // Server is timing this session out
  qtssCliSesCloseClientDisconnect = 2  // Client disconnected.
};
typedef UInt32 QTSS_CliSesClosingReason;

// CLIENT SESSION TEARDOWN REASON
//
//  An attribute in the QTSS_ClientSessionObject 
//
//  When calling QTSS_Teardown, a module should specify the QTSS_CliSesTeardownReason in the QTSS_ClientSessionObject 
//  if the tear down was not a client request.
//  
enum {
  qtssCliSesTearDownClientRequest = 0,
  qtssCliSesTearDownUnsupportedMedia = 1,
  qtssCliSesTearDownServerShutdown = 2,
  qtssCliSesTearDownServerInternalErr = 3,
  qtssCliSesTearDownBroadcastEnded =
  4 // A broadcast the client was watching ended

};
typedef UInt32 QTSS_CliSesTeardownReason;

// Events
enum {
  QTSS_ReadableEvent = 1,
  QTSS_WriteableEvent = 2
};
typedef UInt32 QTSS_EventType;

// Authentication schemes
enum {
  qtssAuthNone = 0,
  qtssAuthBasic = 1,
  qtssAuthDigest = 2
};
typedef UInt32 QTSS_AuthScheme;


/**********************************/
// RTSP SESSION TYPES
//
// Is this a normal RTSP session or an RTSP / HTTP session?
enum {
  qtssRTSPSession = 0,
  qtssRTSPHTTPSession = 1,
  qtssRTSPHTTPInputSession =
  2 //The input half of an RTSPHTTP session. These session types are usually very short lived.
};
typedef UInt32 QTSS_RTSPSessionType;

/**********************************/
//
// What type of RTP transport is being used for the RTP stream?
enum {
  qtssRTPTransportTypeUDP = 0,
  qtssRTPTransportTypeReliableUDP = 1,
  qtssRTPTransportTypeTCP = 2,
  qtssRTPTransportTypeUnknown = 3
};
typedef UInt32 QTSS_RTPTransportType;

/**********************************/
//
// What type of RTP network mode is being used for the RTP stream?
// unicast | multicast (mutually exclusive)
enum {
  qtssRTPNetworkModeDefault = 0, // not declared
  qtssRTPNetworkModeMulticast = 1,
  qtssRTPNetworkModeUnicast = 2
};
typedef UInt32 QTSS_RTPNetworkMode;

/**********************************/
//
// The transport mode in a SETUP request
enum {
  qtssRTPTransportModePlay = 0,
  qtssRTPTransportModeRecord = 1
};
typedef UInt32 QTSS_RTPTransportMode;

/**********************************/
// PAYLOAD TYPES
//
// When a module adds an RTP stream to a client session, it must specify
// the stream's payload type. This is so that other modules can find out
// this information in a generalized fashion. Here are the currently
// defined payload types
enum {
  qtssUnknownPayloadType = 0,
  qtssVideoPayloadType = 1,
  qtssAudioPayloadType = 2
};
typedef UInt32 QTSS_RTPPayloadType;

/**********************************/
// QTSS API OBJECT TYPES
enum {
  qtssDynamicObjectType = FOUR_CHARS_TO_INT('d', 'y', 'm', 'c'), //dymc
  qtssRTPStreamObjectType = FOUR_CHARS_TO_INT('r', 's', 't', 'o'), //rsto
  qtssClientSessionObjectType = FOUR_CHARS_TO_INT('c', 's', 'e', 'o'), //cseo
  qtssRTSPSessionObjectType = FOUR_CHARS_TO_INT('s', 's', 'e', 'o'), //sseo
  qtssRTSPRequestObjectType = FOUR_CHARS_TO_INT('s', 'r', 'q', 'o'), //srqo
  qtssRTSPHeaderObjectType = FOUR_CHARS_TO_INT('s', 'h', 'd', 'o'), //shdo
  qtssServerObjectType = FOUR_CHARS_TO_INT('s', 'e', 'r', 'o'), //sero
  qtssPrefsObjectType = FOUR_CHARS_TO_INT('p', 'r', 'f', 'o'), //prfo
  qtssTextMessagesObjectType = FOUR_CHARS_TO_INT('t', 'x', 't', 'o'), //txto
  qtssFileObjectType = FOUR_CHARS_TO_INT('f', 'i', 'l', 'e'), //file
  qtssModuleObjectType = FOUR_CHARS_TO_INT('m', 'o', 'd', 'o'), //modo
  qtssModulePrefsObjectType = FOUR_CHARS_TO_INT('m', 'o', 'd', 'p'), //modp
  qtssAttrInfoObjectType = FOUR_CHARS_TO_INT('a', 't', 't', 'r'), //attr
  qtssUserProfileObjectType = FOUR_CHARS_TO_INT('u', 's', 'p', 'o'), //uspo
  qtssConnectedUserObjectType = FOUR_CHARS_TO_INT('c', 'u', 's', 'r'), //cusr
  easyHTTPSessionObjectType = FOUR_CHARS_TO_INT('e', 'h', 's', 'o')  //ehso

};
typedef UInt32 QTSS_ObjectType;

/**********************************/
// ERROR LOG VERBOSITIES
//
// This provides some information to the module on the priority or
// type of this error message.
//
// When modules write to the error log stream (see below),
// the verbosity is qtssMessageVerbosity.
enum {
  qtssFatalVerbosity = 0,
  qtssWarningVerbosity = 1,
  qtssMessageVerbosity = 2,
  qtssAssertVerbosity = 3,
  qtssDebugVerbosity = 4,

  qtssIllegalVerbosity = 5
};
typedef UInt32 QTSS_ErrorVerbosity;

enum {
  qtssOpenFileNoFlags = 0,
  qtssOpenFileAsync =
  1,  // File stream will be asynchronous (read may return QTSS_WouldBlock)
  qtssOpenFileReadAhead =
  2   // File stream will be used for a linear read through the file.
};
typedef UInt32 QTSS_OpenFileFlags;


/**********************************/
// SERVER STATES
//
//  An attribute in the QTSS_ServerObject returns the server state
//  as a QTSS_ServerState. Modules may also set the server state.
//
//  Setting the server state to qtssFatalErrorState, or qtssShuttingDownState
//  will cause the server to quit.
//
//  Setting the state to qtssRefusingConnectionsState will cause the server
//  to start refusing new connections.
enum {
  qtssStartingUpState = 0,
  qtssRunningState = 1,
  qtssRefusingConnectionsState = 2,
  qtssFatalErrorState = 3,//a fatal error has occurred, not shutting down yet
  qtssShuttingDownState = 4,
  qtssIdleState =
  5 // Like refusing connections state, but will also kill any currently connected clients
};
typedef UInt32 QTSS_ServerState;

/**********************************/
// ILLEGAL ATTRIBUTE ID
enum {
  qtssIllegalAttrID = -1,
  qtssIllegalServiceID = -1
};

//*********************************/
// QTSS DON'T CALL SENDPACKETS AGAIN
// If this time is specified as the next packet time when returning
// from QTSS_SendPackets_Role, the module won't get called again in
// that role until another QTSS_Play is issued
enum {
  qtssDontCallSendPacketsAgain = -1
};

// DATA TYPES
enum {
  qtssAttrDataTypeUnknown = 0,
  qtssAttrDataTypeCharArray = 1,
  qtssAttrDataTypeBool16 = 2,
  qtssAttrDataTypeSInt16 = 3,
  qtssAttrDataTypeUInt16 = 4,
  qtssAttrDataTypeSInt32 = 5,
  qtssAttrDataTypeUInt32 = 6,
  qtssAttrDataTypeSInt64 = 7,
  qtssAttrDataTypeUInt64 = 8,
  qtssAttrDataTypeQTSS_Object = 9,
  qtssAttrDataTypeQTSS_StreamRef = 10,
  qtssAttrDataTypeFloat32 = 11,
  qtssAttrDataTypeFloat64 = 12,
  qtssAttrDataTypeVoidPointer = 13,
  qtssAttrDataTypeTimeVal = 14,

  qtssAttrDataTypeNumTypes = 15
};
typedef UInt32 QTSS_AttrDataType;

enum {
  qtssAttrModeRead = 1,
  qtssAttrModeWrite = 2,
  qtssAttrModePreempSafe = 4,
  qtssAttrModeInstanceAttrAllowed = 8,
  qtssAttrModeCacheable = 16,
  qtssAttrModeDelete = 32
};
typedef UInt32 QTSS_AttrPermission;

enum {
  qtssAttrRightNone = 0,
  qtssAttrRightRead = 1 << 0,
  qtssAttrRightWrite = 1 << 1,
  qtssAttrRightAdmin = 1 << 2,
  qtssAttrRightExtended = 1
      << 30, // Set this flag in the qtssUserRights when defining a new right. The right is a string i.e. "myauthmodule.myright" store the string in the QTSS_UserProfileObject attribute qtssUserExtendedRights
  qtssAttrRightQTSSExtended = 1
      << 31  // This flag is reserved for future use by the server. Additional rights are stored in qtssUserQTSSExtendedRights.
};
typedef UInt32 QTSS_AttrRights; // see QTSS_UserProfileObject


/**********************************/
//BUILT IN SERVER ATTRIBUTES

//The server maintains many attributes internally, and makes these available to plug-ins.
//Each value is a standard attribute, with a name and everything. Plug-ins may resolve the id's of
//these values by name if they'd like, but in the initialize role they will receive a struct of
//all the ids of all the internally maintained server parameters. This enumerated type block defines the indexes
//in that array for the id's.

enum {
  //QTSS_RTPStreamObject parameters. All of these are preemptive safe.

  qtssRTPStrTrackID =
  0,    //r/w       //UInt32            //Unique ID identifying each stream. This will default to 0 unless set explicitly by a module.
  qtssRTPStrSSRC =
  1,    //read      //UInt32            //SSRC (Synchronization Source) generated by the server. Guarenteed to be unique amongst all streams in the session.
  //This SSRC will be included in all RTCP Sender Reports generated by the server. See The RTP / RTCP RFC for more info on SSRCs.
      qtssRTPStrPayloadName =
      2,    //r/w       //char array        //Payload name of the media on this stream. This will be empty unless set explicitly by a module.
  qtssRTPStrPayloadType =
  3,    //r/w       //QTSS_RTPPayloadType   //Payload type of the media on this stream. This will default to qtssUnknownPayloadType unless set explicitly by a module.
  qtssRTPStrFirstSeqNumber =
  4,    //r/w       //SInt16            //Sequence number of the first RTP packet generated for this stream after the last PLAY request was issued. If known, this must be set by a module before calling QTSS_Play. It is used by the server to generate a proper RTSP PLAY response.
  qtssRTPStrFirstTimestamp =
  5,    //r/w       //SInt32            //RTP timestamp of the first RTP packet generated for this stream after the last PLAY request was issued. If known, this must be set by a module before calling QTSS_Play. It is used by the server to generate a proper RTSP PLAY response.
  qtssRTPStrTimescale =
  6,    //r/w       //SInt32            //Timescale for the track. If known, this must be set before calling QTSS_Play.
  qtssRTPStrQualityLevel = 7,    //r/w       //UInt32            //Private
  qtssRTPStrNumQualityLevels = 8,    //r/w       //UInt32            //Private
  qtssRTPStrBufferDelayInSecs =
  9,    //r/w       //Float32           //Size of the client's buffer. Server always sets this to 3 seconds, it is up to a module to determine what the buffer size is and set this attribute accordingly.

  // All of these parameters come out of the last RTCP packet received on this stream.
  // If the corresponding field in the last RTCP packet was blank, the attribute value will be 0.

  qtssRTPStrFractionLostPackets =
  10,   //read      //UInt32            // Fraction lost packets so far on this stream.
  qtssRTPStrTotalLostPackets =
  11,   //read      //UInt32            // Total lost packets so far on this stream.
  qtssRTPStrJitter =
  12,   //read      //UInt32            // Cumulative jitter on this stream.
  qtssRTPStrRecvBitRate =
  13,   //read      //UInt32            // Average bit rate received by the client in bits / sec.
  qtssRTPStrAvgLateMilliseconds =
  14,   //read      //UInt16            // Average msec packets received late.
  qtssRTPStrPercentPacketsLost =
  15,   //read      //UInt16            // Percent packets lost on this stream, as a fixed %.
  qtssRTPStrAvgBufDelayInMsec =
  16,   //read      //UInt16            // Average buffer delay in milliseconds
  qtssRTPStrGettingBetter =
  17,   //read      //UInt16            // Non-zero if the client is reporting that the stream is getting better.
  qtssRTPStrGettingWorse =
  18,   //read      //UInt16            // Non-zero if the client is reporting that the stream is getting worse.
  qtssRTPStrNumEyes =
  19,   //read      //UInt32            // Number of clients connected to this stream.
  qtssRTPStrNumEyesActive =
  20,   //read      //UInt32            // Number of clients playing this stream.
  qtssRTPStrNumEyesPaused =
  21,   //read      //UInt32            // Number of clients connected but currently paused.
  qtssRTPStrTotPacketsRecv =
  22,   //read      //UInt32            // Total packets received by the client
  qtssRTPStrTotPacketsDropped =
  23,   //read      //UInt16            // Total packets dropped by the client.
  qtssRTPStrTotPacketsLost =
  24,   //read      //UInt16            // Total packets lost.
  qtssRTPStrClientBufFill =
  25,   //read      //UInt16            // How much the client buffer is filled in 10ths of a second.
  qtssRTPStrFrameRate =
  26,   //read      //UInt16            // Current frame rate, in frames per second.
  qtssRTPStrExpFrameRate =
  27,   //read      //UInt16            // Expected frame rate, in frames per second.
  qtssRTPStrAudioDryCount =
  28,   //read      //UInt16            // Number of times the audio has run dry.
  // Address & network related parameters
      qtssRTPStrIsTCP =
      29,   //read      //bool            //Is this RTP stream being sent over TCP? If false, it is being sent over UDP.
  qtssRTPStrStreamRef =
  30,   //read      //QTSS_StreamRef    //A QTSS_StreamRef used for sending RTP or RTCP packets to the client. Use the QTSS_WriteFlags to specify whether each packet is an RTP or RTCP packet.
  qtssRTPStrTransportType =
  31,   //read      //QTSS_RTPTransportType // What kind of transport is being used?
  qtssRTPStrStalePacketsDropped =
  32,   //read      //UInt32            // Number of packets dropped by QTSS_Write because they were too old.
  qtssRTPStrCurrentAckTimeout =
  33,   //read      //UInt32            // Current ack timeout being advertised to the client in msec (part of reliable udp).

  qtssRTPStrCurPacketsLostInRTCPInterval =
  34,   // read     //UInt32            // An RTCP delta count of lost packets equal to qtssRTPStrPercentPacketsLost
  qtssRTPStrPacketCountInRTCPInterval =
  35,           // read     //UInt32            // An RTCP delta count of packets
  qtssRTPStrSvrRTPPort =
  36,   //read      //UInt16            // Port the server is sending RTP packets from for this stream
  qtssRTPStrClientRTPPort =
  37,   //read      //UInt16            // Port the server is sending RTP packets to for this stream
  qtssRTPStrNetworkMode =
  38,   //read      //QTSS_RTPNetworkMode // unicast or multicast

  qtssRTPStrThinningDisabled =
  39,   //read      //bool            //Stream thinning is disabled on this stream.
  qtssRTPStrNumParams = 40

};
typedef UInt32 QTSS_RTPStreamAttributes;

enum {
  //QTSS_ClientSessionObject parameters. All of these are preemptive safe

  qtssCliSesStreamObjects =
  0,    //read      //QTSS_RTPStreamObject//Iterated attribute. All the QTSS_RTPStreamRefs belonging to this session.
  qtssCliSesCreateTimeInMsec =
  1,    //read      //QTSS_TimeVal  //Time in milliseconds the session was created.
  qtssCliSesFirstPlayTimeInMsec =
  2,    //read      //QTSS_TimeVal  //Time in milliseconds the first QTSS_Play call was issued.
  qtssCliSesPlayTimeInMsec =
  3,    //read      //QTSS_TimeVal  //Time in milliseconds the most recent play was issued.
  qtssCliSesAdjustedPlayTimeInMsec =
  4,    //read      //QTSS_TimeVal  //Private - do not use
  qtssCliSesRTPBytesSent =
  5,    //read      //UInt32        //Number of RTP bytes sent so far on this session.
  qtssCliSesRTPPacketsSent =
  6,    //read      //UInt32        //Number of RTP packets sent so far on this session.
  qtssCliSesState =
  7,    //read      //QTSS_RTPSessionState // State of this session: is it paused or playing currently?
  qtssCliSesPresentationURL =
  8,    //read      //char array    //Presentation URL for this session. This URL is the "base" URL for the session. RTSP requests to this URL are assumed to affect all streams on the session.
  qtssCliSesFirstUserAgent = 9,    //read      //char array    //Private
  qtssCliSesMovieDurationInSecs =
  10,   //r/w       //Float64       //Duration of the movie for this session in seconds. This will default to 0 unless set by a module.
  qtssCliSesMovieSizeInBytes =
  11,   //r/w       //UInt64        //Movie size in bytes. This will default to 0 unless explictly set by a module
  qtssCliSesMovieAverageBitRate =
  12,   //r/w       //UInt32        //average bits per second based on total RTP bits/movie duration. This will default to 0 unless explictly set by a module.
  qtssCliSesLastRTSPSession =
  13,   //read      //QTSS_RTSPSessionObject //Private
  qtssCliSesFullURL =
  14,   //read      //char array    //full Presentation URL for this session. Same as qtssCliSesPresentationURL, but includes rtsp://domain.com prefix
  qtssCliSesHostName =
  15,   //read      //char array    //requestes host name for s session. Just the "domain.com" portion from qtssCliSesFullURL above

  qtssCliRTSPSessRemoteAddrStr =
  16,   //read      //char array        //IP address addr of client, in dotted-decimal format.
  qtssCliRTSPSessLocalDNS =
  17,   //read      //char array        //DNS name of local IP address for this RTSP connection.
  qtssCliRTSPSessLocalAddrStr =
  18,   //read      //char array        //Ditto, in dotted-decimal format.

  qtssCliRTSPSesUserName =
  19,   //read      //char array        // from the most recent (last) request.
  qtssCliRTSPSesUserPassword =
  20,   //read      //char array        // from the most recent (last) request.
  qtssCliRTSPSesURLRealm =
  21,   //read      //char array        // from the most recent (last) request.

  qtssCliRTSPReqRealStatusCode =
  22,   //read      //UInt32            //Same as qtssRTSPReqRTSPReqRealStatusCode, the status from the most recent (last) request.
  qtssCliTeardownReason =
  23,   //r/w       //QTSS_CliSesTeardownReason // Must be set by a module that calls QTSS_Teardown if it is not a client requested disconnect.

  qtssCliSesReqQueryString =
  24,   //read      //char array    //Query string from the request that creates this  client session

  qtssCliRTSPReqRespMsg =
  25,   //read      //char array    // from the most recent (last) request. Error message sent back to client if response was an error.

  qtssCliSesCurrentBitRate =
  26,   //read      //UInt32    //Current bit rate of all the streams on this session. This is not an average. In bits per second.
  qtssCliSesPacketLossPercent =
  27,   //read      //Float32   //Current percent loss as a fraction. .5 = 50%. This is not an average.
  qtssCliSesTimeConnectedInMsec =
  28,   //read      //SInt64    //Time in milliseconds that this client has been connected.
  qtssCliSesCounterID =
  29,   //read      //UInt32    //A unique, non-repeating ID for this session.
  qtssCliSesRTSPSessionID =
  30,   //read      //char array//The RTSP session ID that refers to this client session
  qtssCliSesFramesSkipped =
  31,   //r/w       //UInt32    //Modules can set this to be the number of frames skipped for this client
  qtssCliSesTimeoutMsec =
  32,    //r/w        //UInt32    // client session timeout in milliseconds refreshed by RefreshTimeout API call or any rtcp or rtp packet on the session.
  qtssCliSesOverBufferEnabled =
  33,   //read      //bool    // client overbuffers using dynamic rate streams
  qtssCliSesRTCPPacketsRecv =
  34,   //read      //UInt32    //Number of RTCP packets received so far on this session.
  qtssCliSesRTCPBytesRecv =
  35,   //read      //UInt32    //Number of RTCP bytes received so far on this session.
  qtssCliSesStartedThinning =
  36,   //read      //bool    // At least one of the streams in the session is thinned

  qtssCliSessLastRTSPBandwidth =
  37,   //read      //UInt32    // The last RTSP Bandwidth header value received from the client.
  qtssCliSesNumParams = 38

};
typedef UInt32 QTSS_ClientSessionAttributes;

enum {
  //QTSS_RTSPSessionObject parameters

  //Valid in any role that receives a QTSS_RTSPSessionObject
      qtssRTSPSesID =
      0,        //read      //UInt32        //This is a unique ID for each session since the server started up.
  qtssRTSPSesLocalAddr =
  1,        //read      //UInt32        //Local IP address for this RTSP connection
  qtssRTSPSesLocalAddrStr =
  2,        //read      //char array            //Ditto, in dotted-decimal format.
  qtssRTSPSesLocalDNS =
  3,        //read      //char array            //DNS name of local IP address for this RTSP connection.
  qtssRTSPSesRemoteAddr =
  4,        //read      //UInt32        //IP address of client.
  qtssRTSPSesRemoteAddrStr =
  5,        //read      //char array            //IP address addr of client, in dotted-decimal format.
  qtssRTSPSesEventCntxt =
  6,        //read      //QTSS_EventContextRef //An event context for the RTSP connection to the client. This should primarily be used to wait for EV_WR events if flow-controlled when responding to a client.
  qtssRTSPSesType =
  7,        //read      //QTSS_RTSPSession //Is this a normal RTSP session, or is it a HTTP tunnelled RTSP session?
  qtssRTSPSesStreamRef =
  8,        //read      //QTSS_RTSPSessionStream    // A QTSS_StreamRef used for sending data to the RTSP client.

  qtssRTSPSesLastUserName = 9,//read      //char array        // Private
  qtssRTSPSesLastUserPassword = 10,//read     //char array        // Private
  qtssRTSPSesLastURLRealm = 11,//read     //char array        // Private

  qtssRTSPSesLocalPort =
  12,       //read      //UInt16        // This is the local port for the connection
  qtssRTSPSesRemotePort =
  13,       //read      //UInt16        // This is the client port for the connection

  qtssRTSPSesLastDigestChallenge = 14,//read      //char array        // Private
  qtssRTSPSesNumParams = 15
};
typedef UInt32 QTSS_RTSPSessionAttributes;

enum {
  //Easy_HTTPSessionObject parameters
      easyHTTPSesID =
      0,        //read      //UInt32        //This is a unique ID for each session since the server started up.
  easyHTTPSesLocalAddr =
  1,        //read      //UInt32        //Local IP address for this HTTP connection
  easyHTTPSesLocalAddrStr =
  2,        //read      //char array	//Ditto, in dotted-decimal format.
  easyHTTPSesLocalDNS =
  3,        //read      //char array	//DNS name of local IP address for this RTSP connection.
  easyHTTPSesRemoteAddr =
  4,        //read      //UInt32        //IP address of client.
  easyHTTPSesRemoteAddrStr =
  5,        //read      //char array	//IP address addr of client, in dotted-decimal format.
  easyHTTPSesEventCntxt =
  6,        //read      //QTSS_EventContextRef //An event context for the HTTP connection to the client. This should primarily be used to wait for EV_WR events if flow-controlled when responding to a client.
  easyHTTPSesLastUserName = 7,        //read      //char array	// Private
  easyHTTPSesLastUserPassword = 8,        //read     //char array		// Private
  easyHTTPSesLastURLRealm = 9,        //read     //char array		// Private

  easyHTTPSesLocalPort =
  10,       //read      //UInt16        // This is the local port for the connection
  easyHTTPSesRemotePort =
  11,       //read      //UInt16        // This is the client port for the connection

  easyHTTPSesLastToken = 12,        //read      //char array	// Private

  easyHTTPSesContentBody = 13,        //read		//char array
  easyHTTPSesContentBodyOffset = 14,        //read		//UInt32

  easyHTTPSesNumParams = 15
};
typedef UInt32 Easy_HTTPSessionAttributes;

enum {
  //All text names are identical to the enumerated type names

  //QTSS_RTSPRequestObject parameters. All of these are pre-emptive safe parameters

  //Available in every role that receives the QTSS_RTSPRequestObject

  qtssRTSPReqFullRequest =
  0,    //read      //char array        //The full request sent by the client

  //Available in every method that receives the QTSS_RTSPRequestObject except for the QTSS_FilterMethod

  qtssRTSPReqMethodStr =
  1,    //read      //char array        //RTSP Method of this request.
  qtssRTSPReqFilePath =
  2,    //r/w        //char array        //Not pre-emptive safe!! //URI for this request, converted to a local file system path.
  qtssRTSPReqURI = 3,    //read      //char array        //URI for this request
  qtssRTSPReqFilePathTrunc =
  4,    //read      //char array        //Not pre-emptive safe!! //Same as qtssRTSPReqFilePath, without the last element of the path
  qtssRTSPReqFileName =
  5,    //read      //char array        //Not pre-emptive safe!! //Everything after the last path separator in the file system path
  qtssRTSPReqFileDigit =
  6,    //read      //char array        //Not pre-emptive safe!! //If the URI ends with one or more digits, this points to those.
  qtssRTSPReqAbsoluteURL =
  7,    //read      //char array        //The full URL, starting from "rtsp://"
  qtssRTSPReqTruncAbsoluteURL =
  8,    //read      //char array        //Absolute URL without last element of path
  qtssRTSPReqMethod =
  9,    //read      //QTSS_RTSPMethod   //Method as QTSS_RTSPMethod
  qtssRTSPReqStatusCode =
  10,   //r/w       //QTSS_RTSPStatusCode   //The current status code for the request as QTSS_RTSPStatusCode. By default, it is always qtssSuccessOK. If a module sets this attribute, and calls QTSS_SendRTSPHeaders, the status code of the header generated by the server will reflect this value.
  qtssRTSPReqStartTime =
  11,   //read      //Float64   //Start time specified in Range: header of PLAY request.
  qtssRTSPReqStopTime =
  12,   //read      //Float64   //Stop time specified in Range: header of PLAY request.
  qtssRTSPReqRespKeepAlive =
  13,   //r/w       //bool        //Will (should) the server keep the connection alive. Set this to false if the connection should be terminated after completion of this request.
  qtssRTSPReqRootDir =
  14,   //r/w       //char array    //Not pre-emptive safe!! //Root directory to use for this request. The default value for this parameter is the server's media folder path. Modules may set this attribute from the QTSS_RTSPRoute_Role.
  qtssRTSPReqRealStatusCode =
  15,   //read      //UInt32    //Same as qtssRTSPReqStatusCode, but translated from QTSS_RTSPStatusCode into an actual RTSP status code.
  qtssRTSPReqStreamRef =
  16,   //read      //QTSS_RTSPRequestStream //A QTSS_StreamRef for sending data to the RTSP client. This stream ref, unlike the one provided as an attribute in the QTSS_RTSPSessionObject, will never return EWOULDBLOCK in response to a QTSS_Write or a QTSS_WriteV call.

  qtssRTSPReqUserName =
  17,   //read      //char array//decoded Authentication information when provided by the RTSP request. See RTSPSessLastUserName.
  qtssRTSPReqUserPassword =
  18,   //read      //char array //decoded Authentication information when provided by the RTSP request. See RTSPSessLastUserPassword.
  qtssRTSPReqUserAllowed =
  19,   //r/w       //bool    //Default is server pref based, set to false if request is denied. Missing or bad movie files should allow the server to handle the situation and return true.
  qtssRTSPReqURLRealm =
  20,   //r/w       //char array //The authorization entity for the client to display "Please enter password for -realm- at server name. The default realm is "Streaming Server".
  qtssRTSPReqLocalPath =
  21,   //read      //char array //Not pre-emptive safe!! //The full local path to the file. This Attribute is first set after the Routing Role has run and before any other role is called.

  qtssRTSPReqIfModSinceDate =
  22,   //read      //QTSS_TimeVal  // If the RTSP request contains an If-Modified-Since header, this is the if-modified date, converted to a QTSS_TimeVal


  qtssRTSPReqQueryString =
  23,   //read      //char array  // query stting (CGI parameters) passed to the server in the request URL, does not include the '?' separator

  qtssRTSPReqRespMsg =
  24,   //r/w       //char array        // A module sending an RTSP error to the client should set this to be a text message describing why the error occurred. This description is useful to add to log files. Once the RTSP response has been sent, this attribute contains the response message.
  qtssRTSPReqContentLen =
  25,   //read      //UInt32            // Content length of incoming RTSP request body
  qtssRTSPReqSpeed =
  26,   //read      //Float32           // Value of Speed header, converted to a Float32.
  qtssRTSPReqLateTolerance =
  27,   //read      //Float32           // Value of the late-tolerance field of the x-RTP-Options header, or -1 if not present.

  qtssRTSPReqTransportType =
  28,   //read      //QTSS_RTPTransportType // What kind of transport?
  qtssRTSPReqTransportMode =
  29,   //read      //QTSS_RTPTransportMode // A setup request from the client. * maybe should just be an enum or the text of the mode value?
  qtssRTSPReqSetUpServerPort =
  30,   //r/w       //UInt16            // the ServerPort to respond to a client SETUP request with.

  qtssRTSPReqAction =
  31,   //r/w       //QTSS_ActionFlags  //Set by a module in the QTSS_RTSPSetAction_Role - for now, the server will set it as the role hasn't been added yet
  qtssRTSPReqUserProfile =
  32,   //r/w       //QTSS_UserProfileObject    //Object's username is filled in by the server and its password and group memberships filled in by the authentication module.
  qtssRTSPReqPrebufferMaxTime =
  33,   //read      //Float32           //The maxtime field of the x-Prebuffer RTSP header
  qtssRTSPReqAuthScheme = 34,   //read      //QTSS_AuthScheme

  qtssRTSPReqSkipAuthorization =
  35,   //r/w       //bool            // Set by a module that wants the particular request to be
  // allowed by all authorization modules
      qtssRTSPReqNetworkMode =
      36,   //read      //QTSS_RTPNetworkMode // unicast or multicast
  qtssRTSPReqDynamicRateState =
  37,   //read      //SInt32            // -1 not in request, 0 off, 1 on

  qtssRTSPReqBandwidthBits =
  38,   //read      //UInt32            //  Value of the Bandwdith header. Default is 0.
  qtssRTSPReqUserFound =
  39,   //r/w       //bool    //Default is false, set to true if the user is found in the authenticate role and the module wants to take ownership of authenticating the user.
  qtssRTSPReqAuthHandled =
  40,   //r/w       //bool    //Default is false, set to true in the authorize role to take ownerhsip of authorizing the request.
  qtssRTSPReqDigestChallenge =
  41,   //read      //char array //Challenge used by the server for Digest authentication
  qtssRTSPReqDigestResponse =
  42,   //read      //char array //Digest response used by the server for Digest authentication
  qtssRTSPReqNumParams = 43

};
typedef UInt32 QTSS_RTSPRequestAttributes;

enum {
  //QTSS_ServerObject parameters

  // These parameters ARE pre-emptive safe.

  qtssServerAPIVersion =
  0,    //read		//UInt32            //The API version supported by this server (format 0xMMMMmmmm, where M=major version, m=minor version)
  qtssSvrDefaultDNSName =
  1,    //read		//char array        //The "default" DNS name of the server
  qtssSvrDefaultIPAddr =
  2,    //read		//UInt32            //The "default" IP address of the server
  qtssSvrServerName = 3,    //read		//char array        //Name of the server
  qtssSvrServerVersion =
  4,    //read		//char array        //Version of the server
  qtssSvrServerBuildDate =
  5,    //read		//char array        //When was the server built?
  qtssSvrRTSPPorts =
  6,    //read		// NOT PREEMPTIVE SAFE!//UInt16         //Indexed parameter: all the ports the server is listening on
  qtssSvrRTSPServerHeader =
  7,    //read		//char array        //Server: header that the server uses to respond to RTSP clients

  // These parameters are NOT pre-emptive safe, they cannot be accessed
  // via. QTSS_GetValuePtr. Some exceptions noted below

  qtssSvrState =
  8,    //r/w		//QTSS_ServerState  //The current state of the server. If a module sets the server state, the server will respond in the appropriate fashion. Setting to qtssRefusingConnectionsState causes the server to refuse connections, setting to qtssFatalErrorState or qtssShuttingDownState causes the server to quit.
  qtssSvrIsOutOfDescriptors =
  9,    //read		//bool            //true if the server has run out of file descriptors, false otherwise
  qtssRTSPCurrentSessionCount =
  10,   //read		//UInt32            //Current number of connected clients over standard RTSP
  qtssRTSPHTTPCurrentSessionCount =
  11,   //read		//UInt32            //Current number of connected clients over RTSP / HTTP

  qtssRTPSvrNumUDPSockets =
  12,   //read      //UInt32    //Number of UDP sockets currently being used by the server
  qtssRTPSvrCurConn =
  13,   //read      //UInt32    //Number of clients currently connected to the server
  qtssRTPSvrTotalConn =
  14,   //read      //UInt32    //Total number of clients since startup
  qtssRTPSvrCurBandwidth =
  15,   //read      //UInt32    //Current bandwidth being output by the server in bits per second
  qtssRTPSvrTotalBytes =
  16,   //read      //UInt64    //Total number of bytes served since startup
  qtssRTPSvrAvgBandwidth =
  17,   //read      //UInt32    //Average bandwidth being output by the server in bits per second
  qtssRTPSvrCurPackets =
  18,   //read      //UInt32    //Current packets per second being output by the server
  qtssRTPSvrTotalPackets =
  19,   //read      //UInt64    //Total number of bytes served since startup

  qtssSvrHandledMethods =
  20,   //r/w       //QTSS_RTSPMethod   //The methods that the server supports. Modules should append the methods they support to this attribute in their QTSS_Initialize_Role.
  qtssSvrModuleObjects =
  21,   //read		//this IS PREMPTIVE SAFE!  //QTSS_ModuleObject // A module object representing each module
  qtssSvrStartupTime =
  22,   //read      //QTSS_TimeVal  //Time the server started up
  qtssSvrGMTOffsetInHrs =
  23,   //read      //SInt32        //Server time zone (offset from GMT in hours)
  qtssSvrDefaultIPAddrStr =
  24,   //read      //char array    //The "default" IP address of the server as a string

  qtssSvrPreferences =
  25,   //read      //QTSS_PrefsObject  // An object representing each the server's preferences
  qtssSvrMessages =
  26,   //read      //QTSS_Object   // An object containing the server's error messages.
  qtssSvrClientSessions =
  27,   //read      //QTSS_Object // An object containing all client sessions stored as indexed QTSS_ClientSessionObject(s).
  qtssSvrCurrentTimeMilliseconds =
  28,   //read      //QTSS_TimeVal  //Server's current time in milliseconds. Retrieving this attribute is equivalent to calling QTSS_Milliseconds
  qtssSvrCPULoadPercent =
  29,   //read      //Float32       //Current % CPU being used by the server

  qtssSvrNumReliableUDPBuffers =
  30,   //read      //UInt32    //Number of buffers currently allocated for UDP retransmits
  qtssSvrReliableUDPWastageInBytes =
  31,   //read      //UInt32    //Amount of data in the reliable UDP buffers being wasted
  qtssSvrConnectedUsers =
  32,   //r/w       //QTSS_Object   //List of connected user sessions (updated by modules for their sessions)

  qtssSvrServerBuild = 33,   //read      //char array //build of the server
  qtssSvrServerPlatform =
  34,   //read      //char array //Platform (OS) of the server
  qtssSvrRTSPServerComment =
  35,   //read      //char array //RTSP comment for the server header
  qtssSvrNumThinned = 36,   //read      //SInt32    //Number of thinned sessions
  qtssSvrNumThreads =
  37,   //read		//UInt32    //Number of task threads // see also qtssPrefsRunNumThreads
  qtssSvrNumParams = 38
};
typedef UInt32 QTSS_ServerAttributes;

enum {
  //QTSS_PrefsObject parameters

  // Valid in all methods. None of these are pre-emptive safe, so the version
  // of QTSS_GetAttribute that copies data must be used.

  // All of these parameters are read-write.

  qtssPrefsRTSPTimeout =
  0,    //"rtsp_timeout"                //UInt32    //RTSP timeout in seconds sent to the client.
  qtssPrefsRTSPSessionTimeout =
  1,    //"rtsp_session_timeout"        //UInt32    //Amount of time in seconds the server will wait before disconnecting idle RTSP clients. 0 means no timeout
  qtssPrefsRTPSessionTimeout =
  2,    //"rtp_session_timeout"         //UInt32    //Amount of time in seconds the server will wait before disconnecting idle RTP clients. 0 means no timeout
  qtssPrefsMaximumConnections =
  3,    //"maximum_connections"         //SInt32    //Maximum # of concurrent RTP connections allowed by the server. -1 means unlimited.
  qtssPrefsMaximumBandwidth =
  4,    //"maximum_bandwidth"           //SInt32    //Maximum amt of bandwidth the server is allowed to serve in K bits. -1 means unlimited.
  qtssPrefsMovieFolder =
  5,    //"movie_folder"           //char array    //Path to the root movie folder
  qtssPrefsRTSPIPAddr =
  6,    //"bind_ip_addr"                //char array    //IP address the server should accept RTSP connections on. 0.0.0.0 means all addresses on the machine.
  qtssPrefsBreakOnAssert =
  7,    //"break_on_assert"             //bool        //If true, the server will break in the debugger when an assert fails.
  qtssPrefsAutoRestart =
  8,    //"auto_restart"                //bool        //If true, the server will automatically restart itself if it crashes.
  qtssPrefsTotalBytesUpdate =
  9,    //"total_bytes_update"          //UInt32    //Interval in seconds between updates of the server's total bytes and current bandwidth statistics
  qtssPrefsAvgBandwidthUpdate =
  10,   //"average_bandwidth_update"    //UInt32    //Interval in seconds between computations of the server's average bandwidth
  qtssPrefsSafePlayDuration =
  11,   //"safe_play_duration"          //UInt32    //Hard to explain... see streamingserver.conf
  qtssPrefsModuleFolder =
  12,   //"module_folder"               //char array    //Path to the module folder

  // There is a compiled-in error log module that loads before all the other modules
  // (so it can log errors from the get-go). It uses these prefs.

  qtssPrefsErrorLogName =
  13,   //"error_logfile_name"          //char array        //Name of error log file
  qtssPrefsErrorLogDir =
  14,   //"error_logfile_dir"           //char array        //Path to error log file directory
  qtssPrefsErrorRollInterval =
  15,   //"error_logfile_interval"      //UInt32    //Interval in days between error logfile rolls
  qtssPrefsMaxErrorLogSize =
  16,   //"error_logfile_size"          //UInt32    //Max size in bytes of the error log
  qtssPrefsErrorLogVerbosity =
  17,   //"error_logfile_verbosity"     //UInt32    //Max verbosity level of messages the error logger will log
  qtssPrefsScreenLogging =
  18,   //"screen_logging"              //bool        //Should the error logger echo messages to the screen?
  qtssPrefsErrorLogEnabled =
  19,   //"error_logging"               //bool        //Is error logging enabled?

  qtssPrefsDropVideoAllPacketsDelayInMsec =
  20,   // "drop_all_video_delay"//SInt32 // Don't send video packets later than this
  qtssPrefsStartThinningDelayInMsec =
  21,   // "start_thinning_delay"//SInt32 // lateness at which we might start thinning
  qtssPrefsLargeWindowSizeInK =
  22,   // "large_window_size"	// UInt32    //default size that will be used for high bitrate movies
  qtssPrefsWindowSizeThreshold =
  23,   // "window_size_threshold"  // UInt32    //bitrate at which we switch to larger window size

  qtssPrefsMinTCPBufferSizeInBytes =
  24,   // "min_tcp_buffer_size" //UInt32    // When streaming over TCP, this is the minimum size the TCP Socket send buffer can be set to
  qtssPrefsMaxTCPBufferSizeInBytes =
  25,   // "max_tcp_buffer_size" //UInt32    // When streaming over TCP, this is the maximum size the TCP Socket send buffer can be set to
  qtssPrefsTCPSecondsToBuffer =
  26,   // "tcp_seconds_to_buffer" //Float32 // When streaming over TCP, the size of the TCP send buffer is scaled based on the bitrate of the movie. It will fit all the data that gets sent in this amount of time.

  qtssPrefsDoReportHTTPConnectionAddress =
  27,   // "do_report_http_connection_ip_address"    //bool    // when behind a round robin DNS, the client needs to be told the specific ip address of the maching handling its request. this pref tells the server to repot its IP address in the reply to the HTTP GET request when tunneling RTSP through HTTP

  qtssPrefsDefaultAuthorizationRealm =
  28,   // "default_authorization_realm" //char array   //

  qtssPrefsRunUserName =
  29,   // "run_user_name"       //char array        //Run under this user's account
  qtssPrefsRunGroupName =
  30,   // "run_group_name"      //char array        //Run under this group's account

  qtssPrefsSrcAddrInTransport =
  31,   // "append_source_addr_in_transport" // bool   //If true, the server will append the src address to the Transport header responses
  qtssPrefsRTSPPorts = 32,   // "rtsp_ports"          // UInt16

  qtssPrefsMaxRetransDelayInMsec =
  33,   // "max_retransmit_delay" // UInt32  //maximum interval between when a retransmit is supposed to be sent and when it actually gets sent. Lower values means smoother flow but slower server performance
  qtssPrefsSmallWindowSizeInK =
  34,   // "small_window_size"  // UInt32    //default size that will be used for low bitrate movies
  qtssPrefsAckLoggingEnabled =
  35,   // "ack_logging_enabled"  // bool  //Debugging only: turns on detailed logging of UDP acks / retransmits
  qtssPrefsRTCPPollIntervalInMsec =
  36,   // "rtcp_poll_interval"      // UInt32   //interval (in Msec) between poll for RTCP packets
  qtssPrefsRTCPSockRcvBufSizeInK =
  37,   // "rtcp_rcv_buf_size"   // UInt32   //Size of the receive Socket buffer for udp sockets used to receive rtcp packets
  qtssPrefsSendInterval = 38,   // "send_interval"  // UInt32    //
  qtssPrefsThickAllTheWayDelayInMsec =
  39,   // "thick_all_the_way_delay"     // UInt32   //
  qtssPrefsAltTransportIPAddr =
  40,   // "alt_transport_src_ipaddr"// char     //If empty, the server uses its own IP addr in the source= param of the transport header. Otherwise, it uses this addr.
  qtssPrefsMaxAdvanceSendTimeInSec =
  41,   // "max_send_ahead_time"     // UInt32   //This is the farthest in advance the server will send a packet to a client that supports overbuffering.
  qtssPrefsReliableUDPSlowStart =
  42,   // "reliable_udp_slow_start" // bool   //Is reliable UDP slow start enabled?
  qtssPrefsEnableCloudPlatform = 43,   // "enable_cloud_platform"   // bool
  qtssPrefsAuthenticationScheme =
  44,   // "authentication_scheme" // char   //Set this to be the authentication scheme you want the server to use. "basic", "digest", and "none" are the currently supported values
  qtssPrefsDeleteSDPFilesInterval =
  45,   // "sdp_file_delete_interval_seconds" //UInt32 //Feature rem
  qtssPrefsAutoStart =
  46,   // "auto_start" //bool //If true, streaming server likes to be started at system startup
  qtssPrefsReliableUDP =
  47,   // "reliable_udp" //bool //If true, uses reliable udp transport if requested by the client
  qtssPrefsReliableUDPDirs = 48,   // "reliable_udp_dirs" //CharArray
  qtssPrefsReliableUDPPrintfs =
  49,   // "reliable_udp_printfs" //bool //If enabled, server prints out interesting statistics for the reliable UDP clients

  qtssPrefsDropAllPacketsDelayInMsec =
  50,   // "drop_all_packets_delay" // SInt32    // don't send any packets later than this
  qtssPrefsThinAllTheWayDelayInMsec =
  51,   // "thin_all_the_way_delay" // SInt32    // thin to key frames
  qtssPrefsAlwaysThinDelayInMsec =
  52,   // "always_thin_delay" // SInt32         // we always start to thin at this point
  qtssPrefsStartThickingDelayInMsec =
  53,   // "start_thicking_delay" // SInt32      // maybe start thicking at this point
  qtssPrefsQualityCheckIntervalInMsec =
  54,   // "quality_check_interval" // UInt32    // adjust thinnning params this often
  qtssPrefsEnableRTSPErrorMessage =
  55,   // "RTSP_error_message" //bool // Appends a content body string error message for reported RTSP errors.
  qtssPrefsEnableRTSPDebugPrintfs =
  56,   // "RTSP_debug_printfs" //Boo1l6 // printfs incoming RTSPRequests and Outgoing RTSP responses.

  qtssPrefsEnableMonitorStatsFile =
  57,   // "enable_monitor_stats_file" //bool //write server stats to the monitor file
  qtssPrefsMonitorStatsFileIntervalSec =
  58,   // "monitor_stats_file_interval_seconds" // private
  qtssPrefsMonitorStatsFileName = 59,   // "monitor_stats_file_name" // private

  qtssPrefsEnablePacketHeaderPrintfs =
  60,   // "enable_packet_header_printfs" //bool // RTP and RTCP printfs of outgoing packets.
  qtssPrefsPacketHeaderPrintfOptions =
  61,   // "packet_header_printf_options" //char //set of printfs to print. Form is [text option] [;]  default is "rtp;rr;sr;". This means rtp packets, rtcp sender reports, and rtcp receiver reports.
  qtssPrefsOverbufferRate = 62,   // "overbuffer_rate"    //Float32
  qtssPrefsMediumWindowSizeInK =
  63,   // "medium_window_size" // UInt32    //default size that will be used for medium bitrate movies
  qtssPrefsWindowSizeMaxThreshold =
  64,   // "window_size_threshold"  // UInt32    //bitrate at which we switch from medium to large window size
  qtssPrefsEnableRTSPServerInfo =
  65,   // "RTSP_server_info" //Boo1l6 // Adds server info to the RTSP responses.
  qtssPrefsRunNumThreads =
  66,   // "run_num_threads" //UInt32 // if value is non-zero, will  create that many task threads; otherwise a Thread will be created for each processor
  qtssPrefsPidFile = 67,   // "pid_file" //Char Array //path to pid file
  qtssPrefsCloseLogsOnWrite =
  68,   // "force_logs_close_on_write" //bool // force log files to close after each write.
  qtssPrefsDisableThinning =
  69,   // "disable_thinning" //bool // Usually used for performance testing. Turn off stream thinning from packet loss or stream lateness.
  qtssPrefsPlayersReqRTPHeader =
  70,   // "player_requires_rtp_header_info" //Char array //name of player to match against the player's user agent header
  qtssPrefsPlayersReqBandAdjust =
  71,   // "player_requires_bandwidth_adjustment //Char array //name of player to match against the player's user agent header
  qtssPrefsPlayersReqNoPauseTimeAdjust =
  72,   // "player_requires_no_pause_time_adjustment //Char array //name of player to match against the player's user agent header

  qtssPrefsDefaultStreamQuality =
  73,   // "default_stream_quality //UInt16 //0 is all day and best quality. Higher values are worse maximum depends on the media and the media module
  qtssPrefsPlayersReqRTPStartTimeAdjust =
  74,   // "player_requires_rtp_start_time_adjust" //Char Array //name of players to match against the player's user agent header

  qtssPrefsEnableUDPMonitor =
  75,   // "enable_udp_monitor_stream" //Boo1l6 // reflect all udp streams to the monitor ports, use an sdp to view
  qtssPrefsUDPMonitorAudioPort =
  76,   // "udp_monitor_video_port" //UInt16 // localhost destination port of reflected stream
  qtssPrefsUDPMonitorVideoPort =
  77,   // "udp_monitor_audio_port" //UInt16 // localhost destination port of reflected stream
  qtssPrefsUDPMonitorDestIPAddr =
  78,   // "udp_monitor_dest_ip"    //char array    //IP address the server should send RTP monitor reflected streams.
  qtssPrefsUDPMonitorSourceIPAddr =
  79,   // "udp_monitor_src_ip"    //char array    //client IP address the server monitor should reflect. *.*.*.* means all client addresses.
  qtssPrefsEnableAllowGuestDefault =
  80,   // "enable_allow_guest_authorize_default" //Boo1l6 // server hint to access modules to allow guest access as the default (can be overriden in a qtaccess file or other means)
  qtssPrefsNumRTSPThreads =
  81,   // "run_num_rtsp_threads" //UInt32 // if value is non-zero, the server will  create that many task threads; otherwise a single Thread will be created.

  easyPrefsHTTPServiceLanPort = 82,    // "service_lan_port"	//UInt16
  easyPrefsHTTPServiceWanPort = 83,    // "service_wan_port"	//UInt16

  easyPrefsServiceWANIPAddr = 84,    // "service_wan_ip"		//char array
  easyPrefsRTSPWANPort = 85,    // "rtsp_wan_port"		//UInt16

  easyPrefsNginxRTMPServer = 86,    // "nginx_rtmp_server"		//char array
  qtssPrefsNumParams = 87
};

typedef UInt32 QTSS_PrefsAttributes;

enum {
  //QTSS_TextMessagesObject parameters

  // All of these parameters are read-only, char*'s, and preemptive-safe.

  qtssMsgNoMessage = 0,    //"NoMessage"
  qtssMsgNoURLInRequest = 1,
  qtssMsgBadRTSPMethod = 2,
  qtssMsgNoRTSPVersion = 3,
  qtssMsgNoRTSPInURL = 4,
  qtssMsgURLTooLong = 5,
  qtssMsgURLInBadFormat = 6,
  qtssMsgNoColonAfterHeader = 7,
  qtssMsgNoEOLAfterHeader = 8,
  qtssMsgRequestTooLong = 9,
  qtssMsgNoModuleFolder = 10,
  qtssMsgCouldntListen = 11,
  qtssMsgInitFailed = 12,
  qtssMsgNotConfiguredForIP = 13,
  qtssMsgDefaultRTSPAddrUnavail = 14,
  qtssMsgBadModule = 15,
  qtssMsgRegFailed = 16,
  qtssMsgRefusingConnections = 17,
  qtssMsgTooManyClients = 18,
  qtssMsgTooMuchThruput = 19,
  qtssMsgNoSessionID = 20,
  qtssMsgFileNameTooLong = 21,
  qtssMsgNoClientPortInTransport = 22,
  qtssMsgRTPPortMustBeEven = 23,
  qtssMsgRTCPPortMustBeOneBigger = 24,
  qtssMsgOutOfPorts = 25,
  qtssMsgNoModuleForRequest = 26,
  qtssMsgAltDestNotAllowed = 27,
  qtssMsgCantSetupMulticast = 28,
  qtssListenPortInUse = 29,
  qtssListenPortAccessDenied = 30,
  qtssListenPortError = 31,
  qtssMsgBadBase64 = 32,
  qtssMsgSomePortsFailed = 33,
  qtssMsgNoPortsSucceeded = 34,
  qtssMsgCannotCreatePidFile = 35,
  qtssMsgCannotSetRunUser = 36,
  qtssMsgCannotSetRunGroup = 37,
  qtssMsgNoSesIDOnDescribe = 38,
  qtssServerPrefMissing = 39,
  qtssServerPrefWrongType = 40,
  qtssMsgCantWriteFile = 41,
  qtssMsgSockBufSizesTooLarge = 42,
  qtssMsgBadFormat = 43,
  qtssMsgNumParams = 44

};
typedef UInt32 QTSS_TextMessagesAttributes;

enum {
  //QTSS_FileObject parameters

  // All of these parameters are preemptive-safe.

  qtssFlObjStream =
  0,    // read // QTSS_FileStream. Stream ref for this file object
  qtssFlObjFileSysModuleName =
  1,    // read // char array. Name of the file system module handling this file object
  qtssFlObjLength = 2,    // r/w  // UInt64. Length of the file
  qtssFlObjPosition =
  3,    // read // UInt64. Current position of the file pointer in the file.
  qtssFlObjModDate =
  4,    // r/w  // QTSS_TimeVal. Date & time of last modification

  qtssFlObjNumParams = 5
};
typedef UInt32 QTSS_FileObjectAttributes;

enum {
  //QTSS_ModuleObject parameters

  qtssModName =
  0,    //read      //preemptive-safe       //char array        //Module name.
  qtssModDesc =
  1,    //r/w       //not preemptive-safe   //char array        //Text description of what the module does
  qtssModVersion =
  2,    //r/w       //not preemptive-safe   //UInt32            //Version of the module. UInt32 format should be 0xMM.m.v.bbbb M=major version m=minor version v=very minor version b=build #
  qtssModRoles =
  3,    //read      //preemptive-safe       //QTSS_Role         //List of all the roles this module has registered for.
  qtssModPrefs =
  4,    //read      //preemptive-safe       //QTSS_ModulePrefsObject //An object containing as attributes the preferences for this module
  qtssModAttributes = 5,    //read      //preemptive-safe       //QTSS_Object

  qtssModNumParams = 6
};
typedef UInt32 QTSS_ModuleObjectAttributes;

enum {
  //QTSS_AttrInfoObject parameters

  // All of these parameters are preemptive-safe.

  qtssAttrName = 0,    //read //char array             //Attribute name
  qtssAttrID = 1,    //read //QTSS_AttributeID       //Attribute ID
  qtssAttrDataType = 2,    //read //QTSS_AttrDataType      //Data type
  qtssAttrPermissions = 3,    //read //QTSS_AttrPermission    //Permissions

  qtssAttrInfoNumParams = 4
};
typedef UInt32 QTSS_AttrInfoObjectAttributes;

enum {
  //QTSS_UserProfileObject parameters

  // All of these parameters are preemptive-safe.

  qtssUserName = 0, //read  //char array
  qtssUserPassword = 1, //r/w   //char array
  qtssUserGroups =
  2, //r/w   //char array -  multi-valued attribute, all values should be C strings padded with \0s to                                         //              make them all of the same length
  qtssUserRealm =
  3, //r/w   //char array -  the authentication realm for username
  qtssUserRights = 4, //r/w   //QTSS_AttrRights - rights granted this user
  qtssUserExtendedRights =
  5, //r/w   //qtssAttrDataTypeCharArray - a list of strings with extended rights granted to the user.
  qtssUserQTSSExtendedRights =
  6, //r/w   //qtssAttrDataTypeCharArray - a private list of strings with extended rights granted to the user and reserved by QTSS/Apple.
  qtssUserNumParams = 7,
};
typedef UInt32 QTSS_UserProfileObjectAttributes;

enum {
  //QTSS_ConnectedUserObject parameters

  //All of these are preemptive safe

  qtssConnectionType =
  0,    //read      //char array    // type of user connection (e.g. "RTP reflected")
  qtssConnectionCreateTimeInMsec =
  1,    //read      //QTSS_TimeVal  //Time in milliseconds the session was created.
  qtssConnectionTimeConnectedInMsec =
  2,    //read      //QTSS_TimeVal  //Time in milliseconds the session was created.
  qtssConnectionBytesSent =
  3,    //read      //UInt32        //Number of RTP bytes sent so far on this session.
  qtssConnectionMountPoint =
  4,    //read      //char array    //Presentation URL for this session. This URL is the "base" URL for the session. RTSP requests to this URL are assumed to affect all streams on the session.
  qtssConnectionHostName =
  5,    //read      //char array    //host name for this request

  qtssConnectionSessRemoteAddrStr =
  6,    //read      //char array        //IP address addr of client, in dotted-decimal format.
  qtssConnectionSessLocalAddrStr =
  7,    //read      //char array        //Ditto, in dotted-decimal format.

  qtssConnectionCurrentBitRate =
  8,    //read          //UInt32    //Current bit rate of all the streams on this session. This is not an average. In bits per second.
  qtssConnectionPacketLossPercent =
  9,    //read          //Float32   //Current percent loss as a fraction. .5 = 50%. This is not an average.

  qtssConnectionTimeStorage =
  10,   //read          //QTSS_TimeVal  //Internal, use qtssConnectionTimeConnectedInMsec above

  qtssConnectionNumParams = 11
};
typedef UInt32 QTSS_ConnectedUserObjectAttributes;


/********************************************************************/
// QTSS API ROLES
//
// Each role represents a unique situation in which a module may be
// invoked. Modules must specify which roles they want to be invoked for. 

enum {
  //Global
      QTSS_Register_Role = FOUR_CHARS_TO_INT('r',
                                             'e',
                                             'g',
                                             ' '), //reg  //All modules get this once at startup
  QTSS_Initialize_Role = FOUR_CHARS_TO_INT('i',
                                           'n',
                                           'i',
                                           't'), //init //Gets called once, later on in the startup process
  QTSS_Shutdown_Role =
  FOUR_CHARS_TO_INT('s', 'h', 'u', 't'), //shut //Gets called once at shutdown

  QTSS_ErrorLog_Role = FOUR_CHARS_TO_INT('e',
                                         'l',
                                         'o',
                                         'g'), //elog //This gets called when the server wants to log an error.
  QTSS_RereadPrefs_Role = FOUR_CHARS_TO_INT('p',
                                            'r',
                                            'e',
                                            'f'), //pref //This gets called when the server rereads preferences.
  QTSS_StateChange_Role = FOUR_CHARS_TO_INT('s',
                                            't',
                                            'a',
                                            't'), //stat //This gets called whenever the server changes state.

  QTSS_Interval_Role = FOUR_CHARS_TO_INT('t',
                                         'i',
                                         'm',
                                         'r'), //timr //This gets called whenever the module's interval timer times out calls.

  //RTSP-specific
      QTSS_RTSPFilter_Role = FOUR_CHARS_TO_INT('f',
                                               'i',
                                               'l',
                                               't'), //filt //Filter all RTSP requests before the server parses them
  QTSS_RTSPRoute_Role = FOUR_CHARS_TO_INT('r',
                                          'o',
                                          'u',
                                          't'), //rout //Route all RTSP requests to the correct root folder.
  QTSS_RTSPAuthenticate_Role = FOUR_CHARS_TO_INT('a',
                                                 't',
                                                 'h',
                                                 'n'), //athn //Authenticate the RTSP request username.
  QTSS_RTSPAuthorize_Role = FOUR_CHARS_TO_INT('a',
                                              'u',
                                              't',
                                              'h'), //auth //Authorize RTSP requests to proceed
  QTSS_RTSPPreProcessor_Role = FOUR_CHARS_TO_INT('p',
                                                 'r',
                                                 'e',
                                                 'p'), //prep //Pre-process all RTSP requests before the server responds.
  //Modules may opt to "steal" the request and return a client response.
      QTSS_RTSPRequest_Role = FOUR_CHARS_TO_INT('r',
                                                'e',
                                                'q',
                                                'u'), //requ //Process an RTSP request & send client response
  QTSS_RTSPPostProcessor_Role =
  FOUR_CHARS_TO_INT('p', 'o', 's', 't'), //post //Post-process all RTSP requests
  QTSS_RTSPSessionClosing_Role =
  FOUR_CHARS_TO_INT('s', 'e', 's', 'c'), //sesc //RTSP session is going away

  QTSS_RTSPIncomingData_Role = FOUR_CHARS_TO_INT('i',
                                                 'c',
                                                 'm',
                                                 'd'), //icmd //Incoming interleaved RTP data on this RTSP connection

  //RTP-specific
      QTSS_RTPSendPackets_Role = FOUR_CHARS_TO_INT('s',
                                                   'e',
                                                   'n',
                                                   'd'), //send //Send RTP packets to the client
  QTSS_ClientSessionClosing_Role =
  FOUR_CHARS_TO_INT('d', 'e', 's', 's'), //dess //Client session is going away

  //RTCP-specific
      QTSS_RTCPProcess_Role = FOUR_CHARS_TO_INT('r',
                                                't',
                                                'c',
                                                'p'), //rtcp //Process all RTCP packets sent to the server

  //File system roles
      QTSS_OpenFilePreProcess_Role =
      FOUR_CHARS_TO_INT('o', 'p', 'p', 'r'),  //oppr
  QTSS_OpenFile_Role = FOUR_CHARS_TO_INT('o', 'p', 'f', 'l'),  //opfl
  QTSS_AdviseFile_Role = FOUR_CHARS_TO_INT('a', 'd', 'f', 'l'),  //adfl
  QTSS_ReadFile_Role = FOUR_CHARS_TO_INT('r', 'd', 'f', 'l'),  //rdfl
  QTSS_CloseFile_Role = FOUR_CHARS_TO_INT('c', 'l', 'f', 'l'),  //clfl
  QTSS_RequestEventFile_Role = FOUR_CHARS_TO_INT('r', 'e', 'f', 'l'),  //refl

  //EasyHLSModule
      Easy_HLSOpen_Role = FOUR_CHARS_TO_INT('h', 'l', 's', 'o'),  //hlso
  Easy_HLSClose_Role = FOUR_CHARS_TO_INT('h', 'l', 's', 'c'),  //hlsc

  //EasyCMSModule
      Easy_CMSFreeStream_Role = FOUR_CHARS_TO_INT('e', 'f', 's', 'r'),  //efsr

  //EasyRedisModule
      Easy_RedisSetRTSPLoad_Role =
      FOUR_CHARS_TO_INT('c', 'r', 'n', 'r'),    //crnr
  Easy_RedisUpdateStreamInfo_Role =
  FOUR_CHARS_TO_INT('a', 'p', 'n', 'r'),    //apnr
  Easy_RedisTTL_Role = FOUR_CHARS_TO_INT('t', 't', 'l', 'r'),    //ttlr
  Easy_RedisGetAssociatedCMS_Role =
  FOUR_CHARS_TO_INT('g', 'a', 'c', 'r'),    //gacr
  Easy_RedisJudgeStreamID_Role =
  FOUR_CHARS_TO_INT('j', 's', 'i', 'r'),    //jsir

  //RESTful
      Easy_GetDeviceStream_Role =
      FOUR_CHARS_TO_INT('g', 'd', 's', 'r'),    //gdsr
  Easy_LiveDeviceStream_Role = FOUR_CHARS_TO_INT('l', 'd', 's', 'r'),    //ldsr
};
typedef UInt32 QTSS_Role;


//***********************************************/
// TYPEDEFS

typedef void *QTSS_StreamRef;
typedef void *QTSS_Object;
typedef void *QTSS_ServiceFunctionArgsPtr;
typedef SInt32 QTSS_AttributeID;
typedef SInt32 QTSS_ServiceID;
typedef SInt64 QTSS_TimeVal;

typedef QTSS_Object QTSS_RTPStreamObject;
typedef QTSS_Object QTSS_RTSPSessionObject;
typedef QTSS_Object QTSS_RTSPRequestObject;
typedef QTSS_Object QTSS_RTSPHeaderObject;
typedef QTSS_Object QTSS_ClientSessionObject;
typedef QTSS_Object QTSS_ServerObject;
typedef QTSS_Object QTSS_PrefsObject;
typedef QTSS_Object QTSS_TextMessagesObject;
typedef QTSS_Object QTSS_FileObject;
typedef QTSS_Object QTSS_ModuleObject;
typedef QTSS_Object QTSS_ModulePrefsObject;
typedef QTSS_Object QTSS_AttrInfoObject;
typedef QTSS_Object QTSS_UserProfileObject;
typedef QTSS_Object QTSS_ConnectedUserObject;

typedef QTSS_StreamRef QTSS_ErrorLogStream;
typedef QTSS_StreamRef QTSS_FileStream;
typedef QTSS_StreamRef QTSS_RTSPSessionStream;
typedef QTSS_StreamRef QTSS_RTSPRequestStream;
typedef QTSS_StreamRef QTSS_RTPStreamStream;
typedef QTSS_StreamRef QTSS_SocketStream;


//***********************************************/
// ROLE PARAMETER BLOCKS
//
// Each role has a unique set of parameters that get passed
// to the module.

typedef struct {
  char outModuleName[QTSS_MAX_MODULE_NAME_LENGTH];
} QTSS_Register_Params;

typedef struct {
  QTSS_ServerObject inServer;           // Global dictionaries
  QTSS_PrefsObject inPrefs;
  QTSS_TextMessagesObject inMessages;
  QTSS_ErrorLogStream
      inErrorLogStream;   // Writing to this stream causes modules to
  // be invoked in the QTSS_ErrorLog_Role
  QTSS_ModuleObject inModule;
} QTSS_Initialize_Params;

typedef struct {
  QTSS_ErrorVerbosity inVerbosity;
  char *inBuffer;

} QTSS_ErrorLog_Params;

typedef struct {
  QTSS_ServerState inNewState;
} QTSS_StateChange_Params;

typedef struct {
  QTSS_RTSPSessionObject inRTSPSession;
  QTSS_RTSPRequestObject inRTSPRequest;
  QTSS_RTSPHeaderObject inRTSPHeaders;
  QTSS_ClientSessionObject inClientSession;

} QTSS_StandardRTSP_Params;

typedef struct {
  QTSS_RTSPSessionObject inRTSPSession;
  QTSS_RTSPRequestObject inRTSPRequest;
  char **outNewRequest;

} QTSS_Filter_Params;

typedef struct {
  QTSS_RTSPRequestObject inRTSPRequest;
} QTSS_RTSPAuth_Params;

typedef struct {
  QTSS_RTSPSessionObject inRTSPSession;
  QTSS_ClientSessionObject inClientSession;
  char *inPacketData;
  UInt32 inPacketLen;

} QTSS_IncomingData_Params;

typedef struct {
  QTSS_RTSPSessionObject inRTSPSession;
} QTSS_RTSPSession_Params;

typedef struct {
  QTSS_ClientSessionObject inClientSession;
  QTSS_TimeVal inCurrentTime;
  QTSS_TimeVal outNextPacketTime;
} QTSS_RTPSendPackets_Params;

typedef struct {
  QTSS_ClientSessionObject inClientSession;
  QTSS_CliSesClosingReason inReason;
} QTSS_ClientSessionClosing_Params;

typedef struct {
  QTSS_ClientSessionObject inClientSession;
  QTSS_RTPStreamObject inRTPStream;
  void *inRTCPPacketData;
  UInt32 inRTCPPacketDataLen;
} QTSS_RTCPProcess_Params;

typedef struct {
  char *inPath;
  QTSS_OpenFileFlags inFlags;
  QTSS_Object inFileObject;
} QTSS_OpenFile_Params;

typedef struct {
  QTSS_Object inFileObject;
  UInt64 inPosition;
  UInt32 inSize;
} QTSS_AdviseFile_Params;

typedef struct {
  QTSS_Object inFileObject;
  UInt64 inFilePosition;
  void *ioBuffer;
  UInt32 inBufLen;
  UInt32 *outLenRead;
} QTSS_ReadFile_Params;

typedef struct {
  QTSS_Object inFileObject;
} QTSS_CloseFile_Params;

typedef struct {
  QTSS_Object inFileObject;
  QTSS_EventType inEventMask;
} QTSS_RequestEventFile_Params;


typedef struct {
  char *inSerial;
  char *outCMSIP;
  char *outCMSPort;
} QTSS_GetAssociatedCMS_Params;

typedef struct {
  char *inStreanID;
  char *outresult;
} QTSS_JudgeStreamID_Params;

typedef union {
  QTSS_Register_Params regParams;
  QTSS_Initialize_Params initParams;
  QTSS_ErrorLog_Params errorParams;
  QTSS_StateChange_Params stateChangeParams;

  QTSS_Filter_Params rtspFilterParams;
  QTSS_IncomingData_Params rtspIncomingDataParams;
  QTSS_StandardRTSP_Params rtspRouteParams;
  QTSS_RTSPAuth_Params rtspAthnParams;
  QTSS_StandardRTSP_Params rtspAuthParams;
  QTSS_StandardRTSP_Params rtspPreProcessorParams;
  QTSS_StandardRTSP_Params rtspRequestParams;
  QTSS_StandardRTSP_Params rtspPostProcessorParams;
  QTSS_RTSPSession_Params rtspSessionClosingParams;

  QTSS_RTPSendPackets_Params rtpSendPacketsParams;
  QTSS_ClientSessionClosing_Params clientSessionClosingParams;
  QTSS_RTCPProcess_Params rtcpProcessParams;

  QTSS_OpenFile_Params openFilePreProcessParams;
  QTSS_OpenFile_Params openFileParams;
  QTSS_AdviseFile_Params adviseFileParams;
  QTSS_ReadFile_Params readFileParams;
  QTSS_CloseFile_Params closeFileParams;
  QTSS_RequestEventFile_Params reqEventFileParams;

  QTSS_GetAssociatedCMS_Params GetAssociatedCMSParams;
  QTSS_JudgeStreamID_Params JudgeStreamIDParams;


} QTSS_RoleParams, *QTSS_RoleParamPtr;

typedef struct {
  void *packetData;
  QTSS_TimeVal packetTransmitTime;
  QTSS_TimeVal suggestedWakeupTime;
} QTSS_PacketStruct;



#ifdef __cplusplus
}
#endif

#endif
