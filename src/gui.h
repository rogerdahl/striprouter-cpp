# pragma once

#include <string>

#include <Eigen/Core>

#include "circuit.h"
#include "render.h"


using namespace Eigen;


std::string getComponentAtMouseCoordinate(const Render &render, Circuit &circuit, const Pos &mouseBoardPos);

void
setComponentPosition(const Render &render, Circuit &circuit, const Pos &mouseBoardPos, const std::string &componentName);
