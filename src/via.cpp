#include <limits.h>

#include <fmt/format.h>

#include "via.h"


using namespace std;

//
// Via
//

Via::Via()
  : x(0), y(0), isValid(false)
{}

Via::Via(u32 xIn, u32 yIn)
  : x(xIn), y(yIn), isValid(true)
{}

//
// ViaLayer
//

ViaLayer::ViaLayer()
: isWireLayer(false)
{}

ViaLayer::ViaLayer(u32 xIn, u32 yIn, bool isWireLayerIn)
  : Via(xIn, yIn), isWireLayer(isWireLayerIn)
{}

bool operator<(const ViaLayer& l, const ViaLayer& r)
{
//  fmt::print("ViaLayer operator<\n");
  if (l.x == r.x && l.y == r.y) {
    return !l.isWireLayer && r.isWireLayer;
  }
  return l.x + 10000 * l.y < r.x + 10000 * r.y;
}

string ViaLayer::str()
{
  return fmt::format("x={} y={} wire={}", x, y, isWireLayer);
}

//
// ViaStartEnd
//

ViaStartEnd::ViaStartEnd()
{}

ViaStartEnd::ViaStartEnd(const ViaLayer& startIn, const ViaLayer& endIn)
: start(startIn), end(endIn)
{}

//
// ViaLayerCost
//

ViaLayerCost::ViaLayerCost(const ViaLayer& viaLayerIn, u32 costIn)
  : viaLayer(viaLayerIn), cost(costIn)
{}

ViaLayerCost::ViaLayerCost(u32 xIn, u32 yIn, bool isWireLayerIn, u32 costIn)
: viaLayer(ViaLayer(xIn, yIn, isWireLayerIn)), cost(costIn)
{}

std::string ViaLayerCost::str()
{
  return fmt::format("{} cost={}", viaLayer.str(), cost);
}
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

