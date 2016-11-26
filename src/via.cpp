#include <limits.h>

#include <fmt/format.h>

#include "via.h"


using namespace Eigen;
using namespace std;

//
// Via
//

ViaValid::ViaValid()
  : via(0,0), isValid(false)
{}

ViaValid::ViaValid(const Via& _via)
  : via(_via), isValid(true)
{}

ViaValid::ViaValid(const Via& _via, bool _isValid)
: via(_via), isValid(_isValid)
{}

//
// ViaLayer
//

ViaLayer::ViaLayer()
: via(0,0), isWireLayer(false)
{}

ViaLayer::ViaLayer(const Via& _via, bool _isWireLayer)
  : via(_via), isWireLayer(_isWireLayer)
{}

ViaLayer::ViaLayer(const ViaLayerCost& viaLayerCost)
  : via(viaLayerCost.via), isWireLayer(viaLayerCost.isWireLayer)
{}


bool operator<(const ViaLayer& l, const ViaLayer& r)
{
//  fmt::print("ViaLayer operator<\n");
  if ((l.via == r.via).all()) {
    return !l.isWireLayer && r.isWireLayer;
  }
  return l.via.x() + 10000 * l.via.y() < r.via.x() + 10000 * r.via.y(); // TODO: Fix constants
}

bool operator==(const ViaLayer& l, const ViaLayerCost& r)
{
  return (l.via == r.via).all() && l.isWireLayer == r.isWireLayer;
}

bool operator!=(const ViaLayer& l, const ViaLayerCost& r)
{
  return !(l == r);
}

//string ViaLayer::str()
//{
//  return fmt::format("{} wire={}", Via::str(), isWireLayer);
//}

//
// ViaLayerCost
//

ViaLayerCost::ViaLayerCost()
  : ::ViaLayer(), cost(0)
{}

ViaLayerCost::ViaLayerCost(const ViaLayer& viaLayerIn, u32 costIn)
  : ::ViaLayer(viaLayerIn), cost(costIn)
{}

ViaLayerCost::ViaLayerCost(u32 xIn, u32 yIn, bool isWireLayerIn, u32 costIn)
: ::ViaLayer(ViaLayer(Via(xIn, yIn), isWireLayerIn)), cost(costIn)
{}

//std::string ViaLayerCost::str()
//{
//  return fmt::format("{} cost={:n}", ViaLayer::str(), cost);
//}
//const bool ViaLayerCost::operator < ( const ViaLayerCost& r ) const
//{
//  return cost < r.cost;
//}

bool operator<(const ViaLayerCost& l, const ViaLayerCost& r)
{
//  fmt::print("ViaLayerCost operator<\n");
  return l.cost > r.cost;
}

//
// ViaStartEnd
//

ViaStartEnd::ViaStartEnd()
: start(0,0), end(0,0)
{}

ViaStartEnd::ViaStartEnd(const Via& startIn, const Via& endIn)
  : start(startIn), end(endIn)
{}

//
// ViaLayerStartEnd
//

ViaLayerStartEnd::ViaLayerStartEnd()
{}

ViaLayerStartEnd::ViaLayerStartEnd(const ViaLayer& startIn, const ViaLayer& endIn)
: start(startIn), end(endIn)
{}

//
// ViaTrace
//

ComponentPin::ComponentPin(string nameIn, int pinIdxIn)
  : name(nameIn), pinIdx(pinIdxIn)
{}

ViaTrace::ViaTrace()
: wireSideIsUsed_(false)
{}

bool ViaTrace::wireSideIsUsed()
{
  return wireSideIsUsed_;
}

bool ViaTrace::stripSideIsUsed()
{
  return componentPinVec_.size() > 0;
}

void ViaTrace::addComponentPin(const ComponentPin& componentPin)
{
  componentPinVec_.push_back(componentPin);
}

void ViaTrace::setWireSideUsed()
{
  wireSideIsUsed_ = true;
}

void ViaTrace::setStripSideUsed()
{
  addComponentPin(ComponentPin("test", 1));
}

//
// ViaCost
//

ViaCost::ViaCost()
  : wireCost(INT_MAX), stripCost(INT_MAX)
{}

