#pragma once

#include <string>
#include <vector>

#include <Eigen/Core>


// Hold floating point screen and board positions
typedef Eigen::Array2f Pos;

/*
 * We need to keep track of a bunch of types of vias:
 *
 * Via:
 * - Hold the coordinate of a via
 * - Used to specify start and end of route because routes always start and end on strip layer
 *
 * ViaLayer:
 * - Hold the coordinate for a layer of a via
 * - Has operator< on combination of all values, for use in std::set
 *
 * ViaLayerCost:
 * - Hold the coordinates and cost for a layer of a via
 * - Has operator< on cost for use in std::priority_queue
 * - Has constructor for creating from ViaLayer + cost
 *
 * ViaTrace:
 * - Block off component footprints
 * - Block off used vias on wire layer
 * - Keep track of where used vias on strip layer have been connected
 * - Always in a grid, so no need for coords
 * - Persistent through all routes
 *
 * ViaCost:
 * - Hold the current wire and strip layer costs for a via
 * - Always in a grid, so no need for coords
 * - Reset before starting each new route
 * - Default constructor resets wire and strip costs to INT_MAX
*/

// Via

typedef Eigen::Array2i Via;

//bool operator< (const Via& l, const Via& r);

//namespace std {
//  template <>
//  struct less {
//    bool operator()(const Eigen::Array2i& a, const Eigen::Array2i& b) const;
//  };
//}

//namespace std {
//  std::function<bool(const Eigen::Array2i &, const Eigen::Array2i &)>
//    m ([](const Eigen::Array2i &a, const Eigen::Array2i &b)->bool
//  {
//    return true;
//  });
//}


std::string str(const Via &v);

//

class ViaValid
{
public:
  ViaValid();
  ViaValid(const Via &);
  ViaValid(const Via &, bool isValid);
//  std::string str();
  Via via;
  bool isValid;
};

//

class ViaLayerCost;

class ViaLayer
{
public:
  ViaLayer();
  ViaLayer(const ViaLayer &);
  ViaLayer(const Via &, bool _isWireLayer);
  ViaLayer(const ViaLayerCost &);
  std::string str() const;
  Via via;
  bool isWireLayer;
};

bool operator<(const ViaLayer &l, const ViaLayer &r);
bool operator>(const ViaLayer &l, const ViaLayer &r);
bool operator<=(const ViaLayer &l, const ViaLayer &r);
bool operator>=(const ViaLayer &l, const ViaLayer &r);

bool operator==(const ViaLayer &l, const ViaLayerCost &r);
bool operator!=(const ViaLayer &l, const ViaLayerCost &r);

//

class ViaLayerCost: public ViaLayer
{
public:
  ViaLayerCost();
  ViaLayerCost(const ViaLayer &viaLayerIn, int costIn);
  ViaLayerCost(int xIn, int yIn, bool isWireLayerIn, int costIn);
//  std::string str();
  int cost;
};

bool operator<(const ViaLayerCost &l, const ViaLayerCost &r);

//

class ViaStartEnd
{
public:
  ViaStartEnd();
  ViaStartEnd(const Via &_start, const Via &_end);
  Via start;
  Via end;
};


//

class ViaLayerStartEnd
{
public:
  ViaLayerStartEnd();
  ViaLayerStartEnd(const ViaLayer &startIn, const ViaLayer &endIn);
  ViaLayer start;
  ViaLayer end;
};

//

class ViaTrace
{
public:
  ViaTrace();
  bool isWireSideBlocked;
  ViaValid wireToVia;
};

typedef std::vector<ViaTrace> ViaTraceVec;

//

class ViaCost
{
public:
  ViaCost();
  int wireCost;
  int stripCost;
};

typedef std::vector<ViaCost> ViaCostVec;
