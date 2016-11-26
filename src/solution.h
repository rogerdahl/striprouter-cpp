#pragma once

#include <vector>
#include <mutex>

#include "int_types.h"
#include "via.h"


extern std::mutex solutionMutex;


typedef std::vector<ViaLayer> RouteStepVec;
typedef std::vector<RouteStepVec> RouteVec;
typedef std::vector<std::string> SolutionInfoVec;

class Solution
{
public:
  Solution();

  RouteVec& getRouteVec();
  const RouteVec& getRouteVec() const;

  SolutionInfoVec& getSolutionInfoVec();
  const SolutionInfoVec& getSolutionInfoVec() const;

  void dump() const;

  void setErrorBool(bool errorBool);
  bool getErrorBool() const;

  u32 totalCost;
  u32 numCompletedRoutes;
  u32 numFailedRoutes;
  u32 ready;
private:
  SolutionInfoVec solutionInfoVec_;
  RouteVec routeVec_;
  bool hasError_;
};
