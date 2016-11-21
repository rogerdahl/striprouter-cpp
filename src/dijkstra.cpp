#include <algorithm>
#include <limits.h>
#include <mutex>
#include <queue>
#include <set>

#include <fmt/format.h>

#include "dijkstra.h"

using namespace std;


Costs::Costs()
  : wire(DEFAULT_WIRE_COST), strip(DEFAULT_STRIP_COST), via(DEFAULT_VIA_COST)
{}


Dijkstra::Dijkstra(u32 gridW, u32 gridH)
: gridW_(gridW), gridH_(gridH), totalCost_(0), numCompletedRoutes(0), numFailedRoutes(0)
{
  viaTraceVec_ = ViaTraceVec(gridW_ * gridH_);
}

void Dijkstra::route(Solution& solution, Costs &costs, Circuit &circuit, std::mutex &stopThreadMutex)
{
  solution = Solution();
  blockComponentFootprints(circuit);
  routeAll(solution, costs, circuit, stopThreadMutex);
}

void Dijkstra::blockComponentFootprints(Circuit &circuit)
{
  for (auto componentName : circuit.getComponentNameVec()) {
    auto ci = circuit.getComponentInfoMap().find(componentName)->second;
    for (u32 y = ci.footprint.start.y; y <= ci.footprint.end.y; ++y) {
      for (u32 x = ci.footprint.start.x; x <= ci.footprint.end.x; ++x) {
        setViaLayerUsed(ViaLayer(x, y, true));
        setViaLayerUsed(ViaLayer(x, y, false));
      }
    }
  }
}

void Dijkstra::routeAll(Solution& solution, Costs& costs, Circuit& circuit, std::mutex &stopThreadMutex)
{
  for (auto viaStartEnd : circuit.getConnectionCoordVec()) {
    findLowestCostRoute(solution, costs, circuit, viaStartEnd);
    if (!stopThreadMutex.try_lock()) {
      return;
    }
    else {
      stopThreadMutex.unlock();
    }
  }
}

void Dijkstra::findLowestCostRoute(Solution& solution, Costs &costs, Circuit &circuit,
                                    ViaStartEnd& viaStartEnd)
{
  viaCostVec_ = ViaCostVec(gridW_ * gridH_);

  bool success_bool = findCosts(costs, viaStartEnd);
  if (success_bool) {
    ++numCompletedRoutes;
    auto routeStepVec = backtraceLowestCostRoute(viaStartEnd);
    addRouteToUsed(routeStepVec);
    reverse(routeStepVec.begin(), routeStepVec.end());
    {
      lock_guard<mutex> lockSolution(solutionMutex);
      solution.getRouteVec().push_back(routeStepVec);
    }
  }
  else {
    ++numFailedRoutes;
  }
  circuit.getCircuitInfoVec().clear();
  circuit.getCircuitInfoVec().push_back(fmt::format("Completed: {}", numCompletedRoutes));
  circuit.getCircuitInfoVec().push_back(fmt::format("Failed: {}", numFailedRoutes));
  circuit.getCircuitInfoVec().push_back(fmt::format("Total cost: {}", totalCost_));
}


void Dijkstra::addRouteToUsed(RouteStepVec& routeStepVec) {
  for (auto c : routeStepVec) {
    setViaLayerUsed(c);
  }
}


void Dijkstra::setViaLayerUsed(const ViaLayer& viaLayer) {
  if (viaLayer.isWireLayer) {
    viaTraceVec_[idx(viaLayer)].setWireSideUsed();
  }
  else {
    viaTraceVec_[idx(viaLayer)].setStripSideUsed();
  }
}


bool Dijkstra::getViaLayerUsed(const ViaLayer& viaLayer) {
  if (viaLayer.isWireLayer) {
    return viaTraceVec_[idx(viaLayer)].wireSideIsUsed();
  }
  else {
    return viaTraceVec_[idx(viaLayer)].stripSideIsUsed();
  }
}

