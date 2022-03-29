#pragma once

#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "layout.h"

class CircuitFileParser
{
  public:
  CircuitFileParser(Layout&);
  ~CircuitFileParser();
  void parse(std::string& circuitFilePath);

  private:
  void parseLine(std::string lineStr);
  bool parseCommentOrEmpty(std::string& lineStr);
  bool parseBoard(std::string& lineStr);
  bool parseOffset(std::string& lineStr);
  bool parsePackage(std::string& lineStr);
  bool parseComponent(std::string& lineStr);
  bool parseDontCare(std::string& lineStr);
  bool parseConnection(std::string& lineStr);
  void checkConnectionPoint(const ConnectionPoint& connectionPoint);
  bool parseAlias(std::string& lineStr);
  Layout& layout_;
  Via offset_;
  std::vector<std::pair<std::string, std::string> > aliases_;
};
