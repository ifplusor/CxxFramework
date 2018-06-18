#include <sys/errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <map>

#include <CF/Core/SpinLock.h>
#include <CF/Core/Thread.h>
#include <CF/Net/ev.h>

#define MAX_EPOLL_FD 20000

using namespace CF::Core;

static int epoll_fd = -1;            // epoll 描述符
static epoll_event *_events = NULL;  // epoll 事件接收数组
static int m_curEventReadPos = 0;    // 当前读事件位置，在epoll事件数组中的位置
static int m_curTotalEvents = 0;     // 总的事件个数，每次epoll_wait之后更新
static std::map<int, void *> epollFDMap;  // 映射 fd和对应的RTSPSession对象
static SpinLock sMapLock;            // epollFDMap 自旋锁
static SpinLock sArrayLock;          // _events 自旋锁

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
 *           Socket 的话，需要再次把这个socket加入到EPOLL队列里
 */

void select_startevents() {
  if (epoll_fd == -1) {
    epoll_fd = epoll_create(MAX_EPOLL_FD);
    if (epoll_fd == -1) {
      perror("create epoll fd error: ");
      exit(-1);
    }
  }

  if (_events == NULL) {
    _events = new epoll_event[MAX_EPOLL_FD];  // we only listen the read event
    if (_events == NULL) {
      perror("new epoll_event error: ");
      exit(-1);
    }
  }

  m_curEventReadPos = 0;
  m_curTotalEvents = 0;
}

void select_stopevents() {
  if (epoll_fd != -1) {
    ::close(epoll_fd); /* 关闭文件描述符 */
    epoll_fd = -1;
  }

  delete[] _events;
  _events = NULL;
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

  if (which & EV_RE)
    ev.events |= EPOLLIN | EPOLLHUP | EPOLLERR;

  if (which & EV_WR)
    ev.events |= EPOLLOUT;

  int ret = -1;
  if (isAdd) {
    do {
      ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, req->er_handle, &ev);
    } while (ret == -1 && Thread::GetErrno() == EINTR);
  } else {
    do {
      ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, req->er_handle, &ev);
    } while (ret == -1 && Thread::GetErrno() == EINTR);
  }

  if (ret == 0) {
    epollFDMap[req->er_handle] = req->er_data;
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
  int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, which, NULL); // remove all this fd events
  if (ret == 0) {
    epollFDMap.erase(which);
  }
  return ret;
}

int epoll_waitevent() {
  int curreadPos = -1;  // m_curEventReadPos;//start from 0

  if (m_curTotalEvents <= 0) { // 当前一个epoll事件都没有的时候，执行epoll_wait
    m_curTotalEvents = 0;
    m_curEventReadPos = 0;

    int nfds = epoll_wait(epoll_fd, _events, MAX_EPOLL_FD, 15000); // 15秒超时
    if (nfds > 0) m_curTotalEvents = nfds;
  }

  if (m_curTotalEvents > 0) { // 从事件数组中每次取一个，取的位置通过m_curEventReadPos设置
    curreadPos = m_curEventReadPos++;
    if (m_curEventReadPos >= m_curTotalEvents - 1) {
      m_curEventReadPos = 0;
      m_curTotalEvents = 0;
    }
  }

  return curreadPos;
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
    req->er_handle = _events[eventPos].data.fd;
    if (_events[eventPos].events == EPOLLIN ||
        _events[eventPos].events == EPOLLHUP ||
        _events[eventPos].events == EPOLLERR) {
      req->er_eventbits = EV_RE;  // we only support read event
    } else if (_events[eventPos].events == EPOLLOUT) {
      req->er_eventbits = EV_WR;
    }
    SpinLocker locker1(&sMapLock);
    req->er_data = epollFDMap[req->er_handle];
    return 0;
  }
  return EINTR;
}
