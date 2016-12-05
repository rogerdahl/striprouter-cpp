#pragma once

#include <mutex>


extern std::mutex statusMutex;

class Status
{
public:
  Status();
  int nunCombinationsChecked;
};
