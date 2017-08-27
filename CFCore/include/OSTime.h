//
// Created by james on 8/26/17.
//

#ifndef __OS_TIME_H__
#define __OS_TIME_H__

#include <OSHeaders.h>

class OSTime {
 public:

  static void Initialize();

  /**
   * Milliseconds always returns milliseconds since Jan 1, 1970 GMT.
   * This basically makes it the same as a POSIX time_t value, except
   * in msec, not seconds. To convert to a time_t, divide by 1000.
   */
  static SInt64 Milliseconds();

  static SInt64 Microseconds();

  static SInt64 TimeMilli_To_Fixed64Secs(SInt64 inMilliseconds);

  static SInt64 Fixed64Secs_To_TimeMilli(SInt64 inFixed64Secs) {
    UInt64 value = (UInt64) inFixed64Secs;
    return (value >> 32) * 1000 + (((value % ((UInt64) 1 << 32)) * 1000) >> 32);
  }

  //This converts the local time (from OS::Milliseconds) to NTP time.
  static SInt64 TimeMilli_To_1900Fixed64Secs(SInt64 inMilliseconds) {
    return TimeMilli_To_Fixed64Secs(sMsecSince1900)
        + TimeMilli_To_Fixed64Secs(inMilliseconds);
  }

  static SInt64 TimeMilli_To_UnixTimeMilli(SInt64 inMilliseconds) {
    return inMilliseconds;
  }

  static time_t TimeMilli_To_UnixTimeSecs(SInt64 inMilliseconds) {
    return (time_t) ((SInt64) TimeMilli_To_UnixTimeMilli(inMilliseconds)
                         / (SInt64) 1000);
  }

  // Seconds since 1970
  static time_t UnixTime_Secs(void) {
    return TimeMilli_To_UnixTimeSecs(Milliseconds());
  }

  static time_t Time1900Fixed64Secs_To_UnixTimeSecs(SInt64 in1900Fixed64Secs) {
    return (time_t) ((SInt64) ((SInt64)
        (in1900Fixed64Secs - TimeMilli_To_Fixed64Secs(sMsecSince1900))
            / ((SInt64) 1 << 32)));
  }

  static SInt64 Time1900Fixed64Secs_To_TimeMilli(SInt64 in1900Fixed64Secs) {
    return ((SInt64) ((Float64) ((SInt64)
        in1900Fixed64Secs - (SInt64) TimeMilli_To_Fixed64Secs(sMsecSince1900))
                          / (Float64) ((SInt64) 1 << 32)) * 1000);
  }

  static int GetTimeOfDay(struct timeval *tv);

  static SInt64 InitialMSec() { return sInitialMsec; }
  static Float32 StartTimeMilli_Float() {
    return (Float32) ((Float64)
        (OSTime::Milliseconds() - OSTime::InitialMSec()) / (Float64) 1000.0);
  }
  static SInt64 StartTimeMilli_Int() {
    return (OSTime::Milliseconds() - OSTime::InitialMSec());
  }

  // Returns the offset in hours between local time and GMT (or UTC) time.
  static SInt32 GetGMTOffset();

 private:
#if !defined(__Win32__) && !defined(EASY_DEVICE) && !defined(__MinGW__)
  static inline int getCpuSpeed_mhz(unsigned int wait_us);
  static int getCpuSpeed();

  static struct timeval walltime;
  static u_int64_t walltick;
  static int cpuspeed_mhz;
#endif

  static SInt64 sMsecSince1900;
  static SInt64 sMsecSince1970;
  static SInt64 sInitialMsec;
  static SInt64 sWrapTime;
  static SInt64 sCompareWrap;
  static SInt64 sLastTimeMilli;
};

#endif // __OS_TIME_H__
