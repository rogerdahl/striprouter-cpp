#pragma once

#include <mutex>

#include "int_types.h"

extern std::mutex settingsMutex;

const int DEFAULT_WIRE_COST = 1000;
const int DEFAULT_STRIP_COST = 1;
const int DEFAULT_VIA_COST = 100;

class Settings
{
public:
  Settings();
  u32 wire_cost;
  u32 strip_cost;
  u32 via_cost;
};
