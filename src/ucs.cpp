#include <fmt/format.h>

#include "ucs.h"
#include "router.h"


UniformCostSearch::UniformCostSearch(Router &dijkstra,
                                     Solution &solution,
                                     Nets &nets,
                                     Via &shortcutEndVia,
                                     const ViaStartEnd &viaStartEnd)
  : dijkstra_(dijkstra), solution_(solution), nets_(nets), shortcutEndVia_(shortcutEndVia), viaStartEnd_(viaStartEnd)
{
  viaCostVec_ = ViaCostVec(solution_.gridW * solution_.gridH);
}

RouteStepVec UniformCostSearch::findLowestCostRoute()
{
  shortcutEndVia_ = viaStartEnd_.end;
  bool foundRoute = findCosts(shortcutEndVia_);
  if (foundRoute) {
    return backtraceLowestCostRoute(ViaStartEnd(viaStartEnd_.start, shortcutEndVia_));
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
//       'return' solution
//     explored.add(node)
//     'for each' of node's neighbors n
//       'if' n is not in explored
//         'if' n is not in frontier
//           frontier.add(n)
//         'else if' n is in frontier with higher cost
//           replace existing node with n

bool UniformCostSearch::findCosts(Via &shortcutEndVia)
{
  Settings &settings = solution_.settings;

  auto start = ViaLayer(viaStartEnd_.start, false);
  auto end = ViaLayer(viaStartEnd_.end, false);

//  setCost(ViaLayer(start.via, true), 0);///////////////////////
  setCost(start, 0);

  frontierPri.push(ViaLayerCost(start, 0));
  frontierSet.insert(ViaLayerCost(start, 0));

  while (true) {
    if (!frontierPri.size()) {
#ifndef NDEBUG
      solution_.diagCostVec = viaCostVec_;
#endif
      return false;
    }

    ViaLayerCost node = frontierPri.top();
    frontierPri.pop();
    frontierSet.erase(node);

    node.cost = getCost(node);

    if (dijkstra_.isTarget(node, end.via)) {

//      if (nets_.isEquivalentVia(node.via, end.via)) {
//        shortcutEndVia = node.via;
//      }

#ifndef NDEBUG
      solution_.diagCostVec = viaCostVec_;
#endif
      return true;
    }

    exploredSet.insert(node);

    // Only nodes that pass isAvailable() can become <node> here, from which
    // new exploration can take place.

    if (node.isWireLayer) {
      exploreNeighbour(node, ViaLayerCost(stepLeft(node), settings.wire_cost));
      exploreNeighbour(node, ViaLayerCost(stepRight(node), settings.wire_cost));
      exploreNeighbour(node, ViaLayerCost(stepToStrip(node), settings.via_cost));
    }
    else {
      exploreNeighbour(node, ViaLayerCost(stepUp(node), settings.strip_cost));
      exploreNeighbour(node, ViaLayerCost(stepDown(node), settings.strip_cost));
      exploreNeighbour(node, ViaLayerCost(stepToWire(node), settings.via_cost));

      // Wire jumps
      const auto &wireToVia = dijkstra_.wireToViaRef(node.via);
      if (wireToVia.isValid) {
        exploreFrontier(node, ViaLayerCost(ViaLayer(wireToVia.via, false), settings.wire_cost));
      }
    }

//    for (auto n : neighborVec) {
//      n.cost += node.cost;
//      setCost(n);
//      auto frontierN = frontierSet.find(n);
//      if (frontierN == frontierSet.end()) {
//        frontierPri.push(n);
//        frontierSet.insert(n);
//        setCost(n);
//      }
//      else {
//        auto frontierCost = getCost(*frontierN);
//        if (frontierCost > n.cost) {
//          node.cost = n.cost;
//          setCost(node);
//        }
//      }
  }
}

void UniformCostSearch::exploreNeighbour(ViaLayerCost &node, ViaLayerCost n)
{
  if (dijkstra_.isAvailable(n, viaStartEnd_.start, shortcutEndVia_)) {
    exploreFrontier(node, n);
  }
}

void UniformCostSearch::exploreFrontier(ViaLayerCost &node, ViaLayerCost n)
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

RouteStepVec UniformCostSearch::backtraceLowestCostRoute(const ViaStartEnd &viaStartEnd)
{
  int routeCost = 0;
  auto start = ViaLayer(viaStartEnd.start, false);
  auto end = ViaLayer(viaStartEnd.end, false);
  RouteStepVec routeStepVec;
  auto c = end;
  routeStepVec.push_back(c);

  // If there is an error in the uniform cost search, it typically causes the backtrace to get stuck. We check
  // for the condition and return diagnostics information.
  int checkStuckCnt = 0;

  while (!((c.via == start.via).all() && c.isWireLayer == start.isWireLayer)) {
    if (checkStuckCnt++ > solution_.gridW * solution_.gridH) {
      solution_.errorStringVec.push_back(fmt::format("Error: backtraceLowestCostRoute() stuck at {}", c.str()));
      solution_.diagStartVia = start.via;
      solution_.diagEndVia = end.via;
      solution_.diagRouteStepVec = routeStepVec;
      solution_.hasError = true;
      break;
    }

    ViaLayer n = c;
    if (c.isWireLayer) {
      auto nLeft = stepLeft(c);
      if (c.via.x() > 0 && getCost(nLeft) < getCost(n)) {
        n = nLeft;
      }
      auto nRight = stepRight(c);
      if (c.via.x() < solution_.gridW - 1 && getCost(nRight) < getCost(n)) {
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
      if (c.via.y() < solution_.gridH - 1 && getCost(nDown) < getCost(n)) {
        n = nDown;
      }
      auto nWire = stepToWire(c);
      if (getCost(nWire) < getCost(n)) {
        n = nWire;
      }

      const auto &wireToVia = dijkstra_.wireToViaRef(c.via);
      if (wireToVia.isValid) {
        auto nWireJump = ViaLayer(wireToVia.via, false);
        if (getCost(nWireJump) < getCost(n)) {
          // When we jump, we have to record the steps.
          // Through to wire layer.
          routeStepVec.push_back(ViaLayer(c.via, true));
          int x1 = c.via.x();
          int x2 = nWireJump.via.x();
          int step = x1 > x2 ? -1 : 1;
          for (int x = x1; x != x2; x += step) {
            routeStepVec.push_back(ViaLayer(Via(x, c.via.y()), true));
          }
          if (x1 != x2) {
            routeStepVec.push_back(ViaLayer(Via(x2, c.via.y()), true));
          }
          // Through to strip layer
//          routeStepVec.push_back(ViaLayer(nWireJump.via, true));
          n = nWireJump;
        }
      }
    }
    c = n;
    routeCost += getCost(c);
    routeStepVec.push_back(c);
  }
  solution_.totalCost += routeCost;
  std::reverse(routeStepVec.begin(), routeStepVec.end());

#ifndef NDEBUG
  solution_.diagRouteStepVec = routeStepVec;
  solution_.diagStartVia = ViaValid(start.via, true);
  solution_.diagEndVia = ViaValid(end.via, true);
#endif

  return routeStepVec;
}

int UniformCostSearch::getCost(const ViaLayer &viaLayer)
{
  int cost;
  int i = solution_.idx(viaLayer.via);
  if (viaLayer.isWireLayer) {
    cost = viaCostVec_[i].wireCost;
  }
  else {
    cost = viaCostVec_[i].stripCost;
  }
  return cost;
}

void UniformCostSearch::setCost(const ViaLayer &viaLayer, int cost)
{
  int i = solution_.idx(viaLayer.via);
  if (viaLayer.isWireLayer) {
    viaCostVec_[i].wireCost = cost;
  }
  else {
    viaCostVec_[i].stripCost = cost;
  }
}

void UniformCostSearch::setCost(const ViaLayerCost &viaLayerCost)
{
  setCost(viaLayerCost, viaLayerCost.cost);
}

ViaLayer UniformCostSearch::stepLeft(const ViaLayer &v)
{
  return ViaLayer(v.via + Via(-1, 0), v.isWireLayer);
}

ViaLayer UniformCostSearch::stepRight(const ViaLayer &v)
{
  return ViaLayer(v.via + Via(+1, 0), v.isWireLayer);
}

ViaLayer UniformCostSearch::stepUp(const ViaLayer &v)
{
  return ViaLayer(v.via + Via(0, -1), v.isWireLayer);
}

ViaLayer UniformCostSearch::stepDown(const ViaLayer &v)
{
  return ViaLayer(v.via + Via(0, +1), v.isWireLayer);
}

ViaLayer UniformCostSearch::stepToWire(const ViaLayer &v)
{
  assert(!v.isWireLayer);
  return ViaLayer(v.via, true);
}

ViaLayer UniformCostSearch::stepToStrip(const ViaLayer &v)
{
  assert(v.isWireLayer);
  return ViaLayer(v.via, false);
}

void UniformCostSearch::dump()
{
  fmt::print("Wire layer\n");
  dumpLayer(true);
  fmt::print("Strip layer\n");
  dumpLayer(false);
}

void UniformCostSearch::dumpLayer(bool wireLayer)
{
  for (int y = 0; y < solution_.gridH; ++y) {
    for (int x = 0; x < solution_.gridW; ++x) {
      int v = getCost(ViaLayer(Via(x, y), wireLayer));
      if (v == INT_MAX) {
        fmt::print("   -", v);
      }
      else {
        fmt::print(" {:3d}", v);
      }
    }
    fmt::print("\n");
  }
  fmt::print("\n");
}


