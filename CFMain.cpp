#include <CF/CF.h>
#include <CF/CFState.h>
#include <CF/Net/Socket/Socket.h>
#include <CF/Net/Socket/SocketUtils.h>
#include <CF/Net/Http/HTTPListenerSocket.h>
#include <CF/Net/Http/HTTPSessionInterface.h>

using namespace CF;

/*
 * 主线程是管理线程，不执行实际的任务；
 * TaskThread 中执行 Task::Run，EventThread 中执行 EventContext::ProcessEvent
 * TaskThreadPool，EventThread，IdleTaskThread，TimeoutTaskThread 会在主线程
 *   中创建和销毁
 * ListenerSocket::ProcessEvent 中执行 accept 和 Session 的创建，并将 osSocket
 *   注入 Session 关联的 Socket 对象
 */

std::atomic<UInt32> CFState::sState(0); /* 框架内部状态标识 */
Queue CFState::sListenerSocket;

int CFMain(CFConfigure *config) {
  CF_Error theErr = CF_NoErr;

  //
  // Initialize utility classes
  Core::Initialize();

  Net::Socket::Initialize();
  Net::SocketUtils::Initialize(false);

#if !MACOSXEVENTQUEUE
  // initialize the select() implementation of the event Queue
  ::select_startevents();
#endif

  Core::Utils::SetPersonality(config->GetPersonalityUser(),
                     config->GetPersonalityGroup());

  /*
     切换用户和组，LinuxThread库实现的线程模型，setuid() 和 setgid() 可能会
     出现不同线程中，uid 和 gid 不一致的问题
   */
  Core::Utils::SwitchPersonality();

  UInt32 numShortTaskThreads = config->GetShortTaskThreads();
  UInt32 numBlockingThreads = config->GetBlockingThreads();

  if (Core::Utils::ThreadSafe()) {
    if (numShortTaskThreads == 0) {
      UInt32 numProcessors = Core::Utils::GetNumProcessors();
      // 1 worker Thread per processor, up to 2 threads.
      // Note: Limiting the number of worker threads to 2 on a MacOS X system
      //     with > 2 cores results in better performance on those systems, as
      //     of MacOS X 10.5.  Future improvements should make this limit
      //     unnecessary.
      if (numProcessors > 2)
        numShortTaskThreads = 2;
      else
        numShortTaskThreads = numProcessors;
    }

    if (numBlockingThreads == 0)
      numBlockingThreads = 1;
  }

  if (numShortTaskThreads == 0)
    numShortTaskThreads = 1;

  if (numBlockingThreads == 0)
    numBlockingThreads = 1;

  UInt32 numThreads = numShortTaskThreads + numBlockingThreads;
  s_printf("Add threads short_task=%" _U32BITARG_ " "
               "blocking=%" _U32BITARG_ "\n",
           numShortTaskThreads, numBlockingThreads);

  Thread::TaskThreadPool::SetNumShortTaskThreads(numShortTaskThreads);
  Thread::TaskThreadPool::SetNumBlockingTaskThreads(numBlockingThreads);
  Thread::TaskThreadPool::AddThreads(numThreads);

  //
  // Start up the server's global tasks, and start listening

  Thread::IdleTask::Initialize();

  // The TimeoutTask mechanism is task based,
  // we therefore must do this after adding task threads.
  // this be done before starting the sockets and server tasks
  Thread::TimeoutTask::Initialize();

  // Make sure to do this stuff last. Because these are all the threads that
  // do work in the server, this ensures that no work can go on while the
  // server is in the process of staring up
  Net::Socket::StartThread();

  Core::Thread::Sleep(1000);

  // Http server configure;
  UInt32 numHttpListens;
  CF_NetAddr *httpListenAddrs = config->GetHttpListenAddr(&numHttpListens);
  if (numHttpListens > 0) {
    Net::HTTPSessionInterface::Initialize(config->GetHttpMapping());
    for (UInt32 i = 0; i < numHttpListens; i++) {
      auto *httpSocket = new Net::HTTPListenerSocket();
      theErr = httpSocket->Initialize(Net::SocketUtils::ConvertStringToAddr(
          httpListenAddrs[i].ip), httpListenAddrs[i].port);
      if (theErr == CF_NoErr) {
        CFState::sListenerSocket.EnQueue(new QueueElem(httpSocket));
        httpSocket->RequestEvent(EV_RE);
      } else {
        delete httpSocket;
      }
    }
  }

  //
  // listen status loop
  while (!CFEnv::WillExit()) {
#ifdef __sgi__
    Core::Thread::Sleep(999);
#else
    Core::Thread::Sleep(1000);
#endif
  }

  //
  // exit, release resources

  // 1. stop Socket listen, refuse all new connect
  s_printf("release: step 1, stop listen...\n");
  CFState::WaitProcessState(CFState::kKillListener);

  // 2. disable request new event, but process already exists
  s_printf("release: step 2, disable new event...\n");
  CFState::sState |= CFState::kDisableEvent;

  // 3. clean all event watch
  s_printf("release: step 3, clean event...\n");
  CFState::WaitProcessState(CFState::kCleanEvent);
  // in here, all event stop. EventThread is needless.

  // 4. release EventThread
  s_printf("release: step 4, release event thread...\n");
  Net::Socket::Release();

  // 5. stop events
  //    must after step 4, because select_waitevent is called in EventThread::Entry
  s_printf("release: step 5, stop events...\n");
#if !MACOSXEVENTQUEUE
  ::select_stopevents();
#endif

  // 6. kill TimeoutTaskThread but don't release memory
  s_printf("release: step 6, kill timeout thread...\n");
  Thread::TimeoutTask::StopTask();

  // 7. release TaskThreads in TaskThreadPool
  s_printf("release: step 7, kill task threads...\n");
  Thread::TaskThreadPool::RemoveThreads();

  // 8. release TimeoutTaskThread.
  //    must after step 7, because Task can hold TaskThreads as member
  s_printf("release: step 8, release timeout thread...\n");
  Thread::TimeoutTask::Release();

  // 9. release IdleTaskThread.
  //    must after step 8, because TimeoutTaskThread is a IdleTask,
  s_printf("release: step 9, release idle task thread...\n");
  Thread::IdleTask::Release();

  s_printf("all resources is released, the server will exit soon.\n");

  return 0;
}

int main(int argc, char **argv) {

  // initialize global information
  CFEnv::Initialize();

  // wait for user configure
  CF_Error theErr = CFInit(argc, argv);
  if (theErr != CF_NoErr) return EXIT_FAILURE;

  CFConfigure *config = CFEnv::GetConfigure();
  if (config == nullptr) return EXIT_FAILURE;

  theErr = CFMain(config);

  return CFExit(theErr);
}