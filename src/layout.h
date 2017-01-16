#pragma once

#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "circuit.h"
#include "settings.h"
#include "via.h"

typedef std::vector<LayerVia> RouteStepVec;
typedef std::vector<LayerStartEndVia> RouteSectionVec;
typedef std::vector<RouteSectionVec> RouteVec;
typedef std::vector<std::string> StringVec;
typedef std::vector<bool> RouteStatusVec;
typedef std::vector<Via> StripCutVec;

// Nets
typedef std::set<Via, std::function<bool(const Via&, const Via&)> > ViaSet;
typedef std::vector<ViaSet> ViaSetVec;
typedef std::vector<int> SetIdxVec;

typedef std::chrono::time_point<std::chrono::high_resolution_clock> Timestamp;

class Layout
{
  public:
  Layout();
  Layout(const Layout&);
  Layout& operator=(const Layout&);
  // Lineage
  void updateBaseTimestamp();
  bool isBasedOn(const Layout& other);
  Timestamp& getBaseTimestamp();
  // Locking
  std::unique_lock<std::mutex> scopeLock();
  Layout threadSafeCopy();
  bool isLocked();
  //
  int idx(const Via&);

  Circuit circuit;
  Settings settings;

  int gridW;
  int gridH;

  long cost;
  int nCompletedRoutes;
  int nFailedRoutes;
  int numShortcuts;

  bool isReadyForRouting;
  bool isReadyForEval;
  bool hasError;

  StringVec layoutInfoVec;
  RouteVec routeVec;
  StripCutVec stripCutVec;
  RouteStatusVec routeStatusVec;

  // Nets
  ViaSetVec viaSetVec;
  SetIdxVec setIdxVec;

  // Debug
  ValidVia diagStartVia;
  ValidVia diagEndVia;
  CostViaVec diagCostVec;
  RouteStepVec diagRouteStepVec;
  WireLayerViaVec diagTraceVec;
  StringVec errorStringVec;

  private:
  void copy(const Layout& s);
  std::mutex mutex_;
  Timestamp timestamp_;
};