//
// Private
//

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

bool Dijkstra::findCosts(Costs& costs, ViaStartEnd& viaStartEnd)
{
  auto startC = viaStartEnd.start;
  auto endC = viaStartEnd.end;
  assert(!startC.isWireLayer);
  assert(!endC.isWireLayer);
  Via start(startC.x, startC.y);
  Via end(endC.x, endC.y);
  ViaLayerCost node(start.x, start.y, false, 0);
  priority_queue<ViaLayerCost> frontierPri;
  set<ViaLayer> frontierSet;
  set<ViaLayer> exploredSet;

  setCost(node);
  frontierPri.push(node);
  frontierSet.insert(node.viaLayer);

  bool success = false;

  while (true) {
    if (!frontierPri.size()) {
      return success;
    }
    node = frontierPri.top();
    frontierPri.pop();
    frontierSet.erase(node.viaLayer);
    if (node.viaLayer.x == end.x && node.viaLayer.y == end.y && !node.viaLayer.isWireLayer) {
      success = true;
    }
    exploredSet.insert(node.viaLayer);
    vector<ViaLayerCost> neighborVec;
    if (node.viaLayer.isWireLayer) {
      if (node.viaLayer.x > 0) {
        neighborVec.push_back(
          ViaLayerCost(node.viaLayer.x - 1, node.viaLayer.y, true,
                       node.cost + costs.wire));
      }
      if (node.viaLayer.x < gridW_ - 1) {
        neighborVec.push_back(
          ViaLayerCost(node.viaLayer.x + 1, node.viaLayer.y, true,
                       node.cost + costs.wire));
      }
      neighborVec.push_back(
        ViaLayerCost(node.viaLayer.x, node.viaLayer.y, false,
                     node.cost + costs.via));
    }
    else {
      if (node.viaLayer.y > 0) {
        neighborVec.push_back(
          ViaLayerCost(node.viaLayer.x, node.viaLayer.y - 1, false,
                       node.cost + costs.strip));
      }
      if (node.viaLayer.y < gridH_ - 1) {
        neighborVec.push_back(
          ViaLayerCost(node.viaLayer.x, node.viaLayer.y + 1, false,
                       node.cost + costs.strip));
      }
      neighborVec.push_back(
        ViaLayerCost(node.viaLayer.x, node.viaLayer.y, true,
                     node.cost + costs.via));
    }

    for (auto n : neighborVec) {
      if (getViaLayerUsed(n.viaLayer)) {
        continue;
      }
      if (!exploredSet.count(n.viaLayer)) {
        auto frontier_n = frontierSet.find(n.viaLayer);
        if (frontier_n == frontierSet.end()) {
//        if (!frontierSet.count(n.viaLayer)) {
          frontierPri.push(n);
          frontierSet.insert(n.viaLayer);
          setCost(n);
        }
        else {
          auto frontier_cost = getCost(*frontier_n);
          if (frontier_cost > n.cost) {
            setCost(node.viaLayer, n.cost);
          }
        }
      }
    }
  }
}

u32 Dijkstra::getCost(const ViaLayer& viaLayer)
{
  u32 idx = viaLayer.x + gridW_ * viaLayer.y;
  if (viaLayer.isWireLayer) {
    return viaCostVec_[idx].wireCost;
  }
  else {
    return viaCostVec_[idx].stripCost;
  }
}

void Dijkstra::setCost(const ViaLayer& viaLayer, u32 cost)
{
  u32 idx = viaLayer.x + gridW_ * viaLayer.y;
  if (viaLayer.isWireLayer) {
    viaCostVec_[idx].wireCost = cost;
  }
  else {
    viaCostVec_[idx].stripCost = cost;
  }
}

void Dijkstra::setCost(const ViaLayerCost& viaLayerCost)
{
  setCost(viaLayerCost.viaLayer, viaLayerCost.cost);
}

