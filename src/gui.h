#pragma once

#include <string>

#include <Eigen/Core>

#include "circuit.h"
#include "via.h"

Pos boardToScrPos(
    const Pos& boardPos, const float zoom, const Pos& boardScreenOffset);
Pos screenToBoardPos(
    const Pos& scrPos, const float zoom, const Pos& boardScreenOffset);
Pos getMouseScrPos(const IntPos& intMousePos);
Pos getMouseBoardPos(
    const IntPos& intMousePos, const float zoom, const Pos& boardScreenOffset);
std::string getComponentAtBoardPos(Circuit& circuit, const Pos& boardPos);
void setComponentPosition(
    Circuit& circuit, const Via& mouseBoardVia,
    const std::string& componentName);
