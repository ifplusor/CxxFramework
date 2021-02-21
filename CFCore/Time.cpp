//
// Created by james on 8/26/17.
//

#include <math.h>
#include <string.h>
#include <CF/Core/Time.h>

#if __macOS__

#ifndef __COREFOUNDATION__
#include <CoreFoundation/CoreFoundation.h>
//extern "C" { void Microseconds (UnsignedWide *microTickCount); }
#endif

#endif

#if DEBUG || __Win32__ || __MinGW__
#include <CF/Core/Mutex.h>
#endif

using namespace CF::Core;

#if DEBUG || __Win32__ || __MinGW__
static Mutex* sLastMillisMutex = nullptr;
#endif

SInt64  Time::sMsecSince1970 = 0;
SInt64  Time::sMsecSince1900 = 0;
SInt64  Time::sInitialMsec = 0;
SInt64  Time::sWrapTime = 0;
SInt64  Time::sCompareWrap = 0;
SInt64  Time::sLastTimeMilli = 0;

void Time::Initialize() {
  Assert(sInitialMsec == 0);  // do only once
  if (sInitialMsec != 0) return;
  ::tzset();

  //setup t0 value for msec since 1900

  //t.tv_sec is number of seconds since Jan 1, 1970. Convert to seconds since 1900
  SInt64 the1900Sec = (SInt64) (24 * 60 * 60) * (SInt64) ((70 * 365) + 17);
  sMsecSince1900 = the1900Sec * 1000;

  sWrapTime = (SInt64) (0x00000001ULL << 32U);
  sCompareWrap = (SInt64) (0xffffffffULL << 32U);
  sLastTimeMilli = 0;

  //Milliseconds uses sInitialMsec so this assignment is valid only once.
  sInitialMsec = Time::Milliseconds();

  sMsecSince1970 = ::time(nullptr);
  sMsecSince1970 *= 1000;           // Convert to msec


#if DEBUG || __Win32__ || __MinGW__
  sLastMillisMutex = new Mutex();
#endif
}

SInt64 Time::Milliseconds() {
  /*
  #if __macOS__

  #if DEBUG
      OSMutexLocker locker(sLastMillisMutex);
  #endif

     UnsignedWide theMicros;
      ::Microseconds(&theMicros);
      SInt64 scalarMicros = theMicros.hi;
      scalarMicros <<= 32;
      scalarMicros += theMicros.lo;
      scalarMicros = ((scalarMicros / 1000) - sInitialMsec) + sMsecSince1970; // convert to msec

  #if DEBUG
      static SInt64 sLastMillis = 0;
      //Assert(scalarMicros >= sLastMillis); // currently this fails on dual processor machines
      sLastMillis = scalarMicros;
  #endif
      return scalarMicros;
  */
#if __Win32__ || __MinGW__
  MutexLocker locker(sLastMillisMutex);
  // curTimeMilli = timeGetTime() + ((sLastTimeMilli/ 2^32) * 2^32)
  // using binary & to reduce it to one operation from two
  // sCompareWrap and sWrapTime are constants that are never changed
  // sLastTimeMilli is updated with the curTimeMilli after each call to this function
  SInt64 curTimeMilli = (UInt32) ::timeGetTime() + (sLastTimeMilli & sCompareWrap);
  if ((curTimeMilli - sLastTimeMilli) < 0)
  {
      curTimeMilli += sWrapTime;
  }
  sLastTimeMilli = curTimeMilli;

  // For debugging purposes
  //SInt64 tempCurMsec = (curTimeMilli - sInitialMsec) + sMsecSince1970;
  //SInt32 tempCurSec = tempCurMsec / 1000;
  //char buffer[kTimeStrSize];
  //s_printf("OS::MilliSeconds current Time = %s\n", s_ctime(&tempCurSec, buffer, sizeof(buffer)));

  return (curTimeMilli - sInitialMsec) + sMsecSince1970; // convert to application Time
#else
  struct timeval t;
#if !defined(EASY_DEVICE)
  int theErr = Time::GetTimeOfDay(&t);
#else
  int theErr = ::gettimeofday(&t, nullptr);
#endif
  Assert(theErr == 0);

  SInt64 curTime;
  curTime = t.tv_sec;
  curTime *= 1000;                // sec -> msec
  curTime += t.tv_usec / 1000;    // usec -> msec

  return (curTime - sInitialMsec) + sMsecSince1970;
#endif

}

