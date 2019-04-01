#include <climits>

#include <fmt/format.h>

#include "via.h"

//
// Via
//

std::string str(const Via& v)
{
  return fmt::format("{},{}", v.x(), v.y());
}

namespace std
{
bool operator<(const Via& l, const Via& r)
{
  return std::tie(l.x(), l.y()) < std::tie(r.x(), r.y());
}

bool operator==(const Via& l, const Via& r)
{
  return l.x() == r.x() && l.y() == r.y();
}
} // namespace std

//
// ValidVia
//

ValidVia::ValidVia() : via(-1, -1), isValid(false)
{
}

ValidVia::ValidVia(const Via& _via) : via(_via), isValid(true)
{
}

ValidVia::ValidVia(const Via& _via, bool _isValid)
  : via(_via), isValid(_isValid)
{
}

//
// LayerVia
//

LayerVia::LayerVia() : via(-1, -1), isWireLayer(false)
{
}

LayerVia::LayerVia(const LayerVia& other)
  : via(other.via), isWireLayer(other.isWireLayer)
{
}

LayerVia::LayerVia(const Via& _via, bool _isWireLayer)
  : via(_via), isWireLayer(_isWireLayer)
{
}

LayerVia::LayerVia(const LayerCostVia& viaLayerCost)
  : via(viaLayerCost.via), isWireLayer(viaLayerCost.isWireLayer)
{
}

namespace std
{

bool operator<(const LayerVia& l, const LayerVia& r)
{
  return std::tie(l.via.x(), l.via.y(), l.isWireLayer)
         < std::tie(r.via.x(), r.via.y(), r.isWireLayer);
}

bool operator==(const LayerVia& l, const LayerVia& r)
{
  return (l.via == r.via) && l.isWireLayer == r.isWireLayer;
}
} // namespace std

std::string LayerVia::str() const
{
  return fmt::format("{},{}", ::str(via), isWireLayer ? "wire" : "strip");
}

//
// LayerCostVia
//

LayerCostVia::LayerCostVia() : ::LayerVia(), cost(0)
{
}

LayerCostVia::LayerCostVia(const LayerVia& viaLayerIn, int costIn)
  : ::LayerVia(viaLayerIn), cost(costIn)
{
}

LayerCostVia::LayerCostVia(int xIn, int yIn, bool isWireLayerIn, int costIn)
  : ::LayerVia(LayerVia(Via(xIn, yIn), isWireLayerIn)), cost(costIn)
{
}

std::string LayerCostVia::str()
{
  return fmt::format("{},cost={:n}", LayerVia::str(), cost);
}

namespace std
{
bool operator<(const LayerCostVia& l, const LayerCostVia& r)
{
  return std::tie(l.cost, l.via.x(), l.via.y(), l.isWireLayer)
         > std::tie(r.cost, r.via.x(), r.via.y(), r.isWireLayer);
}

bool operator==(const LayerCostVia& l, const LayerCostVia& r)
{
  return l.cost == r.cost && l.via == r.via && l.isWireLayer == r.isWireLayer;
}
} // namespace std

//
// StartEndVia
//

StartEndVia::StartEndVia() : start(-1, -1), end(-1, -1)
{
}

StartEndVia::StartEndVia(const Via& startIn, const Via& endIn)
  : start(startIn), end(endIn)
{
}

//
// LayerStartEndVia
//

LayerStartEndVia::LayerStartEndVia()
{
}

LayerStartEndVia::LayerStartEndVia(
    const LayerVia& startIn, const LayerVia& endIn)
  : start(startIn), end(endIn)
{
}

//
// WireLayerVia
//

WireLayerVia::WireLayerVia() : isWireSideBlocked(false)
{
}

//
// CostVia
//

CostVia::CostVia() : wireCost(INT_MAX), stripCost(INT_MAX)
{
}
