#include "solution.h"


Solution::Solution(int _gridW, int _gridH)
  : gridW(_gridW),
    gridH(_gridH),
    totalCost(0),
    numCompletedRoutes(0),
    numFailedRoutes(0),
    numShortcuts(0),
    isReady(false),
    hasError(false)
{
}

Solution::~Solution()
{
}

Solution::Solution(const Solution &s)
{
  copy(s);
}

Solution &Solution::operator=(const Solution &s)
{
  copy(s);
  return *this;
}

void Solution::copy(const Solution &s)
{
  circuit = s.circuit;
  settings = s.settings;
  totalCost = s.totalCost;
  numCompletedRoutes = s.numCompletedRoutes;
  numFailedRoutes = s.numFailedRoutes;
  numShortcuts = s.numShortcuts;
  isReady = s.isReady;
  solutionInfoVec = s.solutionInfoVec;
  routeVec = s.routeVec;
  routeStatusVec = s.routeStatusVec;
  // Nets
  viaSetVec = s.viaSetVec;
  setIdxVec = s.setIdxVec;
  // Debug
  hasError = s.hasError;
  diagStartVia = s.diagStartVia;
  diagEndVia = s.diagEndVia;
  diagCostVec = s.diagCostVec;
  diagRouteStepVec = s.diagRouteStepVec;
  diagTraceVec = s.diagTraceVec;
  errorStringVec = s.errorStringVec;
}

std::unique_lock<std::mutex> Solution::scope_lock()
{
  return std::unique_lock<std::mutex>(mutex_);
}

int Solution::idx(const Via &v)
{
  return v.x() + gridW * v.y();
}
