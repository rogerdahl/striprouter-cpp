# pragma once

#include <string>

#include <Eigen/Core>

#include "circuit.h"
#include "render.h"


using namespace Eigen;


std::string
getComponentAtMouseCoordinate(const Render& render,
                              Circuit &circuit,
                              const Pos &mouseCoord);

void setComponentPosition(const Render& render,
                          Circuit &circuit,
                          const Pos &mouseCoord,
                          const std::string& componentName);
