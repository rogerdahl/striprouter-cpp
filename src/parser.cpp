#include <fstream>
#include <iostream>
#include <regex>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include "parser.h"


using namespace std;
using namespace boost;


Parser::Parser()
{}

Parser::~Parser()
{}


void Parser::parse(Circuit& circuit) {
  circuit = Circuit();
  ifstream fin("./circuit.txt");
  string lineStr;
  u32 lineIdx = 0;
  while(getline(fin, lineStr)) {
    ++lineIdx;
    try {
      parseLine(circuit, lineStr);
    }
    catch (string s) {
      circuit.hasError = true;
      circuit.circuitInfoVec.push_back(fmt::format("Line {:n}: {}", lineIdx, lineStr));
      circuit.circuitInfoVec.push_back(fmt::format("Error: {}", s));
    }
  }
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
  else if (parseConnection(circuit, lineStr)) {
    return;
  }
  else {
    throw string("Invalid");
  }
}

//
// Private
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
  PackageRelCoordVec v;
  for (auto iter = sregex_token_iterator(lineStr.begin(), lineStr.end(), pkgRelativeCoordRx); iter != sregex_token_iterator(); ++iter) {
    string s = *iter;
    regex_match(s, m, pkgCoordValuesRx);
    v.push_back(Via(stoi(m[1]), stoi(m[2])));
  }
  circuit.packageToCoordMap[pkgName] = v;
  return true;
}

// Component
// com <component name> <package name> <absolute position of component pin 0>
bool Parser::parseComponent(Circuit& circuit, string &lineStr)
{
  regex comFull("^com\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s*,\\s*(\\d+)\\s*$",  regex_constants::ECMAScript | regex_constants::icase);
  smatch m;
  if (!regex_match(lineStr, m, comFull)) {
    return false;
  }
  if (circuit.packageToCoordMap.find(m[2]) == circuit.packageToCoordMap.end()) {
    throw fmt::format("Unknown package: {}", m[2].str());
  }
//  TODO: add check for out of bounds coord.
//    throw fmt::format("Invalid coordinates: {},{}", c.x, c.y);
  Component component(
    m[2],
    Via(static_cast<u32>(stoi(m[3])), static_cast<u32>(stoi(m[4])))
  );
  circuit.componentNameToInfoMap[m[1]] = component;
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
  ConnectionPoint start(m[1], static_cast<u32>(stoi(m[2]) - 1));
  ConnectionPoint end(m[3], static_cast<u32>(stoi(m[4]) - 1));
  auto startComponent = circuit.componentNameToInfoMap.find(start.componentName); 
  if (startComponent == circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", start.componentName);
  }
  auto endComponent = circuit.componentNameToInfoMap.find(end.componentName);
  if (endComponent == circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", end.componentName);
  }
  auto startCoordVec = circuit.packageToCoordMap.find(startComponent->second.packageName)->second;
  if (start.pinIdx >= startCoordVec.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", start.componentName, start.pinIdx, startCoordVec.size() - 1);
  }
  auto endCoordVec = circuit.packageToCoordMap.find(endComponent->second.packageName)->second;
  if (end.pinIdx >= endCoordVec.size()) {
    throw fmt::format("Invalid pin number for component {}: {}. Max: {}", end.componentName, end.pinIdx, endCoordVec.size() - 1);
  }
  circuit.connectionVec.push_back(Connection(start, end));
  return true;
}
