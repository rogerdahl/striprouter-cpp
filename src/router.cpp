#include <algorithm>
#include <mutex>
#include <thread>
#include <set>

#include <fmt/format.h>

#include "router.h"
#include "ucs.h"


Router::Router(Solution &_solution)
  : solution_(_solution), nets_(_solution),

    allPinSet(ViaSet([](const Via &a, const Via &b) -> bool
                     {
                       return std::lexicographical_compare(a.data(),
                                                           a.data() + a.size(),
                                                           b.data(),
                                                           b.data() + b.size());
                     }))
{
  viaTraceVec_ = ViaTraceVec(solution_.gridW * solution_.gridH);
}

void Router::route(std::mutex &stopThreadMutex)
{
  blockComponentFootprints();
  registerAllComponentPins();
  routeAll(stopThreadMutex);
#ifndef NDEBUG
  solution_.diagTraceVec = viaTraceVec_;
  // Enable rendering of diags
//  solution_.hasError = true;
#endif
}

void Router::registerAllComponentPins()
{
  for (auto &ci : solution_.circuit.componentNameToInfoMap) {
    const auto &componentName = ci.first;
    auto pinViaVec = solution_.circuit.calcComponentPins(componentName);
    for (auto via : pinViaVec) {
      allPinSet.insert(via);
    }
  }
}

//
// Blocking
//

// Blocking only applies to the wire layer.

void Router::blockComponentFootprints()
{
  // Block the entire component footprint on the wire layer
  for (auto &ci : solution_.circuit.componentNameToInfoMap) {
    const auto &componentName = ci.first;
    auto footprint = solution_.circuit.calcComponentFootprint(componentName);
    for (int y = footprint.start.y(); y <= footprint.end.y(); ++y) {
      for (int x = footprint.start.x(); x <= footprint.end.x(); ++x) {
        block(Via(x, y));
      }
    }
  }
}

void Router::blockRoute(const RouteStepVec &routeStepVec)
{
  for (auto &c : routeStepVec) {
    if (c.isWireLayer) {
      block(c.via);
    }
  }
}

void Router::block(const Via &via)
{
  viaTraceVec_[solution_.idx(via)].isWireSideBlocked = true;
}

bool Router::isBlocked(const Via &via)
{
  return viaTraceVec_[solution_.idx(via)].isWireSideBlocked;
}

void Router::routeAll(std::mutex &stopThreadMutex)
{
  random_shuffle(solution_.circuit.connectionVec.begin(), solution_.circuit.connectionVec.end());

#ifndef NDEBUG
//  int breakIdx = 0;
#endif

  for (auto viaStartEnd : solution_.circuit.genConnectionViaVec()) {
    findCompleteRoute(viaStartEnd);

#ifndef NDEBUG
//    if (++breakIdx == 3) {
//      solution_.hasError = true;
//      solution_.errorStringVec.push_back(fmt::format("Forced exit after route {}", breakIdx));
#endif

    if (solution_.hasError) {
      solution_.errorStringVec.push_back(fmt::format("Detected in Router::routeAll()"));
      break;
    }

    if (isRouterStopRequested(stopThreadMutex)) {
      break;
    }
  }

  solution_.isReady = true;
}

//
// Private
//

// If, when routing from A to B, the router finds shortcut that routes to an equivalent of B, B remains unconnected
// and an additional reverse route must be done from B to A. This function handles the potential extra routing.
void Router::findCompleteRoute(const ViaStartEnd &viaStartEnd)
{
  Via shortcutEndVia;
  auto routeWasFound = findRoute(shortcutEndVia, viaStartEnd);
//  if (!(shortcutEndVia == viaStartEnd.end).all()) {
//    ++solution_.numShortcuts;
//    auto reverseStartEnd = ViaStartEnd(viaStartEnd.end, viaStartEnd.start);
//    routeWasFound = findRoute(shortcutEndVia, reverseStartEnd);
//  }
//  // If reverse route was required, both routes must be successful before we count the input connection as successfully
//  // routed.
  if (routeWasFound) {
    ++solution_.numCompletedRoutes;
    solution_.routeStatusVec.push_back(true);
  }
  else {
    ++solution_.numFailedRoutes;
    solution_.routeStatusVec.push_back(false);
  }
}

