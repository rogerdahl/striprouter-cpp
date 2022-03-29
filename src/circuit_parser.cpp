#include <fstream>
#include <iostream>
#include <regex>

#include <fmt/format.h>

#include "circuit_parser.h"
#include "utils.h"

const auto rxFlags =
    std::regex_constants::ECMAScript | std::regex_constants::icase;

CircuitFileParser::CircuitFileParser(Layout& _layout)
  : layout_(_layout), offset_(Via(0, 0))
{
}

CircuitFileParser::~CircuitFileParser()
{
}

void CircuitFileParser::parse(std::string& circuitFilePath)
{
  auto fileLockScope = ExclusiveFileLock(circuitFilePath);
  std::ifstream fin(circuitFilePath);
  if (!fin.good()) {
    layout_.circuit.parserErrorVec.push_back(
        fmt::format("Cannot read .circuit file: {}", circuitFilePath));
    return;
  }
  std::string lineStr;
  int lineIdx = 0;
  while (std::getline(fin, lineStr)) {
    ++lineIdx;
    std::regex stripMultipleWhitespace("\\s+");
    lineStr = trim(std::regex_replace(lineStr, stripMultipleWhitespace, " "));
    std::regex stripCommaWhitespace("\\s*,\\s*");
    lineStr = trim(std::regex_replace(lineStr, stripCommaWhitespace, ","));
    try {
      parseLine(lineStr);
    } catch (std::string errorStr) {
      layout_.circuit.parserErrorVec.push_back(
          fmt::format("Error on line {:n}: {}: {}", lineIdx, lineStr, errorStr));
    }
  }
  layout_.isReadyForRouting = !layout_.circuit.hasParserError();
}

//
// Private
//

void CircuitFileParser::parseLine(std::string lineStr)
{
  // Substitute aliases
  std::string tmp;
  for (auto it = begin(aliases_); it != end(aliases_); ++it) {
    tmp = std::regex_replace(lineStr, std::regex(it->first), it->second);
    if (lineStr.compare(tmp) != 0) {
      lineStr = tmp;
    }
  }

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
  else if (parseDontCare(lineStr)) {
    return;
  }
  else if (parseAlias(lineStr)) {
  }
  else {
    throw std::string("Invalid line");
  }
}

// Alias
bool CircuitFileParser::parseAlias(std::string& lineStr)
{
  static std::regex AliasRx("^([\\w.]+) = ([\\w.]+)$", rxFlags);
  std::smatch m;
  if (!regex_match(lineStr, m, AliasRx)) {
    return false;
  }
  aliases_.push_back(std::make_pair(m[1], m[2]));
  return true;
}

// Comment or empty line
bool CircuitFileParser::parseCommentOrEmpty(std::string& lineStr)
{
  static std::regex commentOrEmptyFull("^(#.*)?$", rxFlags);
  return std::regex_match(lineStr, commentOrEmptyFull);
}

// Board params (currently just size)
// board <number of horizontal vias>,<number of vertical vias>
bool CircuitFileParser::parseBoard(std::string& lineStr)
{
  static std::regex boardSizeRx("^board (\\d+),(\\d+)$", rxFlags);
  std::smatch m;
  if (!regex_match(lineStr, m, boardSizeRx)) {
    return false;
  }
  layout_.gridW = stoi(m[1]);
  layout_.gridH = stoi(m[2]);
  return true;
}

// Component position offset. Can be used multiple times to adjust section of
// circuit. Adds the given offset to the positions of components defined below
// in the .circuit file. To disable, set to 0,0.
// offset <relative x pos>, <relative y pos>
bool CircuitFileParser::parseOffset(std::string& lineStr)
{
  static std::regex offsetRx("^offset (-?\\d+),(-?\\d+)$", rxFlags);
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
bool CircuitFileParser::parsePackage(std::string& lineStr)
{
  static std::regex pkgNameRx("^(\\w+)\\s(.*)", rxFlags);
  static std::regex pkgSepRx("\\s+", rxFlags);
  static std::regex pkgPosRx("(-?\\d+),(-?\\d+)", rxFlags);
  std::smatch m;
  if (!regex_match(lineStr, m, pkgNameRx)) {
    return false;
  }
  std::string pkgName = m[1];
  std::string pkgPos = m[2];
  PackageRelPosVec v;
  for (auto iter = std::sregex_token_iterator(
           pkgPos.begin(), pkgPos.end(), pkgSepRx, -1);
       iter != std::sregex_token_iterator(); ++iter) {
    std::string s = *iter;
    if (regex_match(s, m, pkgPosRx)) {
      v.push_back(Via(stoi(m[1]), stoi(m[2])));
    }
    else {
      return false;
    }
  }
  layout_.circuit.packageToPosMap[pkgName] = v;
  return true;
}

// Component
// <component name> <package name> <absolute position of component pin 0>
bool CircuitFileParser::parseComponent(std::string& lineStr)
{
  static std::regex comFull("^(\\w+) (\\w+) ?(\\d+),(\\d+)$", rxFlags);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return false;
  }
  auto componentName = m[1].str();
  auto packageName = m[2].str();
  auto x = std::stoi(m[3]);
  auto y = std::stoi(m[4]);
  if (layout_.circuit.packageToPosMap.find(packageName)
      == layout_.circuit.packageToPosMap.end()) {
    throw fmt::format("Unknown package: {}", packageName);
  }
  Via p = Via(x, y) + offset_;
  auto i = 0;
  for (auto& v : layout_.circuit.packageToPosMap[packageName]) {
    if (p.x() + v.x() < 0 || p.x() + v.x() >= layout_.gridW || p.y() + v.y() < 0
        || p.y() + v.y() >= layout_.gridH) {
      throw fmt::format(
          "Component pin outside of board: {}.{}", componentName, i + 1);
    }
    ++i;
  }
  Component component(packageName, p);
  layout_.circuit.componentNameToComponentMap[m[1]] = component;
  return true;
}

