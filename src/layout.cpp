#include "layout.h"

Layout::Layout()
  : gridW(0),
    gridH(0),
    totalCost(0),
    numCompletedRoutes(0),
    numFailedRoutes(0),
    numShortcuts(0),
    isReadyForRouting(false),
    isReadyForEval(false),
    hasError(false)
{
  setOriginal();
}

Layout::Layout(const Layout &s)
{
  copy(s);
}

Layout &Layout::operator=(const Layout &s)
{
  copy(s);
  return *this;
}

void Layout::copy(const Layout &s)
{
  circuit = s.circuit;
  settings = s.settings;
  gridW = s.gridW;
  gridH = s.gridH;
  totalCost = s.totalCost;
  numCompletedRoutes = s.numCompletedRoutes;
  numFailedRoutes = s.numFailedRoutes;
  numShortcuts = s.numShortcuts;
  isReadyForRouting = s.isReadyForRouting;
  isReadyForEval = s.isReadyForEval;
  hasError = s.hasError;
  layoutInfoVec = s.layoutInfoVec;
  routeVec = s.routeVec;
  routeStatusVec = s.routeStatusVec;
  // Nets
  viaSetVec = s.viaSetVec;
  setIdxVec = s.setIdxVec;
  // Debug
  diagStartVia = s.diagStartVia;
  diagEndVia = s.diagEndVia;
  diagCostVec = s.diagCostVec;
  diagRouteStepVec = s.diagRouteStepVec;
  diagTraceVec = s.diagTraceVec;
  errorStringVec = s.errorStringVec;
  // Private
  timestamp_ = s.timestamp_;
}

void Layout::setOriginal() {
  timestamp_ = std::chrono::high_resolution_clock::now();
}

bool Layout::isBasedOn(const Layout& other) {
  return timestamp_ == other.timestamp_;
}

std::unique_lock<std::mutex> Layout::scopeLock()
{
  return std::unique_lock<std::mutex>(mutex_);
}

Layout Layout::threadSafeCopy()
{
  auto lock = scopeLock();
  Layout l = *this;
  return l;
}

int Layout::idx(const Via &v)
{
  return v.x() + gridW * v.y();
}


