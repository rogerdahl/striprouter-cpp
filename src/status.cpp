#include "status.h"

std::mutex statusMutex;

Status::Status() : nCombinationsChecked(0)
{
}
