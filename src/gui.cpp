#include "gui.h"

#include <fmt/format.h>


using namespace std;
using namespace fmt;


string getComponentAtMouseCoordinate(
  const Render& render,
  Circuit &circuit,
  const Pos &mouseCoord)
{
  for (auto& ci : circuit.componentNameToInfoMap) {
    auto& componentName = ci.first;
    auto footprint = circuit.calcComponentFootprint(componentName);
    Pos start = footprint.start.cast<float>();
    Pos end = footprint.end.cast<float>();
    start -= 0.5f;
    end += 0.5f;
    Pos startS = render.boardToScreenCoord(start);
    Pos endS = render.boardToScreenCoord(end);
    if (mouseCoord.x() >= startS.x() && mouseCoord.x() <= endS.x()
        && mouseCoord.y() >= startS.y() && mouseCoord.y() <= endS.y()) {
      return componentName;
    }
  }
  return "";
}

void setComponentPosition(
  const Render& render,
  Circuit &circuit,
  const Pos &mouseCoord,
  const string &componentName)
{
  Pos mouseBoardCoord = render.screenToBoardCoord(mouseCoord);
  circuit.componentNameToInfoMap[componentName].pin0AbsCoord = mouseBoardCoord.cast<int>();
}
