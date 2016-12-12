#include <string>

#include "via.h"


class ComponentLine
{
public:
  bool isMatch;
  std::string packageName;
  std::vector<std::string> spaceVec;
};


class CircuitFileWriter
{
public:
  void updateComponentPosition(const std::string circuitFilePath, const std::string& componentName, const Via& via);
private:
  ComponentLine isComponentLine(const std::string &lineStr, const std::string &componentName);
};

