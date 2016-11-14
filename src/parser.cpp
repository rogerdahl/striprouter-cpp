#include "parser.h"

#include <regex>

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <string>

#include <fmt/format.h>

using namespace std;
using namespace boost;


Parser::Parser()  {}

Parser::~Parser() {}


LineErrorPair Parser::parse() {
  std::ifstream fin("./circuit.txt");
  std::string lineStr;
  u32 lineIdx = 0;
  while(std::getline(fin, lineStr)) {
    ++lineIdx;
    try {
      _parseLine(lineStr);
    }
    catch (string s) {
      return LineErrorPair(
        fmt::format("Line {}: {}", lineIdx, lineStr),
        fmt::format("Error: {}", s)
      );
    }
  }

  genComponentInfoMap();
  genConnectionCoordVec();

  return LineErrorPair(
      fmt::format("Lines: {}", lineIdx),
      "Ok"
  );
}


void Parser::_parseLine(string lineStr)
{
  if (parseCommentOrEmpty(lineStr)) {
    return;
  }
  if (parsePackage(lineStr)) {
    return;
  }
  else if (parseComponent(lineStr)) {
    return;
  }
  else if (parsePosition(lineStr)) {
    return;
  }
  else if (parseConnection(lineStr)) {
    return;
  }
  throw string("Could not parse");
}


ComponentNameVec Parser::getComponentNameVec()
{
  ComponentNameVec v;
  for (auto n : componentName2PackageName_) {
    v.push_back(n.first);
  }
  return v;
}


ConnectionCoordVec Parser::getConnectionCoordVec()
{
  return connectionCoordVec_;
}


ComponentInfo Parser::getComponentInfo(string componentName)
{
  return componentName2ComponentInfo_.find(componentName)->second;
}

//
// Private
//


void Parser::genComponentInfoMap()
{
  for (auto componentName : getComponentNameVec()) {
    componentName2ComponentInfo_[componentName] = genComponentInfo(componentName);
  }
}


ComponentInfo Parser::genComponentInfo(string componentName)
{
  ComponentInfo ci;
  auto packageName = componentName2PackageName_.find(componentName)->second;
  ci.componentName = componentName;
  auto componentAbsCoord = componentToAbsCoordMap_[componentName];
  FromToCoord fc = getPackageExtents(packageName);
  fc.x1 += componentAbsCoord.first;
  fc.y1 += componentAbsCoord.second;
  fc.x2 += componentAbsCoord.first;
  fc.y2 += componentAbsCoord.second;
  ci.extent = fc;
  ci.pin0AbsCoord.first = componentAbsCoord.first;
  ci.pin0AbsCoord.second = componentAbsCoord.second;

  auto packageCoordVec = packageToCoordMap_.find(packageName)->second;

  for (auto pc : packageCoordVec) {
    ci.pinAbsCoordVec.push_back(CoordPair(
        componentAbsCoord.first + pc.first,
        componentAbsCoord.second + pc.second
    ));
  }

  return ci;
}


FromToCoord Parser::getPackageExtents(string package)
{
  CoordVec v = packageToCoordMap_.find(package)->second;
  FromToCoord fc;
  fc.x1 = 0;
  fc.y1 = 0;
  fc.x2 = 0;
  fc.y2 = 0;
  for (auto c : v) {
    if (c.first < fc.x1) {
      fc.x1 = c.first;
    }
    if (c.first > fc.x2) {
      fc.x2 = c.first;
    }
    if (c.second < fc.y1) {
      fc.y1 = c.second;
    }
    if (c.second > fc.y2) {
      fc.y2 = c.second;
    }
  }
  return fc;
}


void Parser::genConnectionCoordVec()
{
  for (auto c : connectionPairVec_) {
    auto componentPinPair1 = c.first;
    auto componentPinPair2 = c.second;

    string componentName1 = componentPinPair1.first;
    string componentName2 = componentPinPair2.first;

    u32 componentPinIdx1 = componentPinPair1.second;
    u32 componentPinIdx2 = componentPinPair2.second;

    auto componentInfo1 = getComponentInfo(componentName1);
    auto componentInfo2 = getComponentInfo(componentName2);

    FromToCoord ft;

    ft.x1 = componentInfo1.pinAbsCoordVec[componentPinIdx1].first;
    ft.y1 = componentInfo1.pinAbsCoordVec[componentPinIdx1].second;
    ft.x2 = componentInfo2.pinAbsCoordVec[componentPinIdx2].first;
    ft.y2 = componentInfo2.pinAbsCoordVec[componentPinIdx2].second;
    
    connectionCoordVec_.push_back(ft);
  }
}


