#pragma once

#include <vector>

#include <Eigen/Core>

#include "circuit.h"
#include "int_types.h"
#include "solution.h"
#include "via.h"
#include "settings.h"


class Dijkstra {
public:
  Dijkstra(u32 gridW, u32 gridH);
  void route(Solution&, Settings&, Circuit&, std::mutex&);
private:
  void routeAll(Solution&, Settings&, Circuit&, std::mutex&);
  void blockComponentFootprints(Circuit&);
  void findLowestCostRoute(Solution&, Settings&, Circuit&, ViaLayerStartEnd&);
  void addRouteToUsed(RouteStepVec&);
  void setViaLayerUsed(const ViaLayer&);
  bool getViaLayerUsed(const ViaLayer&);
  void dump();
  void dumpLayer(bool wireLayer);
  bool findCosts(Settings&, ViaLayerStartEnd&);
  u32 getCost(const ViaLayer&);
  void setCost(const ViaLayer&, u32 cost);
  void setCost(const ViaLayerCost&);
  RouteStepVec backtraceLowestCostRoute(u32& routeCost, ViaLayerStartEnd&);
  bool isStopRequested(std::mutex&);

  int idx(const Via&);

  ViaLayer stepLeft(const ViaLayer& v);
  ViaLayer stepRight(const ViaLayer&);
  ViaLayer stepUp(const ViaLayer&);
  ViaLayer stepDown(const ViaLayer&);
  ViaLayer stepToWire(const ViaLayer&);
  ViaLayer stepToStrip(const ViaLayer&);

  const u32 gridW_;
  const u32 gridH_;

  ViaTraceVec viaTraceVec_;
  ViaCostVec viaCostVec_;
};
