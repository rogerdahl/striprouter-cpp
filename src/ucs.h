#pragma once

#include <set>
#include <queue>

#include "solution.h"
#include "via.h"
#include "nets.h"


typedef std::priority_queue<ViaLayerCost> FrontierPri;

typedef std::set<ViaLayer> FrontierSet;

typedef std::set<ViaLayer> ExploredSet;

class Router;

class UniformCostSearch
{
public:
  UniformCostSearch(Router &dijkstra,
                    Solution &solution,
                    Nets &nets,
                    Via &shortcutEndVia,
                    const ViaStartEnd &viaStartEnd);
  RouteStepVec findLowestCostRoute();
private:
  bool findCosts(Via &shortcutEndVia);
  void exploreNeighbour(ViaLayerCost &node, ViaLayerCost n);
  void exploreFrontier(ViaLayerCost &node, ViaLayerCost n);
  RouteStepVec backtraceLowestCostRoute(const ViaStartEnd &);

  int getCost(const ViaLayer &);
  void setCost(const ViaLayer &, int cost);
  void setCost(const ViaLayerCost &);

  ViaLayer stepLeft(const ViaLayer &v);
  ViaLayer stepRight(const ViaLayer &);
  ViaLayer stepUp(const ViaLayer &);
  ViaLayer stepDown(const ViaLayer &);
  ViaLayer stepToWire(const ViaLayer &);
  ViaLayer stepToStrip(const ViaLayer &);

  void dump();
  void dumpLayer(bool wireLayer);

  Router &dijkstra_;
  Solution &solution_;
  Nets &nets_;
  Via &shortcutEndVia_;
  const ViaStartEnd &viaStartEnd_;

  ViaCostVec viaCostVec_;
  FrontierPri frontierPri;
  FrontierSet frontierSet;
  ExploredSet exploredSet;
};
