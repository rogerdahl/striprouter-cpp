#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>

#include <fmt/format.h>

#include "file_writer.h"
#include "utils.h"


void CircuitFileWriter::updateComponentPosition(const std::string circuitFilePath, const std::string& componentName, const Via& via)
{
  auto fd = getExclusiveLock(circuitFilePath);
  auto tmpFilePath = circuitFilePath + ".tmp";
  std::ifstream inFile(circuitFilePath);
  std::ofstream outFile(tmpFilePath);

  std::string lineStr;
  while (std::getline(inFile, lineStr)) {
    auto componentLine = isComponentLine(lineStr, componentName);
    if (componentLine.isMatch) {
      outFile << fmt::format(
        "{}{}{}{}{}{}{},{}{}\n",
        componentLine.spaceVec[0],
        componentName,
        componentLine.spaceVec[1],
        componentLine.packageName,
        componentLine.spaceVec[2],
        via.x(),
        componentLine.spaceVec[3],
        componentLine.spaceVec[4],
        via.y()
      );
    }
    else {
      outFile << lineStr + "\n";
    }
  }

  auto result = rename(tmpFilePath.c_str(), circuitFilePath.c_str());
  if (result) {
    throw std::runtime_error(
      fmt::format("Could not replace file. new=\"{}\" old=\"{}\"",
                  tmpFilePath, circuitFilePath)
    );
  }

  releaseExclusiveLock(fd);
}

ComponentLine CircuitFileWriter::isComponentLine(const std::string &lineStr, const std::string &componentName)
{
  ComponentLine componentLine;
  componentLine.isMatch = false;
  static std::regex comFull("^(\\s*)(\\w+)(\\s+)(\\w+)(\\s+)(\\d+)(\\s*),(\\s*)(\\d+)\\s*$",
                     std::regex_constants::ECMAScript
                       | std::regex_constants::icase);
  std::smatch m;
  if (!std::regex_match(lineStr, m, comFull)) {
    return componentLine;
  }
  if (componentName != m[2].str()) {
    return componentLine;
  }
  componentLine.isMatch = true;
  componentLine.spaceVec.push_back(m[1].str());
  componentLine.spaceVec.push_back(m[3].str());
  componentLine.spaceVec.push_back(m[5].str());
  componentLine.spaceVec.push_back(m[7].str());
  componentLine.spaceVec.push_back(m[8].str());
  componentLine.packageName = m[4].str();
  return componentLine;
}
