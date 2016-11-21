#pragma once

#include <vector>

#include "circuit.h"
#include "dijkstra.h"
#include "int_types.h"
#include "ogl_text.h"
#include "parser.h"
#include "solution.h"


class PcbDraw {
public:
  PcbDraw();
  PcbDraw(u32 windowW, u32 windowH);
  ~PcbDraw();
  void draw(Circuit& circuit, Solution& solution, bool showInputBool);
  void setZoom(float);
private:
  void drawCircuit(Circuit&, bool showInputBool);
  void drawSolution(Solution& solution);
  void drawFilledRectangle(float x1, float y1, float x2, float y2, float r, float g, float b, float alpha=1.0f);
  void drawFilledCircle(float x, float y, float radius, float r, float g, float b, float alpha=1.0f);
  void drawThickLine(float x1, float y1, float x2, float y2, float radius, float r, float g, float b, float alpha=1.0f);
  u32 windowW_;
  u32 windowH_;
  OglText oglText_;
  GLuint fillProgramId;
//  GLuint textBackgroundProgramId;
  GLuint vertexBufId_;
  float zoom_;
};
