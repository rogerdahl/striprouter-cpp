#pragma once

#include <mutex>
#include <thread>

class ThreadStop
{
  public:
  ThreadStop();
  void stop();
  bool isStopped();

  private:
  std::mutex mutex_;
  std::unique_lock<std::mutex> lock_;
};