//ViaCoord Dijkstra::minCost()
//{
//  u32 min = INT_MAX;
//  ViaCoord minCoord;
//  minCoord.isValid = false;
//  for (u32 y = 0; y < H; ++y) {
//    for (u32 x = 0; x < W; ++x) {
//      // wire side
//      ViaCoord wireCoord(x, y, true);
//      if (viaCostVec_[idx(wireCoord)].wireCost <= min) {
//        min = viaCostVec_[idx(wireCoord)].wireCost;
//        minCoord = wireCoord;
//        minCoord.isValid = true;
//      }
//      // strip side
//      ViaCoord stripCoord(x, y, false);
//      if (viaCostVec_[idx(stripCoord)].stripCost <= min) {
//        min = viaCostVec_[idx(stripCoord)].stripCost;
//        minCoord = stripCoord;
//        minCoord.isValid = true;
//      }
//    }
//  }
//  return minCoord;
//}


RouteStepVec Dijkstra::backtraceLowestCostRoute(ViaStartEnd& viaStartEnd)
{
  auto start = viaStartEnd.start;
  auto end = viaStartEnd.end;

  RouteStepVec routeStepVec;
  auto c = end;
  routeStepVec.push_back(c);
  while (c.x != start.x || c.y != start.y || c.isWireLayer != start.isWireLayer) {
    ViaLayer n = c;
    if (c.isWireLayer) {
      if (c.x > 0) {
        n = ViaLayer(c.x - 1, c.y, c.isWireLayer);
      }
      if (c.x < gridW_ - 1 && viaCostVec_[idxRight(c)].wireCost <= viaCostVec_[idx(n)].wireCost) {
        n = ViaLayer(c.x + 1, c.y, c.isWireLayer);
      }
      if (viaCostVec_[idx(c)].stripCost <= viaCostVec_[idx(n)].wireCost) {
        n = ViaLayer(c.x, c.y, false);
      }
    }
    else {
      if (c.y > 0) {
        n = ViaLayer(c.x, c.y - 1, c.isWireLayer);
      }
      if (c.y < gridH_ - 1 && viaCostVec_[idxDown(c)].stripCost < viaCostVec_[idx(n)].stripCost) {
        n = ViaLayer(c.x, c.y + 1, c.isWireLayer);
      }
      if (viaCostVec_[idx(c)].wireCost < viaCostVec_[idx(n)].stripCost) {
        n = ViaLayer(c.x, c.y, true);
      }
    }
    c = n;

    if (c.isWireLayer) {
      totalCost_ += viaCostVec_[idx(c)].wireCost;
    }
    else {
      totalCost_ += viaCostVec_[idx(c)].stripCost;
    }

    routeStepVec.push_back(c);
  }
  return routeStepVec;
}

void Dijkstra::dump()
{
  //fmt::print("Wire layer\n");
  dumpLayer(true);
  //fmt::print("Strip layer\n");
  dumpLayer(false);
}


void Dijkstra::dumpLayer(bool wireLayer)
{
  for (u32 y = 0; y < gridH_; ++y) {
    for (u32 x = 0; x < gridW_; ++x) {
      u32 v;
      if (wireLayer) {
        v = viaCostVec_[x + y * gridW_].wireCost;
      }
      else {
        v = viaCostVec_[x + y * gridW_].stripCost;
      }
      if (v == INT_MAX) {
        //fmt::print("   -", v);
      }
      else {
        //fmt::print(" {:3d}", v);
      }
    }
    //fmt::print("\n");
  }
  //fmt::print("\n");
}


int Dijkstra::idx(const Via& v)
{
  return v.x + gridW_ * v.y;
}

int Dijkstra::idxLeft(const Via& v)
{
  return idx(v) - 1;
}

int Dijkstra::idxRight(const Via& v)
{
  return idx(v) + 1;
}

int Dijkstra::idxUp(const Via& v)
{
  return idx(v) - gridW_;
}

int Dijkstra::idxDown(const Via& v)
{
  return idx(v) + gridW_;
}
