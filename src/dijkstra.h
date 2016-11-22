#pragma once

#include <vector>

#include "circuit.h"
#include "int_types.h"
#include "solution.h"
#include "via.h"


const int DEFAULT_WIRE_COST = 5;
const int DEFAULT_STRIP_COST = 1;
const int DEFAULT_VIA_COST = 50;


class Costs {
public:
  Costs();
  u32 wire;
  u32 strip;
  u32 via;
};


class Dijkstra {
public:
  Dijkstra(u32 gridW, u32 gridH);
  void route(Solution&, Costs, Circuit, std::mutex&);
private:
  void routeAll(Solution&, Costs&, Circuit&, std::mutex&);
  void blockComponentFootprints(Circuit&);
  void findLowestCostRoute(Solution&, Costs&, Circuit&, ViaStartEnd&);
  void addRouteToUsed(RouteStepVec&);
  void setViaLayerUsed(const ViaLayer&);
  bool getViaLayerUsed(const ViaLayer&);
  void dump();
  void dumpLayer(bool wireLayer);
  bool findCosts(Costs& costs, ViaStartEnd&);
  u32 getCost(const ViaLayer&);
  void setCost(const ViaLayer&, u32 cost);
  void setCost(const ViaLayerCost&);
//  Via minCost();
  RouteStepVec backtraceLowestCostRoute(ViaStartEnd&);
  int idx(const Via&);
  int idxLeft(const Via&);
  int idxRight(const Via&);
  int idxUp(const Via&);
  int idxDown(const Via&);

  const u32 gridW_;
  const u32 gridH_;


  ViaTraceVec viaTraceVec_;
  ViaCostVec viaCostVec_;

//  GridVec wireVec;
//  GridVec stripVec;
//  CostVec wireCostVec;
//  CostVec stripCostVec;
//
//  GridVec usedWireVec;
//  GridVec usedStripVec;

  u32 totalCost_;
  u32 numCompletedRoutes;
  u32 numFailedRoutes;
};
