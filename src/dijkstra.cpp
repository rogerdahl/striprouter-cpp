#include <algorithm>
#include <limits.h>
#include <mutex>
#include <thread>
#include <queue>
#include <set>

#include <fmt/format.h>

#include "dijkstra.h"


using namespace std;
using namespace Eigen;


Dijkstra::Dijkstra(u32 gridW, u32 gridH)
: gridW_(gridW), gridH_(gridH)
{
  viaTraceVec_ = ViaTraceVec(gridW_ * gridH_);
}

void Dijkstra::route(Solution& solution, Settings& settings, Circuit& circuit, std::mutex &stopThreadMutex)
{
  solution = Solution();
  blockComponentFootprints(circuit);
  routeAll(solution, settings, circuit, stopThreadMutex);
}

void Dijkstra::blockComponentFootprints(Circuit &circuit)
{
  for (auto& ci : circuit.componentNameToInfoMap) {
    const auto& componentName = ci.first;
    auto footprint = circuit.calcComponentFootprint(componentName);
    for (s32 y = footprint.start.y(); y <= footprint.end.y(); ++y) {
      for (s32 x = footprint.start.x(); x <= footprint.end.x(); ++x) {
        Via pinAbsPos(x, y);
        setViaLayerUsed(ViaLayer(pinAbsPos, true));
        setViaLayerUsed(ViaLayer(pinAbsPos, false));
      }
    }
  }
}

void Dijkstra::routeAll(Solution& solution, Settings& settings, Circuit& circuit, std::mutex &stopThreadMutex)
{
  auto v = circuit.genConnectionViaVec();
  random_shuffle(v.begin(), v.end());
  for (auto viaStartEnd : v) {
    auto start = ViaLayer(viaStartEnd.start, false);
    auto end = ViaLayer(viaStartEnd.end, false);
    ViaLayerStartEnd vl(start, end);
    findLowestCostRoute(solution, settings, circuit, vl);
    if (isStopRequested(stopThreadMutex)) {
      break;
    }
  }
  {
    solution.ready = true;
  }
}

void Dijkstra::findLowestCostRoute(Solution& solution, Settings& settings, Circuit &circuit,
                                    ViaLayerStartEnd& viaStartEnd)
{
  viaCostVec_ = ViaCostVec(gridW_ * gridH_);

  bool success_bool = findCosts(settings, viaStartEnd);
//  dump();
  {
    if (success_bool) {
      u32 routeCost;
      auto routeStepVec = backtraceLowestCostRoute(routeCost, viaStartEnd);
      addRouteToUsed(routeStepVec);
      reverse(routeStepVec.begin(), routeStepVec.end());
      {
        solution.getRouteVec().push_back(routeStepVec);
        ++solution.numCompletedRoutes;
        solution.totalCost += routeCost;
      }
    }
    else {
      ++solution.numFailedRoutes;
    }
  }
  circuit.circuitInfoVec.clear();
}


void Dijkstra::addRouteToUsed(RouteStepVec& routeStepVec) {
  for (auto c : routeStepVec) {
    setViaLayerUsed(c);
  }
}


void Dijkstra::setViaLayerUsed(const ViaLayer& viaLayer) {
  if (viaLayer.isWireLayer) {
    viaTraceVec_[idx(viaLayer.via)].setWireSideUsed();
  }
  else {
    viaTraceVec_[idx(viaLayer.via)].setStripSideUsed();
  }
}


