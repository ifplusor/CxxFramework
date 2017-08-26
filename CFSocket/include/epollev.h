#ifndef __EPOLL_EVENT_H__
#define __EPOLL_EVENT_H__

#include "ev.h"

#if defined(__linux__)

int epoll_stopevents();

#endif

#endif // __EPOLL_EVENT_H__

