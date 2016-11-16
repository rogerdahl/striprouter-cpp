#include <algorithm>
#include <limits.h>
#include <mutex>

#include <fmt/format.h>

#include "dijkstra.h"


using namespace std;


const int W = 60;
const int H = 60;


Costs::Costs()
  : wire(DEFAULT_WIRE_COST), strip(DEFAULT_STRIP_COST), via(DEFAULT_VIA_COST)
{}


Dijkstra::Dijkstra()
: totalCost_(0), numCompletedRoutes(0), numFailedRoutes(0)
{
  usedWireVec = GridVec(W * H, false);
  usedStripVec = GridVec(W * H, false);
}


void Dijkstra::findCheapestRoute(Costs& costs, Circuit& circuit, StartEndCoord& startEndCoord)
{
  wireVec = GridVec(W * H, false);
  stripVec = GridVec(W * H, false);
  wireCostVec = CostVec(W * H, INT_MAX);
  stripCostVec = CostVec(W * H, INT_MAX);

  bool success_bool = findCosts(costs, startEndCoord);
  if (success_bool) {
    ++numCompletedRoutes;
    auto routeStepVec = walkCheapestPath(startEndCoord);
    addRouteToUsed(routeStepVec);
    reverse(routeStepVec.begin(), routeStepVec.end());
    {
      lock_guard<mutex> lockCircuit(circuitMutex);
      circuit.getRouteVec().push_back(routeStepVec);
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
    addCoordToUsed(c);
  }
}


void Dijkstra::addCoordToUsed(HoleCoord& c) {
  if (c.onWireSide) {
    usedWireVec[idx(c)] = true;
  }
  else {
    usedStripVec[idx(c)] = true;
  }
}

//
// Private
//

bool Dijkstra::findCosts(Costs& costs, StartEndCoord& startEndCoord)
{
  auto start = startEndCoord.start;
  auto end = startEndCoord.end;

  if (start.onWireSide) {
    wireCostVec[idx(start)] = 0;
  }
  else {
    stripCostVec[idx(start)] = 0;
  }

  while (true) {
    auto minCoord = minCost();
    if (!minCoord.isValid) {
      return false;
    }
    if (minCoord.x == end.x && minCoord.y == end.y && minCoord.onWireSide == end.onWireSide) {
      if (minCoord.onWireSide && wireCostVec[idx(minCoord)] == INT_MAX) {
        return false;
      }
      if (!minCoord.onWireSide && stripCostVec[idx(minCoord)] == INT_MAX) {
        return false;
      }
      return true;
    }
    // Wires run horizontally
    if (minCoord.onWireSide) {
      // Mark the picked vertex as processed
      wireVec[idx(minCoord)] = true;
      // Left
      if (minCoord.x > 0 && !usedWireVec[idxLeft(minCoord)] && !wireVec[idxLeft(minCoord)] && wireCostVec[idx(minCoord)] != INT_MAX &&
          wireCostVec[idx(minCoord)] + costs.wire < wireCostVec[idxLeft(minCoord)]) {
        wireCostVec[idxLeft(minCoord)] = wireCostVec[idx(minCoord)] + costs.wire;
      }
      // Right
      if (minCoord.x < W - 1 && !usedWireVec[idxRight(minCoord)] && !wireVec[idxRight(minCoord)] && wireCostVec[idx(minCoord)] != INT_MAX &&
          wireCostVec[idx(minCoord)] + costs.wire < wireCostVec[idxRight(minCoord)]) {
        wireCostVec[idxRight(minCoord)] = wireCostVec[idx(minCoord)] + costs.wire;
      }
      // Through to strip layer
      if (!stripVec[idx(minCoord)] && !usedStripVec[idx(minCoord)] && wireCostVec[idx(minCoord)] != INT_MAX &&
          wireCostVec[idx(minCoord)] + costs.via < stripCostVec[idx(minCoord)]) {
        stripCostVec[idx(minCoord)] = wireCostVec[idx(minCoord)] + costs.via;
      }
    }
    // Strips run vertically
    else {
      // Mark the picked vertex as processed
      stripVec[idx(minCoord)] = true;
      // Up
      if (minCoord.y > 0 && !usedStripVec[idxUp(minCoord)] && !stripVec[idxUp(minCoord)] && stripCostVec[idx(minCoord)] != INT_MAX &&
          stripCostVec[idx(minCoord)] + costs.strip < stripCostVec[idxUp(minCoord)]) {
        stripCostVec[idxUp(minCoord)] = stripCostVec[idx(minCoord)] + costs.strip;
      }
      // Down
      if (minCoord.y < H - 1 && !usedStripVec[idxDown(minCoord)]  && !stripVec[idxDown(minCoord)] && stripCostVec[idx(minCoord)] != INT_MAX &&
          stripCostVec[idx(minCoord)] + costs.strip < stripCostVec[idxDown(minCoord)]) {
        stripCostVec[idxDown(minCoord)] = stripCostVec[idx(minCoord)] + costs.strip;
      }
      // Through to wire layer
      if (!wireVec[idx(minCoord)] && !usedWireVec[idx(minCoord)] && stripCostVec[idx(minCoord)] != INT_MAX &&
          stripCostVec[idx(minCoord)] + costs.via < wireCostVec[idx(minCoord)]) {
        wireCostVec[idx(minCoord)] = stripCostVec[idx(minCoord)] + costs.via;
      }
    }
  }
}


HoleCoord Dijkstra::minCost()
{
  int min = INT_MAX;
  HoleCoord minCoord;
  minCoord.isValid = false;
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      // wire side
      HoleCoord wireCoord(x, y, true);
      if (!wireVec[idx(wireCoord)] && !usedWireVec[idx(wireCoord)] && wireCostVec[idx(wireCoord)] <= min) {
//      if (!wireVec[idx(wireCoord)] && wireCostVec[idx(wireCoord)] <= min) {
        min = wireCostVec[idx(wireCoord)];
        minCoord = wireCoord;
        minCoord.isValid = true;
      }
      // strip side
      HoleCoord stripCoord(x, y, false);
//      if (!stripVec[idx(stripCoord)] && !usedStripVec[idx(stripCoord)] && stripCostVec[idx(stripCoord)] <= min) {
      if (!stripVec[idx(stripCoord)] && stripCostVec[idx(stripCoord)] <= min) {
        min = stripCostVec[idx(stripCoord)];
        minCoord = stripCoord;
        minCoord.isValid = true;
      }
    }
  }
  return minCoord;
}


