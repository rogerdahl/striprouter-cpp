#include <string>

#include "circuit.h"


class ComponentLine
{
public:
  bool isComponentLine;
  std::string packageName;
  std::string componentName;
  std::vector<std::string> spaceVec;
};


class CircuitFileWriter
{
public:
  void updateComponentPositions(const std::string circuitFilePath,
                                const Circuit& circuit);
private:
  ComponentLine parseComponentLine(const std::string& lineStr);
};

