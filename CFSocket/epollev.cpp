#include <sys/errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <map>

#include <CF/Core/SpinLock.h>
#include <CF/Core/Thread.h>
#include <CF/Net/ev.h>

/* epoll pool size */
#ifndef MAX_EPOLL_FD
#define MAX_EPOLL_FD 20000
#endif

using namespace CF::Core;

static int gEpollFD = -1;                // epoll 描述符
static epoll_event *gEpollEvents = NULL; // epoll 事件接收数组
static int gCurEventReadPos = 0;         // 当前读事件位置，在epoll事件数组中的位置
static int gCurTotalEvents = 0;          // 总的事件个数，每次epoll_wait之后更新
static std::map<int, void *> gDataMap;   // 映射 fd和对应的RTSPSession对象
static SpinLock sMapLock;                // epollFDMap 自旋锁
static SpinLock sArrayLock;              // _events 自旋锁

/*
 * epoll event:
 *
 * EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
 * EPOLLOUT：表示对应的文件描述符可以写；
 * EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
 * EPOLLERR：表示对应的文件描述符发生错误；
 * EPOLLHUP：表示对应的文件描述符被挂断；
 * EPOLLET： 将EPOLL设为边缘触发（Edge Triggered）模式，这是相对于水平触发
 *           （Level Triggered）来说的。
 * EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个
 *           Socket 的话，需要再次把这个socket加入到EPOLL队列里
 */

void select_startevents() {
  if (gEpollFD == -1) {
    gEpollFD = epoll_create(MAX_EPOLL_FD);
    if (gEpollFD == -1) {
      perror("create gEpollFD error: ");
      exit(-1);
    }
  }

  if (gEpollEvents == NULL) {
    gEpollEvents = new epoll_event[MAX_EPOLL_FD];  // we only listen the read event
    if (gEpollEvents == NULL) {
      perror("new gEpollEvents error: ");
      exit(-1);
    }
  }

  gCurEventReadPos = 0;
  gCurTotalEvents = 0;
}

void select_stopevents() {
  if (gEpollFD != -1) {
    ::close(gEpollFD); /* 关闭文件描述符 */
    gEpollFD = -1;
  }

  delete[] gEpollEvents;
  gEpollEvents = NULL;
}

int select_modwatch0(struct eventreq *req, int which, bool isAdd) {
  if (req == NULL) return -1;

  // 加锁，防止线程池中的多个线程执行该函数，导致插入监听事件失败
  SpinLocker locker(&sMapLock);

  struct epoll_event ev;
  ev.data.fd = req->er_handle;
  ev.events = 0;

  if (which & EV_ET)
    ev.events |= EPOLLET;  // Edge Triggered

  if (which & EV_OS)
    ev.events |= EPOLLONESHOT;  // one shot

  if (which & EV_RE)
    ev.events |= EPOLLIN | EPOLLHUP | EPOLLERR;

  if (which & EV_WR)
    ev.events |= EPOLLOUT;

  int ret = -1;
  if (isAdd) {
    do {
      ret = epoll_ctl(gEpollFD, EPOLL_CTL_ADD, req->er_handle, &ev);
    } while (ret == -1 && Thread::GetErrno() == EINTR);
  } else {
    do {
      ret = epoll_ctl(gEpollFD, EPOLL_CTL_MOD, req->er_handle, &ev);
    } while (ret == -1 && Thread::GetErrno() == EINTR);
  }

  if (ret == 0) {
    gDataMap[req->er_handle] = req->er_data;
  }

  return ret;
}

int select_modwatch(struct eventreq *req, int which) {
  return select_modwatch0(req, which, false);
}

int select_watchevent(struct eventreq *req, int which) {
  return select_modwatch0(req, which, true);
}

int select_removeevent(int which) {
  SpinLocker locker(&sMapLock);
  int ret = epoll_ctl(gEpollFD, EPOLL_CTL_DEL, which, NULL); // remove all this fd events
  if (ret == 0) {
    gDataMap.erase(which);
  }
  return ret;
}

int epoll_waitevent() {
  int curReadPos = -1;

  if (gCurTotalEvents <= 0) { // 当前一个epoll事件都没有的时候，执行 epoll_wait
    gCurTotalEvents = epoll_wait(gEpollFD, gEpollEvents, MAX_EPOLL_FD, 15000); // 15秒超时
    gCurEventReadPos = 0;
  }

  if (gCurTotalEvents > 0) { // 从事件数组中每次取一个，取的位置通过 gCurEventReadPos 设置
    curReadPos = gCurEventReadPos++;
    if (gCurEventReadPos >= gCurTotalEvents) {
      gCurTotalEvents = 0;
    }
  }

  return curReadPos;
}

/**
 * 等待事件到来
 *
 * @note Edge Triggered 模型
 */
int select_waitevent(struct eventreq *req, void *onlyForMOSX) {
  SpinLocker locker(&sArrayLock);
  int eventPos = epoll_waitevent();
  if (eventPos >= 0) {
    req->er_handle = gEpollEvents[eventPos].data.fd;
    if (gEpollEvents[eventPos].events == EPOLLIN ||
        gEpollEvents[eventPos].events == EPOLLHUP ||
        gEpollEvents[eventPos].events == EPOLLERR) {
      if (gEpollEvents[eventPos].events != EPOLLIN) {
        DEBUG_LOG(0, "active non-in event=%u\n", gEpollEvents[eventPos].events);
      }
      req->er_eventbits = EV_RE;  // we only support read event
    } else if (gEpollEvents[eventPos].events == EPOLLOUT) {
      req->er_eventbits = EV_WR;
    }
    SpinLocker locker1(&sMapLock);
    req->er_data = gDataMap[req->er_handle];
    return 0;
  }
  return EINTR;
}
