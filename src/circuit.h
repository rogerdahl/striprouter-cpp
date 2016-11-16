#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>
#include <mutex>

#include "int_types.h"


extern std::mutex circuitMutex;


class HoleCoord {
public:
  HoleCoord();
  HoleCoord(int x_in, int y_in, bool onWireSide_in = false, bool isValid_in = true);
  s32 x;
  s32 y;
  bool onWireSide;
  bool isValid;
};

class StartEndCoord {
public:
  StartEndCoord();
  StartEndCoord(HoleCoord& start_in, HoleCoord& c2_in);
  HoleCoord start;
  HoleCoord end;
};

typedef std::vector<HoleCoord> CoordVec;
typedef std::map<std::string, CoordVec> PackageToCoordMap;
typedef std::map<std::string, std::string> ComponentName2PackageName;
typedef std::map<std::string, HoleCoord> ComponentToCoordMap;
typedef std::pair<std::string, u32> ComponentPinPair;
typedef std::pair<ComponentPinPair, ComponentPinPair> ConnectionPair;
typedef std::vector<ConnectionPair> ConnectionPairVec;
typedef std::vector<std::string> ComponentNameVec;


struct ComponentInfo {
  std::string componentName;
  StartEndCoord footprint;
  HoleCoord pin0AbsCoord;
  CoordVec pinAbsCoordVec;
};

typedef std::map<std::string, ComponentInfo> ComponentName2ComponentInfo;
typedef std::vector<StartEndCoord> ConnectionCoordVec;

typedef std::vector<std::string> CircuitInfoVec;

// Autoroutes

typedef std::vector<HoleCoord> RouteStepVec;
typedef std::vector<RouteStepVec> RouteVec;


class Circuit {
public:
  Circuit();
//  Circuit& operator=(const Circuit& other);
  CircuitInfoVec& getCircuitInfoVec();
  ComponentName2ComponentInfo& getComponentInfoMap();
  ComponentName2PackageName& getComponentName2PackageName();
  ComponentNameVec getComponentNameVec();
  ComponentToCoordMap& getComponentToCoordMap();
  ConnectionCoordVec& getConnectionCoordVec();
  ConnectionPairVec& getConnectionPairVec();
  PackageToCoordMap& getPackageToCoordMap();
  RouteVec& getRouteVec();
  void dump();
  void setErrorBool(bool errorBool);
  bool getErrorBool();
private:
  CircuitInfoVec circuitInfoVec_;
  ComponentName2ComponentInfo componentName2ComponentInfo_;
  ComponentName2PackageName componentName2PackageName_;
  ComponentToCoordMap componentToAbsCoordMap_;
  ConnectionCoordVec connectionCoordVec_;
  ConnectionPairVec connectionPairVec_;
  PackageToCoordMap packageToCoordMap_;
  RouteVec routeVec_;
  bool hasError_;
};
