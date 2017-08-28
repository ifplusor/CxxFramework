#if defined(__linux__)

#include <string.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <map>

#include <OSMutex.h>
#include "ev.h"

using namespace std;

#define MAX_EPOLL_FD    20000

static int epoll_fd = 0;             // epoll 描述符
static epoll_event *_events = NULL;  // epoll 事件接收数组
static int m_curEventReadPos = 0;    // 当前读事件位置，在epoll事件数组中的位置
static int m_curTotalEvents = 0;     // 总的事件个数，每次epoll_wait之后更新
static OSMutex sMaxFDPosMutex;       // 锁
static bool canEpoll = false;        // 是否可以执行epoll_wait,避免频繁执行，浪费CPU
static map<int, void *> epollFDMap;  // 映射 fd和对应的RTSPSession对象

/* epoll event:
 *
 * EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
 * EPOLLOUT：表示对应的文件描述符可以写；
 * EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
 * EPOLLERR：表示对应的文件描述符发生错误；
 * EPOLLHUP：表示对应的文件描述符被挂断；
 * EPOLLET： 将EPOLL设为边缘触发（Edge Triggered）模式，这是相对于水平触发
 *           （Level Triggered）来说的。
 * EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个
 *           socket 的话，需要再次把这个socket加入到EPOLL队列里
 */

void select_startevents() {
  epoll_fd = epoll_create(MAX_EPOLL_FD);

  if (_events == NULL) {
    _events = new epoll_event[MAX_EPOLL_FD];  // we only listen the read event
  }
  if (_events == NULL) {
    perror("new epoll_event error: ");
    exit(1);
  }
  m_curEventReadPos = 0;
  m_curTotalEvents = 0;
}

void select_stopevents() {
  close(epoll_fd); /* 关闭文件描述符 */
  epoll_fd = -1;
  delete[] _events;
  _events = NULL;
}

int select_modwatch(struct eventreq *req, int which) {
  if (req == NULL) {
    return -1;
  }
  struct epoll_event ev;
  memset(&ev, 0x0, sizeof(ev));

  int ret = -1;
  // 加锁，防止线程池中的多个线程执行该函数，导致插入监听事件失败
  OSMutexLocker locker(&sMaxFDPosMutex);
  if (which == EV_RE) {
    ev.data.fd = req->er_handle;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;  // level trigger

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, req->er_handle, &ev);
  } else if (which == EV_WR) {
    ev.data.fd = req->er_handle;
    ev.events = EPOLLOUT;  // level trigger

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, req->er_handle, &ev);
  } else if (which == EV_RM) {
    ret = epoll_ctl(epoll_fd,
                    EPOLL_CTL_DEL,
                    req->er_handle,
                    NULL);  // remove all this fd events
  } else { // epoll can not listen RESET
    // we don't needed
  }

  epollFDMap[req->er_handle] = req->er_data;
  canEpoll = true;  // 每次新增事件后可以马上执行epoll_wait，新增了监听事件，意味着接下来可能马上要发生事件在这个fd上
  return 0;
}

int select_watchevent(struct eventreq *req, int which) {
  return select_modwatch(req, which);
}

int select_removeevent(int which) {
  OSMutexLocker locker(&sMaxFDPosMutex);
  int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, which, NULL); // remove all this fd events
  canEpoll = true; // 每删除一个fd后，可以马上执行epoll_wait，可能在删除掉的fd上出现读异常
  return 0;
}

int epoll_waitevent() {
  if (canEpoll == false) {
    return -1;
  }

  int nfds = 0;
  int curreadPos = -1;  // m_curEventReadPos;//start from 0
  if (m_curTotalEvents <= 0) { // 当前一个epoll事件都没有的时候，执行epoll_wait
    m_curTotalEvents = 0;
    m_curEventReadPos = 0;
    nfds = epoll_wait(epoll_fd, _events, MAX_EPOLL_FD, 15000); // 15秒超时

    if (nfds > 0) {
      canEpoll = false; // wait到了epoll事件，先处理完再wait
      m_curTotalEvents = nfds;
    } else if (nfds < 0) {
      // error
    } else if (nfds == 0) {
      canEpoll = true; // 这一次wait超时了，下一次继续wait
    }
  }

  if (m_curTotalEvents) { // 从事件数组中每次取一个，取的位置通过m_curEventReadPos设置
    curreadPos = m_curEventReadPos++;
    if (m_curEventReadPos >= m_curTotalEvents - 1) {
      m_curEventReadPos = 0;
      m_curTotalEvents = 0;
    }
  }

  return curreadPos;
}

/*
 * 每次 wait 完成，都会清除 fd
 */
int select_waitevent(struct eventreq *req, void *onlyForMOSX) {
  int eventPos = epoll_waitevent();
  if (eventPos >= 0) {
    req->er_handle = _events[eventPos].data.fd;
    if (_events[eventPos].events == EPOLLIN ||
        _events[eventPos].events == EPOLLHUP ||
        _events[eventPos].events == EPOLLERR) {
      req->er_eventbits = EV_RE;  // we only support read event
    } else if (_events[eventPos].events == EPOLLOUT) {
      req->er_eventbits = EV_WR;
    }
    req->er_data = epollFDMap[req->er_handle];
    OSMutexLocker locker(&sMaxFDPosMutex);
    select_removeevent(req->er_handle);
    return 0;
  }
  return EINTR;
}

#endif

