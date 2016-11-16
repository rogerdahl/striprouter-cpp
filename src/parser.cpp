#include "parser.h"

#include <regex>

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <string>

#include <fmt/format.h>

using namespace std;
using namespace boost;


Parser::Parser() {}

Parser::~Parser() {}


Circuit Parser::parse() {
  Circuit circuit;
  std::ifstream fin("./circuit.txt");
  std::string lineStr;
  u32 lineIdx = 0;
  while(std::getline(fin, lineStr)) {
    ++lineIdx;
    try {
      parseLine(circuit, lineStr);
    }
    catch (string s) {
      circuit.setErrorBool(true);
      circuit.getCircuitInfoVec().push_back(fmt::format("Line {}: {}", lineIdx, lineStr));
      circuit.getCircuitInfoVec().push_back(fmt::format("Error: {}", s));
    }
  }
  genComponentInfoMap(circuit);
  genConnectionCoordVec(circuit);
  return circuit;
}


void Parser::parseLine(Circuit& circuit, string lineStr)
{
  if (parseCommentOrEmpty(circuit, lineStr)) {
    return;
  }
  else if (parsePackage(circuit, lineStr)) {
    return;
  }
  else if (parseComponent(circuit, lineStr)) {
    return;
  }
  else if (parsePosition(circuit, lineStr)) {
    return;
  }
  else if (parseConnection(circuit, lineStr)) {
    return;
  }
  else {
    throw string("Could not parse");
  }
}


//
// Private
//


void Parser::genComponentInfoMap(Circuit& circuit)
{
  for (auto componentName : circuit.getComponentNameVec()) {
    circuit.getComponentInfoMap()[componentName] = genComponentInfo(circuit, componentName);
  }
}


ComponentInfo Parser::genComponentInfo(Circuit& circuit, string componentName)
{
  ComponentInfo ci;
  auto packageName = circuit.getComponentName2PackageName().find(componentName)->second;
  ci.componentName = componentName;
  auto componentAbsCoord = circuit.getComponentToCoordMap()[componentName];
  StartEndCoord fc = calcPackageExtents(circuit, packageName);
  fc.start.x += componentAbsCoord.x;
  fc.start.y += componentAbsCoord.y;
  fc.end.x += componentAbsCoord.x;
  fc.end.y += componentAbsCoord.y;
  ci.footprint = fc;
  ci.pin0AbsCoord.x = componentAbsCoord.x;
  ci.pin0AbsCoord.y = componentAbsCoord.y;

  auto packageCoordVec = circuit.getPackageToCoordMap().find(packageName)->second;

  for (auto pc : packageCoordVec) {
    auto h = HoleCoord(componentAbsCoord.x + pc.x, componentAbsCoord.y + pc.y);
    ci.pinAbsCoordVec.push_back(h);
  }

  return ci;
}


StartEndCoord Parser::calcPackageExtents(Circuit& circuit, string package)
{
  CoordVec v = circuit.getPackageToCoordMap().find(package)->second;
  StartEndCoord fc;
  fc.start.x = 0;
  fc.start.y = 0;
  fc.end.x = 0;
  fc.end.y = 0;
  for (auto c : v) {
    if (c.x < fc.start.x) {
      fc.start.x = c.x;
    }
    if (c.x > fc.end.x) {
      fc.end.x = c.x;
    }
    if (c.y < fc.start.y) {
      fc.start.y = c.y;
    }
    if (c.y > fc.end.y) {
      fc.end.y = c.y;
    }
  }
  return fc;
}


