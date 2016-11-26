#pragma once

#include <string>
#include <vector>

#include <Eigen/Core>

#include "int_types.h"


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

typedef Eigen::Array2i Via;

class ViaValid {
public:
  ViaValid();
  ViaValid(const Via&);
  ViaValid(const Via&, bool isValid);
//  virtual std::string str();
  Via via;
  bool isValid;
};

//

class ViaLayerCost;

class ViaLayer {
public:
  ViaLayer();
  ViaLayer(const Via&, bool _isWireLayer);
  ViaLayer(const ViaLayerCost&);
//  std::string str();
  Via via;
  bool isWireLayer;
};

bool operator<(const ViaLayer& l, const ViaLayer& r);
bool operator==(const ViaLayer& l, const ViaLayerCost& r);
bool operator!=(const ViaLayer& l, const ViaLayerCost& r);

//

class ViaLayerCost : public ViaLayer
{
public:
  ViaLayerCost();
  ViaLayerCost(const ViaLayer& viaLayerIn, u32 costIn);
  ViaLayerCost(u32 xIn, u32 yIn, bool isWireLayerIn, u32 costIn);
//  const bool operator < ( const ViaLayerCost& r ) const;
//  std::string str();
  u32 cost;
};

bool operator<(const ViaLayerCost& l, const ViaLayerCost& r);

//

class ViaStartEnd {
public:
  ViaStartEnd();
  ViaStartEnd(const Via& _start, const Via& _end);
  Via start;
  Via end;
};


//

class ViaLayerStartEnd {
public:
  ViaLayerStartEnd();
  ViaLayerStartEnd(const ViaLayer& startIn, const ViaLayer& endIn);
  ViaLayer start;
  ViaLayer end;
};

//

class ComponentPin
{
public:
  ComponentPin(std::string nameIn, int pinIdxIn);
  std::string name;
  u32 pinIdx;
};

typedef std::vector<ComponentPin> ComponentPinVec;

class ViaTrace
{
public:
  ViaTrace();
  bool wireSideIsUsed();
  bool stripSideIsUsed();
  void addComponentPin(const ComponentPin& componentPin);
  void setWireSideUsed();
  void setStripSideUsed();
private:
  bool wireSideIsUsed_;
  ComponentPinVec componentPinVec_;
};

typedef std::vector<ViaTrace> ViaTraceVec;

//

class ViaCost
{
public:
  ViaCost();
  u32 wireCost;
  u32 stripCost;
};

typedef std::vector<ViaCost> ViaCostVec;
