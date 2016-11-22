#include "circuit.h"


std::mutex circuitMutex;


using namespace std;


RelCoord::RelCoord(s32 xIn, s32 yIn)
  : x(xIn), y(yIn)
{}

RelCoordStartEnd::RelCoordStartEnd(const RelCoord& startIn, const RelCoord& endIn)
  : start(startIn), end(endIn)
{}

Circuit::Circuit()
  : hasError_(false)
{}

//Circuit& Circuit::operator=(const Circuit& other)
//{
//  if(&other == this) {
//    return *this;
//  }
//  lock_guard<mutex> _(mutex_);
//  circuitInfoVec_ = other.circuitInfoVec_;
//  componentName2ComponentInfo_ = other.componentName2ComponentInfo_;
//  componentName2PackageName_ = other.componentName2PackageName_;
//  componentToAbsCoordMap_ = other.componentToAbsCoordMap_;
//  connectionCoordVec_ = other.connectionCoordVec_;
//  connectionPairVec_ = other.connectionPairVec_;
//  packageToCoordMap_ = other.packageToCoordMap_;
//  routeVec_ = other.routeVec_;
//  hasError_ = other.hasError_;
//  return *this;
//}

CircuitInfoVec& Circuit::getCircuitInfoVec()
{
  return circuitInfoVec_;
}

const CircuitInfoVec& Circuit::getCircuitInfoVec() const
{
  return circuitInfoVec_;
}


ComponentName2ComponentInfo& Circuit::getComponentInfoMap()
{
  return componentName2ComponentInfo_;
}

const ComponentName2ComponentInfo& Circuit::getComponentInfoMap() const
{
  return componentName2ComponentInfo_;
}


ComponentName2PackageName& Circuit::getComponentName2PackageName()
{
  return componentName2PackageName_;
}

const ComponentName2PackageName& Circuit::getComponentName2PackageName() const
{
  return componentName2PackageName_;
}


const ComponentNameVec Circuit::getComponentNameVec() const
{
  ComponentNameVec v;
  for (auto n : getComponentName2PackageName()) {
    v.push_back(n.first);
  }
  return v;
}


ComponentToCoordMap& Circuit::getComponentToCoordMap()
{
  return componentToAbsCoordMap_;
}

const ComponentToCoordMap& Circuit::getComponentToCoordMap() const
{
  return componentToAbsCoordMap_;
}


ConnectionCoordVec& Circuit::getConnectionCoordVec()
{
  return connectionCoordVec_;
}

const ConnectionCoordVec& Circuit::getConnectionCoordVec() const
{
  return connectionCoordVec_;
}


ConnectionPairVec& Circuit::getConnectionPairVec()
{
  return connectionPairVec_;
}

const ConnectionPairVec& Circuit::getConnectionPairVec() const
{
  return connectionPairVec_;
}


PackageToCoordMap& Circuit::getPackageToCoordMap()
{
  return packageToCoordMap_;
}

const PackageToCoordMap& Circuit::getPackageToCoordMap() const
{
  return packageToCoordMap_;
}


void Circuit::dump() const
{
//  fmt::print("{},{} - {},{}\n", fc.x1, fc.y1, fc.x2, fc.y2);
}


void Circuit::setErrorBool(bool errorBool)
{
  hasError_ = errorBool;
}

bool Circuit::getErrorBool() const
{
  return hasError_;
}
