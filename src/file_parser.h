#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "layout.h"

class CircuitFileParser
{
public:
  CircuitFileParser(Layout&);
  ~CircuitFileParser();
  void parse(std::string& circuitFilePath);
private:
  void parseLine(std::string lineStr);
  bool parseCommentOrEmpty(std::string &lineStr);
  bool parseBoard(std::string &lineStr);
  bool parseOffset(std::string &lineStr);
  bool parsePackage(std::string &lineStr);
  bool parseComponent(std::string &lineStr);
  bool parseConnection(std::string &lineStr);
  Layout& layout_;
  Via offset_;
};
