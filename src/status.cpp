#include "status.h"


std::mutex statusMutex;

Status::Status()
  : nunCombinationsChecked(0)
{
}
