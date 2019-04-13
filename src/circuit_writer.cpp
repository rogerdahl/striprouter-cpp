#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>

#include <fmt/format.h>

#include "circuit_writer.h"
#include "utils.h"

void CircuitFileWriter::updateComponentPositions(
    const std::string circuitFilePath, const Circuit& circuit)
{
  //{
  // Can't open the file with ifstream while holding on to the lock, so
  // we release the lock after gaining it. Not ideal, but probably good
  // enough.
  auto fileLockScope = ExclusiveFileLock(circuitFilePath);
  //}

  std::ifstream inFile(circuitFilePath);
  if (!inFile.good()) {
    throw std::runtime_error(fmt::format(
        "Could not open file for read. path=\"{}\"", circuitFilePath));
  }

  auto tmpFilePath = circuitFilePath + ".tmp";
  std::ofstream outFile(tmpFilePath);
  if (!outFile.good()) {
    throw std::runtime_error(
        fmt::format("Could not open file for write. path=\"{}\"", tmpFilePath));
  }

  std::string lineStr;
  while (std::getline(inFile, lineStr)) {
    auto componentLine = parseComponentLine(lineStr);
    if (componentLine.isComponentLine) {
      auto componentInfo =
          circuit.componentNameToComponentMap.at(componentLine.componentName);
      outFile << fmt::format(
          "{}{}{}{}{}{}{},{}{}\n", componentLine.spaceVec[0],
          componentLine.componentName, componentLine.spaceVec[1],
          componentLine.packageName, componentLine.spaceVec[2],
          componentInfo.pin0AbsPos.x(), componentLine.spaceVec[3],
          componentLine.spaceVec[4], componentInfo.pin0AbsPos.y());
    }
    else {
      outFile << lineStr + "\n";
    }
  }

  fileLockScope.release();
  inFile.close();
  outFile.close();

  auto result = remove(circuitFilePath.c_str());
  if (result) {
    throw std::runtime_error(
        fmt::format("Could not delete old file. path=\"{}\"", circuitFilePath));
  }

  result = rename(tmpFilePath.c_str(), circuitFilePath.c_str());
  if (result) {
    throw std::runtime_error(fmt::format(
        "Could not replace file. new=\"{}\" old=\"{}\"", tmpFilePath,
        circuitFilePath));
  }
}

ComponentLine CircuitFileWriter::parseComponentLine(const std::string& lineStr)
{
  ComponentLine componentLine;
  componentLine.isComponentLine = false;
  static std::regex comFull(
      "^(\\s*)(\\w+)(\\s+)(\\w+)(\\s+)(\\d+)(\\s*),(\\s*)(\\d+)\\s*$",
      std::regex_constants::ECMAScript | std::regex_constants::icase);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return componentLine;
  }
  componentLine.isComponentLine = true;
  componentLine.spaceVec.push_back(m[1].str());
  componentLine.spaceVec.push_back(m[3].str());
  componentLine.spaceVec.push_back(m[5].str());
  componentLine.spaceVec.push_back(m[7].str());
  componentLine.spaceVec.push_back(m[8].str());
  componentLine.componentName = m[2].str();
  componentLine.packageName = m[4].str();
  return componentLine;
}
