#pragma once

#include <vector>

#include "int_types.h"
#include "circuit.h"


typedef std::vector<bool> GridVec;
typedef std::vector<int> CostVec;


const int DEFAULT_WIRE_COST = 5;
const int DEFAULT_STRIP_COST = 1;
const int DEFAULT_VIA_COST = 50;


class Costs {
public:
  Costs();
  int wire;
  int strip;
  int via;
};


class Dijkstra {
public:
  Dijkstra();
  void findCheapestRoute(Costs& costs, Circuit& circuit, StartEndCoord& startEndCoord);
  void addRouteToUsed(RouteStepVec &);
  void addCoordToUsed(HoleCoord&);
  void dump();
private:
  bool findCosts(Costs& costs, StartEndCoord&);
  HoleCoord minCost();
  RouteStepVec walkCheapestPath(StartEndCoord&);

  int idx(HoleCoord&);
  int idxLeft(HoleCoord&);
  int idxRight(HoleCoord&);
  int idxUp(HoleCoord&);
  int idxDown(HoleCoord&);

  GridVec wireVec;
  GridVec stripVec;
  CostVec wireCostVec;
  CostVec stripCostVec;

  GridVec usedWireVec;
  GridVec usedStripVec;

  u32 totalCost_;
  u32 numCompletedRoutes = 0;
  u32 numFailedRoutes = 0;
};