RouteStepVec Dijkstra::walkCheapestPath(StartEndCoord& startEndCoord)
{
  auto start = startEndCoord.start;
  auto end = startEndCoord.end;

  RouteStepVec routeStepVec;
  auto c = end;
  HoleCoord n;
  routeStepVec.push_back(c);
  while (c.x != start.x || c.y != start.y || c.onWireSide != start.onWireSide) {
    n = c;
    if (c.onWireSide) {
      if (c.x > 0) {
        n = HoleCoord(c.x - 1, c.y, c.onWireSide);
      }
      if (c.x < W - 1 && wireCostVec[idxRight(c)] <= wireCostVec[idx(n)]) {
        n = HoleCoord(c.x + 1, c.y, c.onWireSide);
      }
      if (stripCostVec[idx(c)] <= wireCostVec[idx(n)]) {
        n = HoleCoord(c.x, c.y, false);
      }
    }
    else {
      if (c.y > 0) {
        n = HoleCoord(c.x, c.y - 1, c.onWireSide);
      }
      if (c.y < H - 1 && stripCostVec[idxDown(c)] < stripCostVec[idx(n)]) {
        n = HoleCoord(c.x, c.y + 1, c.onWireSide);
      }
      if (wireCostVec[idx(c)] < stripCostVec[idx(n)]) {
        n = HoleCoord(c.x, c.y, true);
      }
    }
    c = n;

    if (c.onWireSide) {
      totalCost_ += wireCostVec[idx(c)];
    }
    else {
      totalCost_ += stripCostVec[idx(c)];
    }

    routeStepVec.push_back(c);
  }
  return routeStepVec;
}

void Dijkstra::dump()
{
  fmt::print("Wire side\n");
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int v = wireCostVec[x + y * W];
      if (v == INT_MAX) {
        v = -1;
      }
      fmt::print("{} ", v);
    }
    fmt::print("\n");
  }
  fmt::print("\n");

  fmt::print("Strip side\n");
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      int v = stripCostVec[x + y * W];
      if (v == INT_MAX) {
        v = -1;
      }
      fmt::print("{} ", v);
    }
    fmt::print("\n");
  }
  fmt::print("\n");
}


int Dijkstra::idx(HoleCoord& c)
{
  return c.x + W * c.y;
}

int Dijkstra::idxLeft(HoleCoord& c)
{
  return idx(c) - 1;
}

int Dijkstra::idxRight(HoleCoord& c)
{
  return idx(c) + 1;
}

int Dijkstra::idxUp(HoleCoord& c)
{
  return idx(c) - W;
}

int Dijkstra::idxDown(HoleCoord& c)
{
  return idx(c) + W;
}