bool Dijkstra::getViaLayerUsed(const ViaLayer& viaLayer) {
  if (viaLayer.isWireLayer) {
    return viaTraceVec_[idx(viaLayer.via)].wireSideIsUsed();
  }
  else {
    return viaTraceVec_[idx(viaLayer.via)].stripSideIsUsed();
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

bool Dijkstra::findCosts(Settings& settings, ViaLayerStartEnd& viaStartEnd)
{
  auto& start = viaStartEnd.start;
  auto& end = viaStartEnd.end;
  assert(!start.isWireLayer);
  assert(!end.isWireLayer);
  ViaLayerCost nodeCost(start, 0);
  priority_queue<ViaLayerCost> frontierPri;
  set<ViaLayer> frontierSet;
  set<ViaLayer> exploredSet;

//  fmt::print("start={}, end={}\n", start.str(), end.str());

  setCost(nodeCost);
  frontierPri.push(nodeCost);
  frontierSet.insert(nodeCost);

  bool success = false;

  while (true) {
    if (!frontierPri.size()) {
      return success;
    }
    ViaLayerCost node = frontierPri.top();
    frontierPri.pop();
    frontierSet.erase(node);
    if ((node.via == end.via).all()) {
      success = true;
    }
    exploredSet.insert(node);
    vector<ViaLayerCost> neighborVec;
    if (node.isWireLayer) {
      if (node.via.x() > 0) {
        neighborVec.push_back(
          ViaLayerCost(node.via.x() - 1, node.via.y(), true, node.cost + settings.wire_cost));
      }
      if (node.via.x() < static_cast<s32>(gridW_) - 1) {
        neighborVec.push_back(
          ViaLayerCost(node.via.x() + 1, node.via.y(), true, node.cost + settings.wire_cost));
      }
      neighborVec.push_back(
        ViaLayerCost(node.via.x(), node.via.y(), false, node.cost + settings.via_cost));
    }
    else {
      if (node.via.y() > 0) {
        neighborVec.push_back(
          ViaLayerCost(node.via.x(), node.via.y() - 1, false, node.cost + settings.strip_cost));
      }
      if (node.via.y() < static_cast<s32>(gridH_) - 1) {
        neighborVec.push_back(
          ViaLayerCost(node.via.x(), node.via.y() + 1, false, node.cost + settings.strip_cost));
      }
      neighborVec.push_back(
        ViaLayerCost(node.via.x(), node.via.y(), true, node.cost + settings.via_cost));
    }

    for (auto n : neighborVec) {
      if (getViaLayerUsed(n)) {
        continue;
      }
      if (!exploredSet.count(n)) {
        auto frontier_n = frontierSet.find(n);
        if (frontier_n == frontierSet.end()) {
          //        if (!frontierSet.count(n)) {
          frontierPri.push(n);
          frontierSet.insert(n);
          setCost(n);
        }
//        for (auto frontier_n : frontierSet) {
//          auto frontier_cost = getCost(frontier_n);
//          if (frontier_cost > n.cost) {
//            setCost(node, n.cost);
//          }
//        }
        else {
          auto frontier_cost = getCost(*frontier_n);
          if (frontier_cost > n.cost) {
            setCost(node, n.cost);
          }
        }
      }
    }
  }
}

u32 Dijkstra::getCost(const ViaLayer& viaLayer)
{
  u32 idx = viaLayer.via.x() + gridW_ * viaLayer.via.y();
  if (viaLayer.isWireLayer) {
    return viaCostVec_[idx].wireCost;
  }
  else {
    return viaCostVec_[idx].stripCost;
  }
}

void Dijkstra::setCost(const ViaLayer& viaLayer, u32 cost)
{
  u32 idx = viaLayer.via.x() + gridW_ * viaLayer.via.y();
  if (viaLayer.isWireLayer) {
    viaCostVec_[idx].wireCost = cost;
  }
  else {
    viaCostVec_[idx].stripCost = cost;
  }
}

void Dijkstra::setCost(const ViaLayerCost& viaLayerCost)
{
  setCost(viaLayerCost, viaLayerCost.cost);
}

RouteStepVec Dijkstra::backtraceLowestCostRoute(u32& routeCost, ViaLayerStartEnd& viaStartEnd)
{
  routeCost = 0;
  auto start = viaStartEnd.start;
  auto end = viaStartEnd.end;
  RouteStepVec routeStepVec;
  auto c = end;
  routeStepVec.push_back(c);
  u32 BUG = 0;
  while (!(c.via == start.via).all()) {
//    fmt::print("start: {} c: {} cost: {}\n", start.str(), c.str(), routeCost);
    // TODO: This is a workaround for a bug that causes the backtrace to sometimes get stuck.
    if (BUG++ > 1000) {
      break;
    }
    ViaLayer n = c;
    if (c.isWireLayer) {
      auto nLeft = stepLeft(c);
      if (c.via.x() > 0 && getCost(nLeft) < getCost(n)) {
        n = nLeft;
      }
      auto nRight = stepRight(c);
      if (c.via.x() < static_cast<s32>(gridW_) - 1 && getCost(nRight) < getCost(n)) {
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
      if (c.via.y() < static_cast<s32>(gridH_) - 1 && getCost(nDown) < getCost(n)) {
        n = nDown;
      }
      auto nWire = stepToWire(c);
      if (getCost(nWire) < getCost(n)) {
        n = nWire;
      }
    }
    c = n;
    routeCost += getCost(c);
    routeStepVec.push_back(c);
  }
  return routeStepVec;
}

void Dijkstra::dump()
{
  fmt::print("Wire layer\n");
  dumpLayer(true);
  fmt::print("Strip layer\n");
  dumpLayer(false);
}

void Dijkstra::dumpLayer(bool wireLayer)
{
  for (u32 y = 0; y < gridH_; ++y) {
    for (u32 x = 0; x < gridW_; ++x) {
      u32 v = getCost(ViaLayer(Via(x, y), wireLayer));
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

int Dijkstra::idx(const Via& v)
{
  return v.x() + gridW_ * v.y();
}

ViaLayer Dijkstra::stepLeft(const ViaLayer& v)
{
  return ViaLayer(v.via + Via(-1,0), v.isWireLayer);
}

ViaLayer Dijkstra::stepRight(const ViaLayer& v)
{
  return ViaLayer(v.via + Via(+1,0), v.isWireLayer);
}

ViaLayer Dijkstra::stepUp(const ViaLayer& v)
{
  return ViaLayer(v.via + Via(0,-1), v.isWireLayer);
}

ViaLayer Dijkstra::stepDown(const ViaLayer& v)
{
  return ViaLayer(v.via + Via(0,+1), v.isWireLayer);
}

ViaLayer Dijkstra::stepToWire(const ViaLayer& v)
{
  assert(!v.isWireLayer);
  return ViaLayer(v.via, true);
}

ViaLayer Dijkstra::stepToStrip(const ViaLayer& v)
{
  assert(v.isWireLayer);
  return ViaLayer(v.via, false);
}

bool Dijkstra::isStopRequested(mutex& stopThreadMutex)
{
  bool lockObtained = stopThreadMutex.try_lock();
  if (lockObtained) {
    stopThreadMutex.unlock();
  }
  return !lockObtained;
}
