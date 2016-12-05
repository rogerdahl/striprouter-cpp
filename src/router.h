#pragma once

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <tuple>

#include <Eigen/Core>

#include "circuit.h"
#include "nets.h"
#include "settings.h"
#include "solution.h"
#include "via.h"


class Router
{
public:
  Router(Solution &);
  void route(std::mutex &);

  bool isAvailable(const ViaLayer &via, const Via &startVia, const Via &targetVia);
  bool isTarget(const ViaLayer &via, const Via &targetVia);
  bool isTargetPin(const ViaLayer &via, const Via &targetVia);
  bool isAnyPin(const ViaLayer &via);
  ViaValid &wireToViaRef(const Via &via);

private:
  void routeAll(std::mutex &);

  void blockComponentFootprints();
  void blockRoute(const RouteStepVec &routeStepVec);
  void block(const Via &via);
  bool isBlocked(const Via &via);

  void findCompleteRoute(const ViaStartEnd &);
  bool findRoute(Via& shortcutEndVia, const ViaStartEnd &viaStartEnd);

  void registerAllComponentPins();

  RouteSectionVec condenseRoute(const RouteStepVec &routeStepVec);
  void addHyperspaceWireJumps(const RouteSectionVec &routeSectionVec);

  bool isRouterStopRequested(std::mutex &);

  void dumpRouteSteps(const RouteStepVec &routeStepVec);
  void dumpRouteSections(const RouteSectionVec &routeSectionVec);

  Solution &solution_;
  ViaTraceVec viaTraceVec_;
  Nets nets_;

  ViaSet allPinSet;
};
