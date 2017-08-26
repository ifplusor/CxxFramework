#include <OS.h>
#include <Socket.h>
#include <SocketUtils.h>
#include <TimeoutTask.h>
#include <HTTPSessionInterface.h>
#include <HTTPListenerSocket.h>
#include <CFEnv.h>
#include <CFConfigure.h>

int CFMain(void) {

  //
  // Initialize utility classes
  OS::Initialize();
  OSThread::Initialize();

  Socket::Initialize();
  SocketUtils::Initialize(false);

#if !MACOSXEVENTQUEUE
  //initialize the select() implementation of the event queue
  ::select_startevents();
#endif

  OS::SetPersonality(CFConfigure::GetPersonalityUser(),
                     CFConfigure::GetPersonalityGroup());

  // 切换用户和组，LinuxThread库实现的线程模型，setuid() 和 setgid() 可能会
  // 出现不同线程中，uid 和 gid 不一致的问题
  OS::SwitchPersonality();

  UInt32 numShortTaskThreads = CFConfigure::GetShortTaskThreads();
  UInt32 numBlockingThreads = CFConfigure::GetBlockingThreads();

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
  qtss_printf("Add threads short_task=%" _U32BITARG_
                  " blocking=%" _U32BITARG_ "\n",
              numShortTaskThreads, numBlockingThreads);

  TaskThreadPool::SetNumShortTaskThreads(numShortTaskThreads);
  TaskThreadPool::SetNumBlockingTaskThreads(numBlockingThreads);
  TaskThreadPool::AddThreads(numThreads);

  //
  // Start up the server's global tasks, and start listening

  // The TimeoutTask mechanism is task based,
  // we therefore must do this after adding task threads.
  // this be done before starting the sockets and server tasks
  TimeoutTask::Initialize();

  // Make sure to do this stuff last. Because these are all the threads that
  // do work in the server, this ensures that no work can go on while the
  // server is in the process of staring up
  IdleTask::Initialize();
  Socket::StartThread();

  OSThread::Sleep(1000);

  HTTPSessionInterface::Initialize(CFConfigure::GetHttpMapping());

  HTTPListenerSocket *httpSocket = new HTTPListenerSocket();
  httpSocket->Initialize(0, 8081);
  httpSocket->RequestEvent(EV_RE);

  while (!CFEnv::WillExit()) {
#ifdef __sgi__
    OSThread::Sleep(999);
#else
    OSThread::Sleep(1000);
#endif
  }

  // Now, make sure that the server can't do any work
  TaskThreadPool::RemoveThreads();

  return 0;
}

int main(int argc, void **argv) {

  // initialize global information
  CFEnv::Initialize();

  // wait for user configure
  CFConfigure::Initialize(argc, argv);

  return CFMain();
}