SInt64 Time::Microseconds() {
  /*
  #if __macOS__
      UnsignedWide theMicros;
      ::Microseconds(&theMicros);
      SInt64 theMillis = theMicros.hi;
      theMillis <<= 32;
      theMillis += theMicros.lo;
      return theMillis;
  */
#if __Win32__ || __MinGW__
  SInt64 curTime = (SInt64)::timeGetTime(); //  system Time in milliseconds
  curTime -= sInitialMsec; // convert to application Time
  curTime *= 1000; // convert to microseconds
  return curTime;
#else
  struct timeval t;
#if !defined(EASY_DEVICE)
  int theErr = Time::GetTimeOfDay(&t);
#else
  int theErr = ::gettimeofday(&t, nullptr);
#endif
  Assert(theErr == 0);

  SInt64 curTime;
  curTime = t.tv_sec;
  curTime *= 1000000;     // sec -> usec
  curTime += t.tv_usec;

  return curTime - (sInitialMsec * 1000);
#endif
}

// CISCO provided fix for integer + fractional fixed64.
SInt64 Time::TimeMilli_To_Fixed64Secs(SInt64 inMilliseconds) {
  SInt64 result = inMilliseconds / 1000;  // The result is in lower bits.
  result <<= 32;  // shift it to higher 32 bits
  // Take the remainder (rem = inMilliseconds%1000) and multiply by
  // 2**32, divide by 1000, effectively this gives (rem/1000) as a
  // binary fraction.
  double p = ldexp((double) (inMilliseconds % 1000), +32) / 1000.;
  UInt32 frac = (UInt32) p;
  result |= frac;
  return result;
}

#if !defined(__Win32__) && !defined(EASY_DEVICE) && !defined(__MinGW__)
#include <sys/time.h>

#define RELOAD_TIME_US 1

#define rdtscll(val) {                               \
  UInt32 __a, __d;                                   \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d));    \
  (val) = ((UInt64) __a) | (((UInt64) __d) << 32);   \
}

#define TIME_ADD_US(a, usec) {        \
  (a)->tv_usec += usec;               \
  while((a)->tv_usec >= 1000000) {    \
  (a)->tv_sec ++;                     \
  (a)->tv_usec -= 1000000;            \
  }                                   \
}

struct timeval Time::walltime;
u_int64_t Time::walltick;
int Time::cpuspeed_mhz;

inline int Time::getCpuSpeed_mhz(unsigned int wait_us) {
  u_int64_t tsc1, tsc2;
  struct timespec t = {
      .tv_sec = 0,
      .tv_nsec = wait_us * 1000
  };

  rdtscll(tsc1);

  // If sleep failed, result is unexpected, the caller should retry
  if (nanosleep(&t, NULL))
    return -1;
  rdtscll(tsc2);
  return (tsc2 - tsc1) / (wait_us);
}

int Time::getCpuSpeed() {
  static int speed = -1;
  while (speed < 100)
    speed = getCpuSpeed_mhz(50 * 1000);
  return speed;
}

int Time::GetTimeOfDay(struct timeval *tv) {
  u_int64_t tick = 0;

  // TSC偏移大于这个值，就需要重新获得系统时间
  static unsigned int max_ticks = 80000000;
  rdtscll(tick);
  if (walltime.tv_sec == 0 || cpuspeed_mhz == 0 ||
      (tick - walltick) > max_ticks) {
    if (tick == 0 || cpuspeed_mhz == 0) {
      cpuspeed_mhz = getCpuSpeed();
      max_ticks = cpuspeed_mhz * RELOAD_TIME_US;
    }
    gettimeofday(tv, NULL);
    ::memcpy(&walltime, tv, sizeof(walltime));
    rdtscll(walltick);
    return 0;
  }

  ::memcpy(tv, &walltime, sizeof(walltime));
  // if RELOAD_TIME_US is 1, we are in the same us, no need to adjust tv
#if RELOAD_TIME_US > 1
  {
    uint32_t t;
    t = ((uint32_t)tick) / cpuspeed_mhz;
    TIME_ADD_US(tv, t);//add 1 us
  }
#endif
  return 0;
}

#endif

SInt32 Time::GetGMTOffset() {
#if defined(__Win32__) || defined(__MinGW__)
  TIME_ZONE_INFORMATION tzInfo;
  DWORD theErr = ::GetTimeZoneInformation(&tzInfo);
  if (theErr == TIME_ZONE_ID_INVALID)
      return 0;

  return ((tzInfo.Bias / 60) * -1);
#else

  time_t clock = 0; //Make 'clock' initialized for valgrind
  struct tm *tmptr = localtime(&clock);
  if (tmptr == nullptr)
    return 0;

  return tmptr->tm_gmtoff / 3600; //convert seconds to hours before or after GMT
#endif
}
