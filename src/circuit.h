#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>
#include <mutex>

#include "int_types.h"
#include "via.h"


extern std::mutex circuitMutex;

class RelCoord {
public:
  RelCoord(s32 x, s32 y);
  s32 x;
  s32 y;
};


class RelCoordStartEnd {
public:
  RelCoordStartEnd(const RelCoord&, const RelCoord&);
  RelCoord start;
  RelCoord end;
};


typedef std::vector<RelCoord> RelCoordVec;
typedef std::map<std::string, RelCoordVec> PackageToCoordMap;
typedef std::map<std::string, std::string> ComponentName2PackageName;
typedef std::map<std::string, Via> ComponentToCoordMap;
typedef std::pair<std::string, u32> ComponentPinPair;
typedef std::pair<ComponentPinPair, ComponentPinPair> ConnectionPair;
typedef std::vector<ConnectionPair> ConnectionPairVec;
typedef std::vector<std::string> ComponentNameVec;


class ComponentInfo {
public:
  std::string componentName;
  ViaStartEnd footprint;
  Via pin0AbsCoord;
  RelCoordVec pinAbsCoordVec;
};

typedef std::map<std::string, ComponentInfo> ComponentName2ComponentInfo;
typedef std::vector<ViaStartEnd> ConnectionCoordVec;

typedef std::vector<std::string> CircuitInfoVec;

class Circuit {
public:
  Circuit();
//  Circuit& operator=(const Circuit& other);
  CircuitInfoVec& getCircuitInfoVec();
  const CircuitInfoVec& getCircuitInfoVec() const;

  ComponentName2ComponentInfo& getComponentInfoMap();
  const ComponentName2ComponentInfo& getComponentInfoMap() const;

  ComponentName2PackageName& getComponentName2PackageName();
  const ComponentName2PackageName& getComponentName2PackageName() const;

  const ComponentNameVec getComponentNameVec() const;

  ComponentToCoordMap& getComponentToCoordMap();
  const ComponentToCoordMap& getComponentToCoordMap() const;

  ConnectionCoordVec& getConnectionCoordVec();
  const ConnectionCoordVec& getConnectionCoordVec() const;

  ConnectionPairVec& getConnectionPairVec();
  const ConnectionPairVec& getConnectionPairVec() const;

  PackageToCoordMap& getPackageToCoordMap();
  const PackageToCoordMap& getPackageToCoordMap() const;

  void dump() const;

  void setErrorBool(bool errorBool);
  bool getErrorBool() const;
private:
  CircuitInfoVec circuitInfoVec_;
  ComponentName2ComponentInfo componentName2ComponentInfo_;
  ComponentName2PackageName componentName2PackageName_;
  ComponentToCoordMap componentToAbsCoordMap_;
  ConnectionCoordVec connectionCoordVec_;
  ConnectionPairVec connectionPairVec_;
  PackageToCoordMap packageToCoordMap_;
  bool hasError_;
};
