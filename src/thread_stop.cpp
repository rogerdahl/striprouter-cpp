#include <cassert>

#include "thread_stop.h"

ThreadStop::ThreadStop() : lock_(mutex_, std::defer_lock)
{
}

void ThreadStop::stop()
{
  assert(!lock_.owns_lock());
  lock_.lock();
}

bool ThreadStop::isStopped()
{
  return lock_.owns_lock();
}
