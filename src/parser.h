#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "int_types.h"


typedef std::pair<s32, s32> CoordPair;
typedef std::vector<CoordPair> CoordVec;
typedef std::map<std::string, CoordVec> PackageToCoordMap;

typedef std::map<std::string, std::string> ComponentName2PackageName;

typedef std::map<std::string, CoordPair> ComponentToCoordMap;

typedef std::pair<std::string, u32> ComponentPinPair;
typedef std::pair<ComponentPinPair, ComponentPinPair> ConnectionPair;
typedef std::vector<ConnectionPair> ConnectionPairVec;

typedef std::vector<std::string> ComponentNameVec;

typedef std::pair<std::string, std::string> LineErrorPair;

struct FromToCoord {
  s32 x1;
  s32 y1;
  s32 x2;
  s32 y2;
};

struct ComponentInfo {
  std::string componentName;
  FromToCoord extent;
  CoordPair pin0AbsCoord;
  CoordVec pinAbsCoordVec;
};

typedef std::map<std::string, ComponentInfo> ComponentName2ComponentInfo;

//typedef std::vector<ComponentInfo> ComponentInfoVec;

typedef std::vector<FromToCoord> ConnectionCoordVec;

struct PackageInfo {
  std::string packageName;
  CoordVec coords;
};


class Parser {
public:
  Parser();
  ~Parser();
  LineErrorPair parse();

  ComponentNameVec getComponentNameVec();
  ConnectionCoordVec getConnectionCoordVec();
  ComponentInfo getComponentInfo(std::string componentName);
  
//  ComponentInfoVec getComponentInfoVec();
//  ConnectionInfoVec getConnectionInfoVec();
//  PackageInfo getPackageInfo(std::string package);
  void dump(FromToCoord &fc);

private:
  void _parseLine(std::string lineStr);

  bool parseCommentOrEmpty(std::string &lineStr);
  bool parsePackage(std::string &lineStr);
  bool parseComponent(std::string &lineStr);
  bool parsePosition(std::string &lineStr);
  bool parseConnection(std::string &lineStr);


  void genComponentInfoMap();
  ComponentInfo genComponentInfo(std::string componentName);
  FromToCoord getPackageExtents(std::string package);
  
  ComponentName2ComponentInfo componentName2ComponentInfo_;
  
  void genConnectionCoordVec();
  ConnectionCoordVec connectionCoordVec_;
  
//  CoordPair getComponentPinCoord(std::string componentName, u32 pinIdx);

//  string Parser::getComponentExtents(string component);
//  ComponentInfo getComponentInfo(std::string componentName);

  PackageToCoordMap packageToCoordMap_;
  ComponentName2PackageName componentName2PackageName_;
  ComponentToCoordMap componentToAbsCoordMap_;
  ConnectionPairVec connectionPairVec_;
};
