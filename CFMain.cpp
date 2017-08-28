#include <OS.h>
#include <Socket.h>
#include <SocketUtils.h>
#include <TimeoutTask.h>
#include <HTTPSessionInterface.h>
#include <HTTPListenerSocket.h>
#include <CF.h>
#include <CFState.h>

/*
 * 主线程是管理线程，不执行实际的任务；
 * TaskThread 中执行 Task::Run，EventThread 中执行 EventContext::ProcessEvent
 * TaskThreadPool，EventThread，IdleTaskThread，TimeoutTaskThread 会在主线程
 *   中创建和销毁
 * ListenerSocket::ProcessEvent 中执行 accept 和 Session 的创建，并将 osSocket
 *   注入 Session 关联的 Socket 对象
 */

int CFMain(CFConfigure *config) {

  //
  // Initialize utility classes
  OS::Initialize();
  OSThread::Initialize();

  Socket::Initialize();
  SocketUtils::Initialize(false);

#if !MACOSXEVENTQUEUE
  // initialize the select() implementation of the event queue
  ::select_startevents();
#endif

  OS::SetPersonality(config->GetPersonalityUser(),
                     config->GetPersonalityGroup());

  /*
     切换用户和组，LinuxThread库实现的线程模型，setuid() 和 setgid() 可能会
     出现不同线程中，uid 和 gid 不一致的问题
   */
  OS::SwitchPersonality();

  UInt32 numShortTaskThreads = config->GetShortTaskThreads();
  UInt32 numBlockingThreads = config->GetBlockingThreads();

  if (OS::ThreadSafe()) {
    if (numShortTaskThreads == 0) {
      UInt32 numProcessors = OS::GetNumProcessors();
      // 1 worker thread per processor, up to 2 threads.
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
  qtss_printf("Add threads short_task=%" _U32BITARG_ " "
                  "blocking=%" _U32BITARG_ "\n",
              numShortTaskThreads, numBlockingThreads);

  TaskThreadPool::SetNumShortTaskThreads(numShortTaskThreads);
  TaskThreadPool::SetNumBlockingTaskThreads(numBlockingThreads);
  TaskThreadPool::AddThreads(numThreads);

  //
  // Start up the server's global tasks, and start listening

  IdleTask::Initialize();

  // The TimeoutTask mechanism is task based,
  // we therefore must do this after adding task threads.
  // this be done before starting the sockets and server tasks
  TimeoutTask::Initialize();

  // Make sure to do this stuff last. Because these are all the threads that
  // do work in the server, this ensures that no work can go on while the
  // server is in the process of staring up
  Socket::StartThread();

  OSThread::Sleep(1000);

  HTTPSessionInterface::Initialize(config->GetHttpMapping());

  HTTPListenerSocket *httpSocket = new HTTPListenerSocket();

  CFState::sListenerSocket.EnQueue(new OSQueueElem(httpSocket));

  httpSocket->Initialize(SocketUtils::ConvertStringToAddr("127.0.0.1"), 8080);
  httpSocket->RequestEvent(EV_RE);

  while (!CFEnv::WillExit()) {
#ifdef __sgi__
    OSThread::Sleep(999);
#else
    OSThread::Sleep(1000);
#endif
  }

  //
  // exit, release resources

  // 1. stop socket listen, refuse all new connect
  CFState::WaitProcessState(CFState::kKillListener);

  // 2. stop all user task





  // 3. send kKillEvent for all task. they (except TimeoutTaskThread) will
  //    be released in TaskThread scheduler
  // Now, make sure that the server can't do any work


  // 4. release TimeoutTaskThread
  TimeoutTask::Release();

  // 5. release IdleTaskThread
  IdleTask::Release();

  // 6. release TaskThread in TaskThreadPool
  TaskThreadPool::RemoveThreads();

  // 7. release EventThread
  /*
   * Socket 析构的时候，会触发 EventContext 的 AutoCleanup，解除 ContextThread
   * 的引用表里的引用。而 Socket 将被 SessionTask 持有，所以对 EventThread 的
   * 释放的时机需要在全部 Socket 执行完 Cleanup 以后。
   */
  Socket::Release();

  // 2. stop network event
#if !MACOSXEVENTQUEUE
  ::select_stopevents();
#endif

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