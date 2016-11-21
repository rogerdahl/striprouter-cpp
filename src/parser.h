#pragma once

#include <string>
#include <vector>
#include <utility>
#include <map>

#include "int_types.h"
#include "circuit.h"


class Parser {
public:
  Parser();
  ~Parser();
  void parse(Circuit& circuit);
private:
  void parseLine(Circuit&, std::string lineStr);
  bool parseCommentOrEmpty(Circuit&, std::string &lineStr);
  bool parsePackage(Circuit&, std::string &lineStr);
  bool parseComponent(Circuit&, std::string &lineStr);
  bool parsePosition(Circuit&, std::string &lineStr);
  bool parseConnection(Circuit&, std::string &lineStr);

  void genComponentInfoMap(Circuit&);
  ComponentInfo genComponentInfo(Circuit&, std::string componentName);
  RelCoordStartEnd calcPackageFootprint(Circuit &, std::string package);
  void genConnectionCoordVec(Circuit&);
};