void Parser::genConnectionCoordVec(Circuit& circuit)
{
  for (auto c : circuit.getConnectionPairVec()) {
    auto componentPinPair1 = c.first;
    auto componentPinPair2 = c.second;

    string componentName1 = componentPinPair1.first;
    string componentName2 = componentPinPair2.first;

    u32 componentPinIdx1 = componentPinPair1.second;
    u32 componentPinIdx2 = componentPinPair2.second;

    auto componentInfo1 = circuit.getComponentInfoMap().find(componentName1)->second;
    auto componentInfo2 = circuit.getComponentInfoMap().find(componentName2)->second;

    StartEndCoord ft;

//    ft.x1 = componentInfo1.pinAbsCoordVec[componentPinIdx1].first;
//    ft.y1 = componentInfo1.pinAbsCoordVec[componentPinIdx1].second;
//    ft.x2 = componentInfo2.pinAbsCoordVec[componentPinIdx2].first;
//    ft.y2 = componentInfo2.pinAbsCoordVec[componentPinIdx2].second;

    ft.start.x = componentInfo1.pinAbsCoordVec[componentPinIdx1].x;
    ft.end.x = componentInfo2.pinAbsCoordVec[componentPinIdx2].x;
    
    // Move connections to the closest vertical point outside the package footprint.
    
    ft.start.y = componentInfo1.pinAbsCoordVec[componentPinIdx1].y;
    float y_half = componentInfo1.footprint.start.y + (componentInfo1.footprint.end.y - componentInfo1.footprint.start.y) / 2.0f;
    if (ft.start.y < y_half) {
      ft.start.y = componentInfo1.footprint.start.y - 1;
    }
    else {
      ft.start.y = componentInfo1.footprint.end.y + 1;
    }

    ft.end.y = componentInfo2.pinAbsCoordVec[componentPinIdx2].y;
    y_half = componentInfo2.footprint.start.y + (componentInfo2.footprint.end.y - componentInfo2.footprint.start.y) / 2.0f;
    if (ft.end.y < y_half) {
      ft.end.y = componentInfo2.footprint.start.y - 1;
    }
    else {
      ft.end.y = componentInfo2.footprint.end.y + 1;
    }

    circuit.getConnectionCoordVec().push_back(ft);
  }
}

//
// Parse
//

// Comment or empty
bool Parser::parseCommentOrEmpty(Circuit& circuit, string &lineStr)
{
  regex commentOrEmptyFull("^(#.*)?$", regex_constants::ECMAScript | regex_constants::icase);
  return regex_match(lineStr, commentOrEmptyFull);
}

// Package
// pkg dip8 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 7,-2 6,-2 5,-2 4,-2 3,-2 2,-2 1,-2
bool Parser::parsePackage(Circuit& circuit, string &lineStr)
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
    HoleCoord h(stoi(m[1]), stoi(m[2]));
    v.push_back(h);
  }
  circuit.getPackageToCoordMap()[pkgName] = v;
  return true;
}

// Component
// com rpi header20
bool Parser::parseComponent(Circuit& circuit, string &lineStr)
{
  regex comFull("^com\\s+(\\w+)\\s+(\\w+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  if (circuit.getPackageToCoordMap().find(m[2]) == circuit.getPackageToCoordMap().end()) {
    throw fmt::format("Unknown package: {}", m[2].str());
  }
  circuit.getComponentName2PackageName()[m[1]] = m[2];
  return true;
}

// Position
// pos rpi 4,4
bool Parser::parsePosition(Circuit& circuit, string &lineStr)
{
  regex comFull("^pos\\s+(\\w+)\\s+(-?\\d+)\\s*,\\s*(-?\\d+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  HoleCoord c(stoi(m[2]), stoi(m[3]));
  if (c.x < 0 || c.x > 1000 || c.y < 0 || c.y > 1000) {
    throw fmt::format("Invalid coordinates: {},{}", c.x, c.y);
  }
  circuit.getComponentToCoordMap()[m[1]] = c;
  return true;
}

// Connection
// 7400.9 rpi.10
bool Parser::parseConnection(Circuit& circuit, string &lineStr)
{
  regex comFull("^c\\s+(\\w+)\\.(\\d+)\\s*(\\w+)\\.(\\d+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  ComponentPinPair p1(m[1], static_cast<u32>(stoi(m[2]) - 1));
  ComponentPinPair p2(m[3], static_cast<u32>(stoi(m[4]) - 1));

  if (circuit.getComponentName2PackageName().find(p1.first) == circuit.getComponentName2PackageName().end()) {
    throw fmt::format("Unknown component: {}", p1.first);
  }
  if (circuit.getComponentName2PackageName().find(p2.first) == circuit.getComponentName2PackageName().end()) {
    throw fmt::format("Unknown component: {}", p2.first);
  }

  auto packageName1 = circuit.getComponentName2PackageName().find(p1.first)->second;
  auto coordVec1 = circuit.getPackageToCoordMap().find(packageName1)->second;
  if (p1.second >= coordVec1.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", p1.first, p1.second, coordVec1.size() - 1);
  }
  auto packageName2 = circuit.getComponentName2PackageName().find(p2.first)->second;
  auto coordVec2 = circuit.getPackageToCoordMap().find(packageName2)->second;
  if (p2.second >= coordVec2.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", p2.first, p2.second, coordVec2.size() - 1);
  }

  ConnectionPair c(p1, p2);
  circuit.getConnectionPairVec().push_back(c);
  return true;
}
