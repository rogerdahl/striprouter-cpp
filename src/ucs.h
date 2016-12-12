#pragma once

#include <set>
#include <queue>

#include "layout.h"
#include "via.h"
#include "nets.h"


typedef std::priority_queue<LayerCostVia> FrontierPri;
typedef std::set<LayerVia> FrontierSet;
typedef std::set<LayerVia> ExploredSet;

class Router;

class UniformCostSearch
{
public:
  UniformCostSearch(Router &router,
                    Layout &layout,
                    Nets &nets,
                    Via &shortcutEndVia,
                    const StartEndVia &viaStartEnd);
  RouteStepVec findLowestCostRoute();
private:
  bool findCosts(Via &shortcutEndVia);
  void exploreNeighbour(LayerCostVia &node, LayerCostVia n);
  void exploreFrontier(LayerCostVia &node, LayerCostVia n);
  RouteStepVec backtraceLowestCostRoute(const StartEndVia &);

  int getCost(const LayerVia &);
  void setCost(const LayerVia &, int cost);
  void setCost(const LayerCostVia &);

  LayerVia stepLeft(const LayerVia &v);
  LayerVia stepRight(const LayerVia &);
  LayerVia stepUp(const LayerVia &);
  LayerVia stepDown(const LayerVia &);
  LayerVia stepToWire(const LayerVia &);
  LayerVia stepToStrip(const LayerVia &);

  Router &router_;
  Layout &layout_;
  Nets &nets_;
  Via &shortcutEndVia_;
  const StartEndVia &viaStartEnd_;

  CostViaVec viaCostVec_;
  FrontierPri frontierPri;
  FrontierSet frontierSet;
  ExploredSet exploredSet;
};
