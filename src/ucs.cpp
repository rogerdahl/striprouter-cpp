#include <fmt/format.h>

#include "ucs.h"
#include "router.h"


UniformCostSearch::UniformCostSearch(Router& router,
                                     Layout& layout,
                                     Nets& nets,
                                     Via& shortcutEndVia,
                                     const StartEndVia& viaStartEnd)
  : router_(router),
    layout_(layout),
    nets_(nets),
    shortcutEndVia_(shortcutEndVia),
    viaStartEnd_(viaStartEnd)
{
  viaCostVec_ = CostViaVec(layout_.gridW * layout_.gridH);
}

RouteStepVec UniformCostSearch::findLowestCostRoute()
{
  shortcutEndVia_ = viaStartEnd_.end;
  bool foundRoute = findCosts(shortcutEndVia_);
#ifndef NDEBUG
  layout_.diagCostVec = viaCostVec_;
#endif
  if (foundRoute) {
    return backtraceLowestCostRoute(StartEndVia(viaStartEnd_.start,
                                    shortcutEndVia_));
  }
  else {
    return RouteStepVec();
  }
}

// 'procedure' 'UniformCostSearch'(Graph, start, goal)
//   node ← start
//   cost ← 0
//   frontier ← priority queue containing node only
//   explored ← empty set
//   'do'
//     'if' frontier is empty
//       'return' failure
//     node ← frontier.pop()
//     'if' node is goal
//       'return' layout
//     explored.add(node)
//     'for each' of node's neighbors n
//       'if' n is not in explored
//         'if' n is not in frontier
//           frontier.add(n)
//         'else if' n is in frontier with higher cost
//           replace existing node with n

bool UniformCostSearch::findCosts(Via& shortcutEndVia)
{
  Settings& settings = layout_.settings;

  auto start = LayerVia(viaStartEnd_.start, false);
  auto end = LayerVia(viaStartEnd_.end, false);

  setCost(start, 0);

  frontierPri.push(LayerCostVia(start, 0));
  frontierSet.insert(LayerCostVia(start, 0));

  while (true) {
    if (!frontierPri.size()) {
//#ifndef NDEBUG
//      layout_.errorStringVec.push_back(fmt::format("Debug: UniformCostSearch::findCosts() No route found"));
//      layout_.diagStartVia = start.via;
//      layout_.diagEndVia = end.via;
//      layout_.hasError = true;
//#endif
      return false;
    }

    LayerCostVia node = frontierPri.top();
    frontierPri.pop();
    frontierSet.erase(node);

    node.cost = getCost(node);

    if (router_.isTarget(node, end.via)) {

//      if (nets_.isConnected(node.via, end.via)) {
//        shortcutEndVia = node.via;
//      }

#ifndef NDEBUG
      layout_.diagCostVec = viaCostVec_;
#endif
      return true;
    }

    exploredSet.insert(node);

    // Only nodes that pass isAvailable() can become <node> here, from which
    // new exploration can take place.

    if (node.isWireLayer) {
      exploreNeighbour(node, LayerCostVia(stepLeft(node), settings.wire_cost));
      exploreNeighbour(node, LayerCostVia(stepRight(node), settings.wire_cost));
      exploreNeighbour(node,
                       LayerCostVia(stepToStrip(node), settings.via_cost));
    }
    else {
      exploreNeighbour(node, LayerCostVia(stepUp(node), settings.strip_cost));
      exploreNeighbour(node, LayerCostVia(stepDown(node), settings.strip_cost));
      exploreNeighbour(node, LayerCostVia(stepToWire(node), settings.via_cost));

      // Wire jumps
      const auto& wireToVia = router_.wireToViaRef(node.via);
      if (wireToVia.isValid) {
        exploreFrontier(node,
                        LayerCostVia(LayerVia(wireToVia.via, false),
                                     settings.wire_cost));
      }
    }
  }
}

void UniformCostSearch::exploreNeighbour(LayerCostVia& node, LayerCostVia n)
{
  if (router_.isAvailable(n, viaStartEnd_.start, shortcutEndVia_)) {
    exploreFrontier(node, n);
  }
}

void UniformCostSearch::exploreFrontier(LayerCostVia& node, LayerCostVia n)
{
  if (exploredSet.count(n)) {
    return;
  }
  n.cost += node.cost;
  setCost(n);
  auto frontierN = frontierSet.find(n);
  if (frontierN == frontierSet.end()) {
    frontierPri.push(n);
    frontierSet.insert(n);
    setCost(n);
  }
  else {
    auto frontierCost = getCost(*frontierN);
    if (frontierCost > n.cost) {
      node.cost = n.cost;
      setCost(node);
    }
  }
}

