#include <limits.h>

#include <fmt/format.h>

#include "via.h"

//
// Via
//

std::string str(const Via &v)
{
  return fmt::format("{},{}", v.x(), v.y());
}

//
// ViaValid
//

ViaValid::ViaValid()
  : via(-1, -1), isValid(false)
{
}

ViaValid::ViaValid(const Via &_via)
  : via(_via), isValid(true)
{
}

ViaValid::ViaValid(const Via &_via, bool _isValid)
  : via(_via), isValid(_isValid)
{
}

//
// ViaLayer
//

ViaLayer::ViaLayer()
  : via(-1, -1), isWireLayer(false)
{
}

ViaLayer::ViaLayer(const ViaLayer &other)
  : via(other.via), isWireLayer(other.isWireLayer)
{
}

ViaLayer::ViaLayer(const Via &_via, bool _isWireLayer)
  : via(_via), isWireLayer(_isWireLayer)
{
}

ViaLayer::ViaLayer(const ViaLayerCost &viaLayerCost)
  : via(viaLayerCost.via), isWireLayer(viaLayerCost.isWireLayer)
{
}

bool operator<(const ViaLayer &l, const ViaLayer &r)
{
  return std::tie(l.via.x(), l.via.y(), l.isWireLayer)
    < std::tie(r.via.x(), r.via.y(), r.isWireLayer);
}

bool operator>(const ViaLayer &l, const ViaLayer &r)
{
  return r < l;
}
bool operator<=(const ViaLayer &l, const ViaLayer &r)
{
  return !(l > r);
}
bool operator>=(const ViaLayer &l, const ViaLayer &r)
{
  return !(l < r);
}

bool operator==(const ViaLayer &l, const ViaLayerCost &r)
{
  return (l.via == r.via).all() && l.isWireLayer == r.isWireLayer;
}

bool operator!=(const ViaLayer &l, const ViaLayerCost &r)
{
  return !(l == r);
}

std::string ViaLayer::str() const
{
  return fmt::format("{},{}", ::str(via), isWireLayer ? "wire" : "strip");
}

//
// ViaLayerCost
//

ViaLayerCost::ViaLayerCost()
  : ::ViaLayer(), cost(0)
{
}

ViaLayerCost::ViaLayerCost(const ViaLayer &viaLayerIn, int costIn)
  : ::ViaLayer(viaLayerIn), cost(costIn)
{
}

ViaLayerCost::ViaLayerCost(int xIn, int yIn, bool isWireLayerIn, int costIn)
  : ::ViaLayer(ViaLayer(Via(xIn, yIn), isWireLayerIn)), cost(costIn)
{
}

//std::string ViaLayerCost::str()
//{
//  return fmt::format("{} cost={:n}", ViaLayer::str(), cost);
//}
//const bool ViaLayerCost::operator < ( const ViaLayerCost& r ) const
//{
//  return cost < r.cost;
//}

bool operator<(const ViaLayerCost &l, const ViaLayerCost &r)
{
  return std::tie(l.cost, l.via.x(), l.via.y(), l.isWireLayer)
    > std::tie(r.cost, r.via.x(), r.via.y(), r.isWireLayer);
}

//
// ViaStartEnd
//

ViaStartEnd::ViaStartEnd()
  : start(-1, -1), end(-1, -1)
{
}

ViaStartEnd::ViaStartEnd(const Via &startIn, const Via &endIn)
  : start(startIn), end(endIn)
{
}

//
// ViaLayerStartEnd
//

ViaLayerStartEnd::ViaLayerStartEnd()
{
}

ViaLayerStartEnd::ViaLayerStartEnd(const ViaLayer &startIn,
                                   const ViaLayer &endIn)
  : start(startIn), end(endIn)
{
}

//
// ViaTrace
//

//ComponentPin::ComponentPin(std::string nameIn, int pinIdxIn)
//  : name(nameIn), pinIdx(pinIdxIn)
//{}

ViaTrace::ViaTrace()
  : isWireSideBlocked(false)
{
}

//
// ViaCost
//

ViaCost::ViaCost()
  : wireCost(INT_MAX), stripCost(INT_MAX)
{
}

