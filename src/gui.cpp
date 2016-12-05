#include "gui.h"

#include <fmt/format.h>


std::string getComponentAtMouseCoordinate(const Render &render, Circuit &circuit, const Pos &mouseBoardPos)
{
  for (auto &ci : circuit.componentNameToInfoMap) {
    auto &componentName = ci.first;
    auto footprint = circuit.calcComponentFootprint(componentName);
    Pos start = footprint.start.cast<float>();
    Pos end = footprint.end.cast<float>();
    start -= 0.5f;
    end += 0.5f;
//    auto m = render.screenToBoardCoord(mouseBoardPos);
//    fmt::print("{},{} / {},{} - ms={},{} mb={},{}\n", start.x(), start.y(), end.x(), end.y(),  mouseBoardPos.x(), mouseBoardPos.y(), m.x(), m.y());
    auto& m = mouseBoardPos;
    if (m.x() >= start.x() && m.x() <= end.x() && m.y() >= start.y() && m.y() <= end.y()) {
      return componentName;
    }
  }
  return "";
}

void
setComponentPosition(const Render &render, Circuit &circuit, const Pos &mouseBoardPos, const std::string &componentName)
{
//  Pos mouseBoardCoord = render.screenToBoardCoord(mouseBoardPos);
  circuit.componentNameToInfoMap[componentName].pin0AbsCoord = mouseBoardPos.cast<int>();
}
