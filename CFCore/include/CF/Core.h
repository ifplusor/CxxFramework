//
// Created by James on 2017/8/30.
//
/*
 * file:         Core.h
 * description:  encapsulate difference of os thread and synchronization.
 */

#ifndef __CF_CORE_H__
#define __CF_CORE_H__

#include <CF/Core/Thread.h>
#include <CF/Core/Mutex.h>
#include <CF/Core/Cond.h>
#include <CF/Core/RWMutex.h>
#include <CF/Core/Time.h>
#include <CF/Core/Utils.h>

namespace CF::Core {

/**
 * @brief set global variables about time.
 * @note call this before calling anything else
 */
static void Initialize() {
  Time::Initialize();
  Thread::Initialize();
}

}

#endif // __CF_CORE_H__