void Parser::dump(FromToCoord &fc)
{
  fmt::print("{},{} - {},{}\n", fc.x1, fc.y1, fc.x2, fc.y2);
}

//
// Parse
//

// Comment or empty
bool Parser::parseCommentOrEmpty(string &lineStr)
{
  regex commentOrEmptyFull("^(#.*)?$", regex_constants::ECMAScript | regex_constants::icase);
  return regex_match(lineStr, commentOrEmptyFull);
}

// Package
// pkg dip8 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 7,-2 6,-2 5,-2 4,-2 3,-2 2,-2 1,-2
bool Parser::parsePackage(string &lineStr)
{
  regex pkgFull("^pkg\\s+(\\w+)(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))+$",  regex_constants::ECMAScript | regex_constants::icase);
  regex pkgRelativeCoordRx("(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))",  regex_constants::ECMAScript | regex_constants::icase);
  regex pkgCoordValuesRx("\\s+(-?\\d+)\\s*,\\s*(-?\\d+)", regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, pkgFull)) {
    return false;
  }
  string pkgName = m[1];
  CoordVec v;
  for (auto iter = sregex_token_iterator(lineStr.begin(), lineStr.end(), pkgRelativeCoordRx); iter != sregex_token_iterator(); ++iter) {
    string s(*iter);
    regex_match(s, m, pkgCoordValuesRx);
    CoordPair c(stoi(m[1]), stoi(m[2]));
    v.push_back(c);
  }
  packageToCoordMap_[pkgName] = v;
  return true;
}

// Component
// com rpi header20
bool Parser::parseComponent(string &lineStr)
{
  regex comFull("^com\\s+(\\w+)\\s+(\\w+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  if (packageToCoordMap_.find(m[2]) == packageToCoordMap_.end()) {
    throw fmt::format("Unknown package: {}", m[2].str());
  }
  componentName2PackageName_[m[1]] = m[2];
  return true;
}

// Position
// pos rpi 4,4
bool Parser::parsePosition(string &lineStr)
{
  regex comFull("^pos\\s+(\\w+)\\s+(-?\\d+)\\s*,\\s*(-?\\d+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  CoordPair c(stoi(m[2]), stoi(m[3]));
  if (c.first < 0 || c.first > 1000 || c.second < 0 || c.second > 1000) {
    throw fmt::format("Invalid coordinates: {},{}", c.first, c.second);
  }
  componentToAbsCoordMap_[m[1]] = c;
  return true;
}

// Connection
// 7400.9 rpi.10
// Positions
// pos rpi 4,4
bool Parser::parseConnection(string &lineStr)
{
  regex comFull("^c\\s+(\\w+)\\.(\\d+)\\s*(\\w+)\\.(\\d+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  ComponentPinPair p1(m[1], static_cast<u32>(stoi(m[2])));
  ComponentPinPair p2(m[3], static_cast<u32>(stoi(m[4])));

  if (componentName2PackageName_.find(p1.first) == componentName2PackageName_.end()) {
    throw fmt::format("Unknown component: {}", p1.first);
  }
  if (componentName2PackageName_.find(p2.first) == componentName2PackageName_.end()) {
    throw fmt::format("Unknown component: {}", p2.first);
  }

  auto packageName1 = componentName2PackageName_.find(p1.first)->second;
  auto coordVec1 = packageToCoordMap_.find(packageName1)->second;
  if (p1.second >= coordVec1.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", p1.first, p1.second, coordVec1.size() - 1);
  }
  auto packageName2 = componentName2PackageName_.find(p2.first)->second;
  auto coordVec2 = packageToCoordMap_.find(packageName2)->second;
  if (p2.second >= coordVec2.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", p2.first, p2.second, coordVec2.size() - 1);
  }

  ConnectionPair c(p1, p2);
  connectionPairVec_.push_back(c);
  return true;
}