bool Router::findRoute(Via& shortcutEndVia, const ViaStartEnd &viaStartEnd)
{
  UniformCostSearch ucs(*this, solution_, nets_, shortcutEndVia, viaStartEnd);
  auto routeStepVec = ucs.findLowestCostRoute();

  if (solution_.hasError) {
    solution_.errorStringVec.push_back(fmt::format("Detected in Router::findCompleteRoute()"));
    return false;
  }

  if (!routeStepVec.size()) {
    // Push an empty routeSectionVec so that it and the values in routeStatusVec line up.
    solution_.routeVec.push_back(RouteSectionVec());
    return false;
  }

  blockRoute(routeStepVec);

  nets_.joinEquivalentRoute(routeStepVec);
  assert(nets_.isEquivalentVia(routeStepVec[0].via, routeStepVec[1].via));

  auto routeSectionVec = condenseRoute(routeStepVec);

#ifndef NDEBUG
//    dumpRouteSteps(routeStepVec);
//    dumpRouteSections(routeSectionVec);
#endif

  addHyperspaceWireJumps(routeSectionVec);
  solution_.routeVec.push_back(routeSectionVec);

  return true;
}


bool Router::isAvailable(const ViaLayer &via, const Via &startVia, const Via &targetVia)
{
  if (via.via.x() < 0 || via.via.y() < 0 || via.via.x() >= solution_.gridW || via.via.y() >= solution_.gridH) {
    return false;
  }

  if (via.isWireLayer) {
    if (isBlocked(via.via)) {
      return false;
    }
  }
  else {
    // If it has an equivalent, it must be our equivalent
    if (nets_.hasEquivalent(via.via) && !nets_.isEquivalentVia(via.via, startVia)) {
      return false;
    }
//    // Allow moving into route from starting pin before the starting pin has been set as equivalent to the new target pin.
//    else if (nets_.isEquivalentVia(via.via, startVia)) {
//      return true;
//    }
    // Can go to component pin only if it's target.
    if (isAnyPin(via) && !isTarget(via, targetVia)) {
      return false;
    }
  }
  // Can go there!
  return true;
}

bool Router::isTarget(const ViaLayer &via, const Via &targetVia)
{
  if (via.isWireLayer) {
    return false;
  }
  else if (isTargetPin(via, targetVia)) {
    return true;
  }
  else if (nets_.isEquivalentVia(via.via, targetVia)) {
    return true;
  }
  return false;
}

bool Router::isTargetPin(const ViaLayer &via, const Via &targetVia)
{
  return (via.via == targetVia).all();
}

bool Router::isAnyPin(const ViaLayer &via)
{
  return allPinSet.count(via.via) > 0;
}

// Route always starts and ends on wire layer
// Through to wire always starts a wire section.
// Through to strip always ends a wire section.
// Everything else is a strip section.
RouteSectionVec Router::condenseRoute(const RouteStepVec &routeStepVec)
{
  RouteSectionVec routeSectionVec;
  assert(!routeStepVec.begin()->isWireLayer);
  assert(!(routeStepVec.rend() - 1)->isWireLayer);
  auto startSection = routeStepVec.begin();
  for (auto i = routeStepVec.begin() + 1; i != routeStepVec.end(); ++i) {
    if (i->isWireLayer != (i - 1)->isWireLayer) {
      if ((i - 1) != startSection) {
        routeSectionVec.push_back(ViaLayerStartEnd(*startSection, *(i - 1)));
        startSection = i;
      }
    }
  }
  if (startSection != routeStepVec.end() - 1) {
    routeSectionVec.push_back(ViaLayerStartEnd(*startSection, *(routeStepVec.end() - 1)));
  }
  return routeSectionVec;
}

void Router::dumpRouteSteps(const RouteStepVec &routeStepVec)
{
  fmt::print("RouteStepVec\n");
  for (const auto &step : routeStepVec) {
    fmt::print("  {}\n", step.str());
  }
}

void Router::dumpRouteSections(const RouteSectionVec &routeSectionVec)
{
  fmt::print("RouteSectionVec\n");
  for (const auto &section : routeSectionVec) {
    fmt::print("  {} - {}\n", section.start.str(), section.end.str());
  }
}

void Router::addHyperspaceWireJumps(const RouteSectionVec &routeSectionVec)
{
  for (auto section : routeSectionVec) {
    const auto &start = section.start;
    const auto &end = section.end;
    assert(start.isWireLayer == end.isWireLayer);
    if (start.isWireLayer) {
      wireToViaRef(start.via) = ViaValid(end.via, true);
      wireToViaRef(end.via) = ViaValid(start.via, true);
    }
  }
}

ViaValid &Router::wireToViaRef(const Via &via)
{
  return viaTraceVec_[solution_.idx(via)].wireToVia;
}

bool Router::isRouterStopRequested(std::mutex &stopThreadMutex)
{
  bool lockObtained = stopThreadMutex.try_lock();
  if (lockObtained) {
    stopThreadMutex.unlock();
  }
  return !lockObtained;
}
