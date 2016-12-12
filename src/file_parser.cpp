#include <fstream>
#include <iostream>
#include <regex>

#include <fmt/format.h>

#include "file_parser.h"
#include "utils.h"


CircuitFileParser::CircuitFileParser(Layout& _layout)
: layout_(_layout), offset_(Via(0,0))
{
}

CircuitFileParser::~CircuitFileParser()
{
}

void CircuitFileParser::parse(std::string& circuitFilePath)
{
  auto fd = getExclusiveLock(circuitFilePath);
  std::ifstream fin(circuitFilePath);
  std::string lineStr;
  int lineIdx = 0;
  while (std::getline(fin, lineStr)) {
    ++lineIdx;
    try {
      parseLine(lineStr);
    }
    catch (std::string s) {
      layout_.circuit.parserErrorVec
        .push_back(fmt::format("Line {:n}: {}", lineIdx, lineStr));
      layout_.circuit.parserErrorVec.push_back(fmt::format("Error: {}", s));
    }
  }
  layout_.isReadyForRouting = !layout_.circuit.hasParserError();
  releaseExclusiveLock(fd);
}

//
// Private
//

void CircuitFileParser::parseLine(std::string lineStr)
{
  // Connections are most common, so they are parsed first to improve
  // performance.
  if (parseConnection(lineStr)) {
    return;
  }
  else if (parseCommentOrEmpty(lineStr)) {
    return;
  }
  else if (parseBoard(lineStr)) {
    return;
  }
  else if (parseOffset(lineStr)) {
    return;
  }
  else if (parsePackage(lineStr)) {
    return;
  }
  else if (parseComponent(lineStr)) {
    return;
  }
  else {
    throw std::string("Invalid line");
  }
}


// Comment or empty
bool CircuitFileParser::parseCommentOrEmpty(std::string &lineStr)
{
  static std::regex commentOrEmptyFull("^(#.*)?$",
                                std::regex_constants::ECMAScript
                                  | std::regex_constants::icase);
  return std::regex_match(lineStr, commentOrEmptyFull);
}

// Board params (currently just size)
bool CircuitFileParser::parseBoard(std::string &lineStr)
{
  static std::regex boardSizeRx("^\\s*board\\s+(\\d+)\\s*,\\s*(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!regex_match(lineStr, m, boardSizeRx)) {
    return false;
  }
  layout_.gridW = stoi(m[1]);
  layout_.gridH = stoi(m[2]);
  return true;
}

// Component position offset. Can be used multiple times to adjust section of circuit.
bool CircuitFileParser::parseOffset(std::string &lineStr)
{
  static std::regex offsetRx("^\\s*offset\\s+(-?\\d+)\\s*,\\s*(-?\\d+)\\s*$",
                         std::regex_constants::ECMAScript
                           | std::regex_constants::icase);
  std::smatch m;
  if (!regex_match(lineStr, m, offsetRx)) {
    return false;
  }
  offset_.x() = stoi(m[1]);
  offset_.y() = stoi(m[2]);
  return true;
}

// Package
// dip8 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 7,-2 6,-2 5,-2 4,-2 3,-2 2,-2 1,-2
bool CircuitFileParser::parsePackage(std::string &lineStr)
{
  static std::regex pkgFull("^\\s*(\\w+)(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))+$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  static std::regex pkgRelativePosRx("(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))",
                              std::regex_constants::ECMAScript
                                | std::regex_constants::icase);
  static std::regex pkgPosValuesRx("\\s+(-?\\d+)\\s*,\\s*(-?\\d+)",
                            std::regex_constants::ECMAScript
                              | std::regex_constants::icase);
  std::smatch m;
  if (!regex_match(lineStr, m, pkgFull)) {
    return false;
  }
  std::string pkgName = m[1];
  PackageRelPosVec v;
  for (auto iter = std::sregex_token_iterator(lineStr.begin(),
                                              lineStr.end(),
                                              pkgRelativePosRx);
       iter != std::sregex_token_iterator(); ++iter) {
    std::string s = *iter;
    regex_match(s, m, pkgPosValuesRx);
    v.push_back(Via(stoi(m[1]), stoi(m[2])));
  }
  layout_.circuit.packageToPosMap[pkgName] = v;
  return true;
}

// Component
// <component name> <package name> <absolute position of component pin 0>
bool CircuitFileParser::parseComponent(std::string &lineStr)
{
  static std::regex comFull("^\\s*(\\w+)\\s+(\\w+)\\s+(\\d+)\\s*,\\s*(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return false;
  }
  auto componentName = m[1].str();
  auto packageName = m[2].str();
  auto x = std::stoi(m[3]);
  auto y = std::stoi(m[4]);
  if (layout_.circuit.packageToPosMap.find(packageName) == layout_.circuit.packageToPosMap.end()) {
    throw fmt::format("Unknown package: {}", packageName);
  }
  Via p = Via(x, y) + offset_;
  auto i = 0;
  for (auto& v : layout_.circuit.packageToPosMap[packageName]) {
    if (p.x() + v.x() < 0 || p.x() + v.x() >= layout_.gridW || p.y() + v.y() < 0 || p.y() + v.y() >= layout_.gridH) {
      throw fmt::format("Component pin outside of board: {}.{}", componentName, i + 1);
    }
    ++i;
  }
  Component component(packageName, p);
  layout_.circuit.componentNameToInfoMap[m[1]] = component;
  return true;
}

// Connection
// 7400.9 rpi.10
bool CircuitFileParser::parseConnection(std::string &lineStr)
{
  static std::regex comFull("^\\s*(\\w+)\\.(\\d+)\\s*(\\w+)\\.(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return false;
  }
  ConnectionPoint start(m[1], std::stoi(m[2]));
  ConnectionPoint end(m[3], std::stoi(m[4]));
  auto
    startComponent = layout_.circuit.componentNameToInfoMap.find(start.componentName);
  if (startComponent == layout_.circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", start.componentName);
  }
  auto endComponent = layout_.circuit.componentNameToInfoMap.find(end.componentName);
  if (endComponent == layout_.circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", end.componentName);
  }
  auto startPosVec =
    layout_.circuit.packageToPosMap.find(startComponent->second.packageName)->second;
  if (start.pinIdx < 1 || start.pinIdx > static_cast<int>(startPosVec.size())) {
    throw fmt::format(
      "Invalid pin number for {}.{}. Must be between 1 and {} (including)",
      start.componentName,
      start.pinIdx,
      startPosVec.size());
  }
  auto endPosVec =
    layout_.circuit.packageToPosMap.find(endComponent->second.packageName)->second;
  if (end.pinIdx < 1 || end.pinIdx > static_cast<int>(endPosVec.size())) {
    throw fmt::format(
      "Invalid pin number for {}.{}. Must be between 1 and {} (including)",
      end.componentName,
      end.pinIdx,
      endPosVec.size());
  }

  if (start.componentName == end.componentName && start.pinIdx == end.pinIdx) {
    return true;
  }

  --start.pinIdx;
  --end.pinIdx;
  layout_.circuit.connectionVec.push_back(Connection(start, end));
  return true;
}
