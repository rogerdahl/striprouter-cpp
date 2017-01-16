#pragma once

#include <string>
#include <vector>

#include <Eigen/Core>

// Hold floating point screen and board positions
typedef Eigen::Array2f Pos;

// Hold integer positions
typedef Eigen::Vector2i IntPos;

//
// Via
//

typedef Eigen::Array2i Via;

std::string str(const Via& v);

//
// ValidVia
//

class ValidVia
{
  public:
  ValidVia();
  ValidVia(const Via&);
  ValidVia(const Via&, bool isValid);
  Via via;
  bool isValid;
};

//
// LayerCostVia
//

class LayerCostVia;

class LayerVia
{
  public:
  LayerVia();
  LayerVia(const LayerVia&);
  LayerVia(const Via&, bool _isWireLayer);
  LayerVia(const LayerCostVia&);
  std::string str() const;
  Via via;
  bool isWireLayer;
};

bool operator<(const LayerVia& l, const LayerVia& r);
bool operator>(const LayerVia& l, const LayerVia& r);
bool operator<=(const LayerVia& l, const LayerVia& r);
bool operator>=(const LayerVia& l, const LayerVia& r);

bool operator==(const LayerVia& l, const LayerCostVia& r);
bool operator!=(const LayerVia& l, const LayerCostVia& r);

//
// LayerCostVia
//

class LayerCostVia : public LayerVia
{
  public:
  LayerCostVia();
  LayerCostVia(const LayerVia& viaLayerIn, int costIn);
  LayerCostVia(int xIn, int yIn, bool isWireLayerIn, int costIn);
  std::string str();
  int cost;
};

bool operator<(const LayerCostVia& l, const LayerCostVia& r);

//
// StartEndVia
//

class StartEndVia
{
  public:
  StartEndVia();
  StartEndVia(const Via& _start, const Via& _end);
  Via start;
  Via end;
};

//
// LayerStartEndVia
//

class LayerStartEndVia
{
  public:
  LayerStartEndVia();
  LayerStartEndVia(const LayerVia& startIn, const LayerVia& endIn);
  LayerVia start;
  LayerVia end;
};

//
// WireLayerVia
//

class WireLayerVia
{
  public:
  WireLayerVia();
  bool isWireSideBlocked;
  ValidVia wireToVia;
};

typedef std::vector<WireLayerVia> WireLayerViaVec;

//
// CostVia
//

class CostVia
{
  public:
  CostVia();
  int wireCost;
  int stripCost;
};

typedef std::vector<CostVia> CostViaVec;
