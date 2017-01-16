#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include "via.h"

// Packages

typedef std::vector<Via> PackageRelPosVec;
typedef std::map<std::string, PackageRelPosVec> PackageToPosMap;

// Components

typedef std::set<int> DontCarePinIdxSet;

class Component
{
  public:
  Component();
  Component(const std::string&, const Via&);
  std::string packageName;
  Via pin0AbsPos;
  DontCarePinIdxSet dontCarePinIdxSet;
};

typedef std::map<std::string, Component> ComponentNameToComponentMap;

// Connections

class ConnectionPoint
{
  public:
  ConnectionPoint(const std::string&, int _pinIdx);
  std::string componentName;
  int pinIdx;
};

class Connection
{
  public:
  Connection(const ConnectionPoint& _start, const ConnectionPoint& _end);
  ConnectionPoint start;
  ConnectionPoint end;
};

// Circuit

typedef std::vector<Connection> ConnectionVec;
typedef std::vector<StartEndVia> ConnectionViaVec;
typedef std::vector<std::string> StringVec;
typedef std::vector<Via> PinViaVec;

class Circuit
{
  public:
  Circuit();
  bool hasParserError() const;
  ConnectionViaVec genConnectionViaVec() const;
  StartEndVia calcComponentFootprint(std::string componentName) const;
  PinViaVec calcComponentPins(std::string componentName) const;
  PackageToPosMap packageToPosMap;
  ComponentNameToComponentMap componentNameToComponentMap;
  ConnectionVec connectionVec;
  StringVec parserErrorVec;
};
