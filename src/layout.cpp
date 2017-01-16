#include <fmt/format.h>

#include "layout.h"

Layout::Layout()
  : gridW(0),
    gridH(0),
    cost(0),
    nCompletedRoutes(0),
    nFailedRoutes(0),
    numShortcuts(0),
    isReadyForRouting(false),
    isReadyForEval(false),
    hasError(false)
{
  updateBaseTimestamp();
}

Layout::Layout(const Layout& s)
{
  copy(s);
}

Layout& Layout::operator=(const Layout& s)
{
  copy(s);
  return *this;
}

void Layout::copy(const Layout& s)
{
  circuit = s.circuit;
  settings = s.settings;
  gridW = s.gridW;
  gridH = s.gridH;
  cost = s.cost;
  nCompletedRoutes = s.nCompletedRoutes;
  nFailedRoutes = s.nFailedRoutes;
  numShortcuts = s.numShortcuts;
  isReadyForRouting = s.isReadyForRouting;
  isReadyForEval = s.isReadyForEval;
  hasError = s.hasError;
  layoutInfoVec = s.layoutInfoVec;
  routeVec = s.routeVec;
  stripCutVec = s.stripCutVec;
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

void Layout::updateBaseTimestamp()
{
  timestamp_ = std::chrono::high_resolution_clock::now();
}

bool Layout::isBasedOn(const Layout& other)
{
  return timestamp_ == other.timestamp_;
}

Timestamp& Layout::getBaseTimestamp()
{
  return timestamp_;
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

// This method of discovering if this layout is locked is potentially
// problematic and I'm only using it in asserts that are active in debug builds,
// where I expect the layout to already be locked. The problem is that the
// function briefly locks the layout, and so can cause other threads that check
// simultaneously to get a false "true" from isLocked().
bool Layout::isLocked()
{
  auto lock = std::unique_lock<std::mutex>(mutex_, std::defer_lock);
  auto wasLocked = !lock.try_lock();
  if (!wasLocked) {
    lock.release();
  }
  //  fmt::print("{}\n", isLocked);
  //  return std::unique_lock<std::mutex>::try_lock(mutex_);
  //  return unique_lock_.owns_lock();
  return wasLocked;
}

int Layout::idx(const Via& v)
{
  return v.x() + gridW * v.y();
}
