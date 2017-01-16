#include "gui.h"

#include <fmt/format.h>

Pos boardToScrPos(
    const Pos& boardPos, const float zoom, const Pos& boardScreenOffset)
{
  return boardPos * zoom + boardScreenOffset;
}

Pos screenToBoardPos(
    const Pos& scrPos, const float zoom, const Pos& boardScreenOffset)
{
  return (scrPos - boardScreenOffset) / zoom;
}

Pos getMouseScrPos(const IntPos& intMousePos)
{
  return intMousePos.cast<float>();
}

Pos getMouseBoardPos(
    const IntPos& intMousePos, const float zoom, const Pos& boardScreenOffset)
{
  return screenToBoardPos(getMouseScrPos(intMousePos), zoom, boardScreenOffset);
}

std::string getComponentAtBoardPos(Circuit& circuit, const Pos& boardPos)
{
  for (auto& ci : circuit.componentNameToComponentMap) {
    auto& componentName = ci.first;
    auto footprint = circuit.calcComponentFootprint(componentName);
    Pos start = footprint.start.cast<float>();
    Pos end = footprint.end.cast<float>();
    start -= 0.5f;
    end += 0.5f;
    auto& p = boardPos;
    if (p.x() >= start.x() && p.x() <= end.x() && p.y() >= start.y()
        && p.y() <= end.y()) {
      return componentName;
    }
  }
  return "";
}

void setComponentPosition(
    Circuit& circuit, const Via& mouseBoardVia,
    const std::string& componentName)
{
  circuit.componentNameToComponentMap[componentName].pin0AbsPos = mouseBoardVia;
}