// Don't Care pins
// <component name> <list of pin indexes>
bool CircuitFileParser::parseDontCare(std::string& lineStr)
{
  static std::regex dontCareFullRx("^(\\w+) (\\d+(,|$))+", rxFlags);
  static std::regex dontCarePinIdxRx("(\\d+)(,|$)", rxFlags);
  std::smatch m;
  if (!regex_match(lineStr, m, dontCareFullRx)) {
    return false;
  }
  auto componentName = m[1].str();
  auto componentItr =
      layout_.circuit.componentNameToComponentMap.find(componentName);
  if (componentItr == layout_.circuit.componentNameToComponentMap.end()) {
    throw fmt::format("Unknown component: {}", componentName);
  }
  auto& component = componentItr->second;
  auto packagePosVec =
      layout_.circuit.packageToPosMap.find(component.packageName)->second;
  for (auto iter = std::sregex_token_iterator(
           lineStr.begin(), lineStr.end(), dontCarePinIdxRx);
       iter != std::sregex_token_iterator(); ++iter) {
    std::string s = *iter;
    regex_match(s, m, dontCarePinIdxRx);
    auto dontCarePinIdx = stoi(m[1]);
    if (dontCarePinIdx < 1
        || dontCarePinIdx > static_cast<int>(packagePosVec.size())) {
      throw fmt::format(
          "Invalid \"Don't Care\" pin number for {}: {}. Must be between 1 "
          "and "
          "{} (including)",
          componentName, dontCarePinIdx, packagePosVec.size());
    }
    component.dontCarePinIdxSet.insert(--dontCarePinIdx);
  }
  return true;
}

// Connection
// 7400.9 rpi.10
bool CircuitFileParser::parseConnection(std::string& lineStr)
{
  static std::regex comFull("^(\\w+)\\.(\\d+) (\\w+)\\.(\\d+)$", rxFlags);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return false;
  }
  ConnectionPoint start(m[1], std::stoi(m[2]) - 1);
  ConnectionPoint end(m[3], std::stoi(m[4]) - 1);
  checkConnectionPoint(start);
  checkConnectionPoint(end);
  if (start.componentName == end.componentName && start.pinIdx == end.pinIdx) {
    return true;
  }
  layout_.circuit.connectionVec.push_back(Connection(start, end));
  return true;
}

void CircuitFileParser::checkConnectionPoint(
    const ConnectionPoint& connectionPoint)
{
  auto componentItr = layout_.circuit.componentNameToComponentMap.find(
      connectionPoint.componentName);
  if (componentItr == layout_.circuit.componentNameToComponentMap.end()) {
    throw fmt::format("Unknown component: {}", connectionPoint.componentName);
  }
  auto component = componentItr->second;
  auto packagePosVec =
      layout_.circuit.packageToPosMap.find(component.packageName)->second;
  auto pinIdx1Base = connectionPoint.pinIdx + 1;
  if (pinIdx1Base < 1 || pinIdx1Base > static_cast<int>(packagePosVec.size())) {
    throw fmt::format(
        "Invalid pin number for {}.{}. Must be between 1 and {} (including)",
        connectionPoint.componentName, pinIdx1Base, packagePosVec.size());
  }
  if (component.dontCarePinIdxSet.count(connectionPoint.pinIdx)) {
    throw fmt::format(
        "Invalid pin number for {}.{}. Pin has been set as \"Don't Care\"",
        connectionPoint.componentName, pinIdx1Base, packagePosVec.size());
  }
}