RouteStepVec
UniformCostSearch::backtraceLowestCostRoute(const StartEndVia& viaStartEnd)
{
  int routeCost = 0;
  auto start = LayerVia(viaStartEnd.start, false);
  auto end = LayerVia(viaStartEnd.end, false);
  RouteStepVec routeStepVec;
  auto c = end;
  routeStepVec.push_back(c);

  // If there is an error in the uniform cost search, it typically causes the backtrace to get stuck. We check
  // for the condition and return diagnostics information.
  int checkStuckCnt = 0;

  while (!((c.via == start.via).all() && c.isWireLayer == start.isWireLayer)) {
    if (checkStuckCnt++ > layout_.gridW * layout_.gridH) {
      layout_.errorStringVec.push_back(fmt::format(
                                         "Error: backtraceLowestCostRoute() stuck at {}",
                                         c.str()));
      layout_.diagStartVia = start.via;
      layout_.diagEndVia = end.via;
      layout_.diagRouteStepVec = routeStepVec;
      layout_.hasError = true;
      break;
    }

    LayerVia n = c;
    if (c.isWireLayer) {
      auto nLeft = stepLeft(c);
      if (c.via.x() > 0 && getCost(nLeft) < getCost(n)) {
        n = nLeft;
      }
      auto nRight = stepRight(c);
      if (c.via.x() < layout_.gridW - 1 && getCost(nRight) < getCost(n)) {
        n = nRight;
      }
      auto nStrip = stepToStrip(c);
      if (getCost(nStrip) < getCost(n)) {
        n = nStrip;
      }
    }
    else {
      auto nUp = stepUp(c);
      if (c.via.y() > 0 && getCost(nUp) < getCost(n)) {
        n = nUp;
      }
      auto nDown = stepDown(c);
      if (c.via.y() < layout_.gridH - 1 && getCost(nDown) < getCost(n)) {
        n = nDown;
      }
      auto nWire = stepToWire(c);
      if (getCost(nWire) < getCost(n)) {
        n = nWire;
      }

      const auto& wireToVia = router_.wireToViaRef(c.via);
      if (wireToVia.isValid) {
        auto nWireJump = LayerVia(wireToVia.via, false);
        if (getCost(nWireJump) < getCost(n)) {
          // When we jump, we have to record the steps.
          // Through to wire layer.
          routeStepVec.push_back(LayerVia(c.via, true));
          int x1 = c.via.x();
          int x2 = nWireJump.via.x();
          int step = x1 > x2 ? -1 : 1;
          for (int x = x1; x != x2; x += step) {
            routeStepVec.push_back(LayerVia(Via(x, c.via.y()), true));
          }
          if (x1 != x2) {
            routeStepVec.push_back(LayerVia(Via(x2, c.via.y()), true));
          }
          // Final step through to strip layer is stored outside the
          // conditional.
          n = nWireJump;
        }
      }
    }
    routeCost += getCost(c) - getCost(n);
    c = n;
    routeStepVec.push_back(c);
  }
  layout_.cost += routeCost;
  std::reverse(routeStepVec.begin(), routeStepVec.end());

#ifndef NDEBUG
  layout_.diagRouteStepVec = routeStepVec;
  layout_.diagStartVia = ValidVia(start.via, true);
  layout_.diagEndVia = ValidVia(end.via, true);
#endif

  return routeStepVec;
}

int UniformCostSearch::getCost(const LayerVia& viaLayer)
{
  int cost;
  int i = layout_.idx(viaLayer.via);
  if (viaLayer.isWireLayer) {
    cost = viaCostVec_[i].wireCost;
  }
  else {
    cost = viaCostVec_[i].stripCost;
  }
  return cost;
}

void UniformCostSearch::setCost(const LayerVia& viaLayer, int cost)
{
  int i = layout_.idx(viaLayer.via);
  if (viaLayer.isWireLayer) {
    viaCostVec_[i].wireCost = cost;
  }
  else {
    viaCostVec_[i].stripCost = cost;
  }
}

void UniformCostSearch::setCost(const LayerCostVia& viaLayerCost)
{
  setCost(viaLayerCost, viaLayerCost.cost);
}

LayerVia UniformCostSearch::stepLeft(const LayerVia& v)
{
  return LayerVia(v.via + Via(-1, 0), v.isWireLayer);
}

LayerVia UniformCostSearch::stepRight(const LayerVia& v)
{
  return LayerVia(v.via + Via(+1, 0), v.isWireLayer);
}

LayerVia UniformCostSearch::stepUp(const LayerVia& v)
{
  return LayerVia(v.via + Via(0, -1), v.isWireLayer);
}

LayerVia UniformCostSearch::stepDown(const LayerVia& v)
{
  return LayerVia(v.via + Via(0, +1), v.isWireLayer);
}

LayerVia UniformCostSearch::stepToWire(const LayerVia& v)
{
  assert(!v.isWireLayer);
  return LayerVia(v.via, true);
}

LayerVia UniformCostSearch::stepToStrip(const LayerVia& v)
{
  assert(v.isWireLayer);
  return LayerVia(v.via, false);
}
