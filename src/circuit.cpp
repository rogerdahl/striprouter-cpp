#include "circuit.h"

std::mutex circuitMutex;

using namespace std;

HoleCoord::HoleCoord()
  : x(-1), y(-1), onWireSide(false), isValid(false)
{}

HoleCoord::HoleCoord(int x_in, int y_in, bool onWireSide_in, bool isValid_in)
  : x(x_in), y(y_in), onWireSide(onWireSide_in), isValid(isValid_in)
{}

StartEndCoord::StartEndCoord()
{}

StartEndCoord::StartEndCoord(HoleCoord& start_in, HoleCoord& end_in)
  : start(start_in), end(end_in)
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

ComponentName2ComponentInfo& Circuit::getComponentInfoMap()
{
  return componentName2ComponentInfo_;
}

ComponentName2PackageName& Circuit::getComponentName2PackageName()
{
  return componentName2PackageName_;
}

ComponentNameVec Circuit::getComponentNameVec()
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

ConnectionCoordVec& Circuit::getConnectionCoordVec()
{
  return connectionCoordVec_;
}

ConnectionPairVec& Circuit::getConnectionPairVec()
{
  return connectionPairVec_;
}

PackageToCoordMap& Circuit::getPackageToCoordMap()
{
  return packageToCoordMap_;
}

RouteVec& Circuit::getRouteVec()
{
  return routeVec_;
}

void Circuit::dump()
{
//  fmt::print("{},{} - {},{}\n", fc.x1, fc.y1, fc.x2, fc.y2);
}

void Circuit::setErrorBool(bool errorBool)
{
  hasError_ = errorBool;
}

bool Circuit::getErrorBool()
{
  return hasError_;
}
