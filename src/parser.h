#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "solution.h"

class Parser
{
public:
  Parser(Solution&);
  ~Parser();
  void parse();
private:
  void parseLine(std::string lineStr);
  bool parseBoard(std::string &lineStr);
  bool parseCommentOrEmpty(std::string &lineStr);
  bool parsePackage(std::string &lineStr);
  bool parseComponent(std::string &lineStr);
  bool parseConnection(std::string &lineStr);
  Solution& solution_;
};
