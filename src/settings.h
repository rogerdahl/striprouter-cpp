#pragma once

#include <mutex>


const int DEFAULT_WIRE_COST = 1000;

const int DEFAULT_STRIP_COST = 1;

const int DEFAULT_VIA_COST = 100;

class Settings
{
public:
  Settings();
  int wire_cost;
  int strip_cost;
  int via_cost;
  bool pause;
};
