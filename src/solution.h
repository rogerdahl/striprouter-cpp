#pragma once

#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "circuit.h"
#include "settings.h"
#include "via.h"


typedef std::vector<ViaLayer> RouteStepVec;

typedef std::vector<ViaLayerStartEnd> RouteSectionVec;

typedef std::vector<RouteSectionVec> RouteVec;

typedef std::vector<std::string> StringVec;

typedef std::vector<bool> RouteStatusVec;

// Nets
typedef std::set<Via, std::function<bool(const Via &, const Via &)> > ViaSet;

//typedef std::map<Via, int, std::function<bool(const Via&,const Via&)> > ViaToIdxMap;
typedef std::vector<ViaSet> ViaSetVec;

typedef std::vector<int> SetIdxVec;

class Solution
{
public:
  Solution(int gridW, int gridH);
  ~Solution();
  Solution(const Solution &);
  Solution &operator=(const Solution &);
  std::unique_lock<std::mutex> scope_lock();

  void dump() const;

  Circuit circuit;
  Settings settings;

  int gridW;
  int gridH;

  int totalCost;
  int numCompletedRoutes;
  int numFailedRoutes;
  int numShortcuts;
  bool isReady;

  StringVec solutionInfoVec;
  RouteVec routeVec;
  RouteStatusVec routeStatusVec;

  int idx(const Via &);

  // Nets
  ViaSetVec viaSetVec;
  SetIdxVec setIdxVec;

  // Debug
  bool hasError;
  ViaValid diagStartVia;
  ViaValid diagEndVia;
  ViaCostVec diagCostVec;
  RouteStepVec diagRouteStepVec;
  ViaTraceVec diagTraceVec;
  StringVec errorStringVec;

private:
  void copy(const Solution &s);
  std::mutex mutex_;
};
