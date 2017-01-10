#pragma once

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <tuple>

#include <Eigen/Core>

#include "circuit.h"
#include "ga_interface.h"
#include "layout.h"
#include "nets.h"
#include "settings.h"
#include "thread_stop.h"
#include "via.h"


typedef std::chrono::duration<double> TimeDuration;


class Router
{
public:
  Router(Layout&, ConnectionIdxVec&, ThreadStop&, Layout& inputLayout,
         Layout& currentLayout, const TimeDuration& _maxRenderDelay);
  bool route();
  // Interface for Uniform Cost Search
  bool
  isAvailable(const LayerVia& via, const Via& startVia, const Via& targetVia);
  bool isTarget(const LayerVia& via, const Via& targetVia);
  bool isTargetPin(const LayerVia& via, const Via& targetVia);
  bool isAnyPin(const Via& via);
  ValidVia& wireToViaRef(const Via& via);

private:
  bool routeAll();
  bool findCompleteRoute(const StartEndVia&);
  bool findRoute(Via& shortcutEndVia, const StartEndVia& viaStartEnd);
  RouteSectionVec condenseRoute(const RouteStepVec& routeStepVec);
  StripCutVec findStripCuts();

  // Wire layer blocking
  void blockComponentFootprints();
  void blockRoute(const RouteStepVec& routeStepVec);
  void block(const Via& via);
  bool isBlocked(const Via& via);
  // Nets
  void joinAllConnections();
  void registerActiveComponentPins();
  void addWireJumps(const RouteSectionVec& routeSectionVec);

  Layout& layout_;
  ConnectionIdxVec& connectionIdxVec_;
  Layout& inputLayout_;
  Layout& currentLayout_;

  Nets nets_;
  ThreadStop& threadStop_;

  WireLayerViaVec viaTraceVec_;
  ViaSet allPinSet_;

  const TimeDuration& maxRenderDelay_;
};
