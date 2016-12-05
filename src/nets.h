#pragma once

#include <set>
#include <vector>

#include "solution.h"


class Nets
{
public:
  Nets(Solution &);
  ViaSet &getViaSet(const Via &via);
  bool isEquivalentVia(const Via &currentVia, const Via &targetVia);
  bool hasEquivalent(const Via &via);
  void joinEquivalent(const Via &viaA, const Via &viaB);
  void joinEquivalentRoute(const RouteStepVec &routeStepVec);
  void registerPin(const Via &via);
  int getViaSetIdx(const Via &via);
  int createViaSet();
private:
  Solution &solution_;
  SetIdxVec &setIdxVec_;
  ViaSetVec &viaSetVec_;
};
