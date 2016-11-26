#pragma once

#include <mutex>

#include "int_types.h"


extern std::mutex statusMutex;


class Status
{
public:
  Status();
  u32 bestCost;
  u32 nChecked;
};
