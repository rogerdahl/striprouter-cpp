#include <fstream>
#include <iostream>
#include <regex>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include "parser.h"


Parser::Parser(Solution& _solution)
: solution_(_solution)
{
}

Parser::~Parser()
{
}

void Parser::parse()
{
  std::ifstream fin("./circuit.txt");
  std::string lineStr;
  int lineIdx = 0;
  while (std::getline(fin, lineStr)) {
    ++lineIdx;
    try {
      parseLine(lineStr);
    }
    catch (std::string s) {
      solution_.circuit.hasError = true;
      solution_.circuit.circuitInfoVec
        .push_back(fmt::format("Line {:n}: {}", lineIdx, lineStr));
      solution_.circuit.circuitInfoVec.push_back(fmt::format("Error: {}", s));
    }
  }
  solution_.isReady = true;
}

void Parser::parseLine(std::string lineStr)
{
  if (parseCommentOrEmpty(lineStr)) {
    return;
  }
  else if (parseBoard(lineStr)) {
    return;
  }
  else if (parsePackage(lineStr)) {
    return;
  }
  else if (parseComponent(lineStr)) {
    return;
  }
  else if (parseConnection(lineStr)) {
    return;
  }
  else {
    throw std::string("Invalid line");
  }
}

//
// Private
//

// Comment or empty
bool Parser::parseCommentOrEmpty(std::string &lineStr)
{
  std::regex commentOrEmptyFull("^(#.*)?$",
                                std::regex_constants::ECMAScript
                                  | std::regex_constants::icase);
  return std::regex_match(lineStr, commentOrEmptyFull);
}

// Board params (currently just size)
bool Parser::parseBoard(std::string &lineStr)
{
  std::regex boardSizeRx("^\\s*board\\s+(\\d+)\\s*,\\s*(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!regex_match(lineStr, m, boardSizeRx)) {
    return false;
  }
  solution_.gridW = stoi(m[1]);
  solution_.gridH = stoi(m[2]);
  return true;
}

// Package
// pkg dip8 0,0 1,0 2,0 3,0 4,0 5,0 6,0 7,0 7,-2 6,-2 5,-2 4,-2 3,-2 2,-2 1,-2
bool Parser::parsePackage(std::string &lineStr)
{
  std::regex pkgFull("^\\s*(\\w+)(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))+$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::regex pkgRelativePosRx("(\\s+(-?\\d+)\\s*,\\s*(-?\\d+))",
                              std::regex_constants::ECMAScript
                                | std::regex_constants::icase);
  std::regex pkgPosValuesRx("\\s+(-?\\d+)\\s*,\\s*(-?\\d+)",
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
  solution_.circuit.packageToPosMap[pkgName] = v;
  return true;
}

// Component
// com <component name> <package name> <absolute position of component pin 0>
bool Parser::parseComponent(std::string &lineStr)
{
  std::regex comFull("^\\s*(\\w+)\\s+(\\w+)\\s+(\\d+)\\s*,\\s*(\\d+)\\s*$",
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
  if (solution_.circuit.packageToPosMap.find(packageName) == solution_.circuit.packageToPosMap.end()) {
    throw fmt::format("Unknown package: {}", packageName);
  }
  Via p(x, y);
  auto i = 0;
  for (auto& v : solution_.circuit.packageToPosMap[packageName]) {
    if (p.x() + v.x() < 0 || p.x() + v.x() >= solution_.gridW || p.y() + v.y() < 0 || p.y() + v.y() >= solution_.gridH) {
      throw fmt::format("Component pin outside of board: {}.{}", componentName, i + 1);
    }
    ++i;
  }
  Component component(packageName, p);
  solution_.circuit.componentNameToInfoMap[m[1]] = component;
  return true;
}

// Connection
// 7400.9 rpi.10
bool Parser::parseConnection(std::string &lineStr)
{
  std::regex comFull("^\\s*(\\w+)\\.(\\d+)\\s*(\\w+)\\.(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return false;
  }
  ConnectionPoint start(m[1], std::stoi(m[2]));
  ConnectionPoint end(m[3], std::stoi(m[4]));
  auto
    startComponent = solution_.circuit.componentNameToInfoMap.find(start.componentName);
  if (startComponent == solution_.circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", start.componentName);
  }
  auto endComponent = solution_.circuit.componentNameToInfoMap.find(end.componentName);
  if (endComponent == solution_.circuit.componentNameToInfoMap.end()) {
    throw fmt::format("Unknown component: {}", end.componentName);
  }
  auto startPosVec =
    solution_.circuit.packageToPosMap.find(startComponent->second.packageName)->second;
  if (start.pinIdx < 1 || start.pinIdx > static_cast<int>(startPosVec.size())) {
    throw fmt::format(
      "Invalid pin number for {}.{}. Must be between 1 and {} (including)",
      start.componentName,
      start.pinIdx,
      startPosVec.size());
  }
  auto endPosVec =
    solution_.circuit.packageToPosMap.find(endComponent->second.packageName)->second;
  if (end.pinIdx < 1 || end.pinIdx > static_cast<int>(endPosVec.size())) {
    throw fmt::format(
      "Invalid pin number for {}.{}. Must be between 1 and {} (including)",
      end.componentName,
      end.pinIdx,
      endPosVec.size());
  }
  --start.pinIdx;
  --end.pinIdx;
  solution_.circuit.connectionVec.push_back(Connection(start, end));
  return true;
}
