#pragma once

#include <set>
#include <vector>

#include "layout.h"

// This class keeps track of nets, which are pins and traces which are
// electrically connected and so can be considered to be equivalents by the
// router. When possible, the router uses this information to make shortcuts by
// routing to the lowest cost point that is connected to the final location,
// instead of to the actual final location.
//
// The nets are also what allows creating multiple routes from a single pin or
// to a single pin. Without the nets, the first route connected to a pin would
// block the pin off for other routes.

class Nets
{
  public:
  Nets(Layout&);
  void connect(const Via& viaA, const Via& viaB);
  void connectRoute(const RouteStepVec& routeStepVec);
  void registerPin(const Via& via);
  bool isConnected(const Via& currentVia, const Via& targetVia);
  bool hasConnection(const Via& via);
  ViaSet& getViaSet(const Via& via);
  int getViaSetIdx(const Via& via);

  private:
  int createViaSet();
  Layout& layout_;
  SetIdxVec& setIdxVec_;
  ViaSetVec& viaSetVec_;
};
