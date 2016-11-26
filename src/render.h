#pragma once

#include <vector>

//#include <Eigen/Core>

#include "circuit.h"
#include "dijkstra.h"
#include "int_types.h"
#include "ogl_text.h"
#include "parser.h"
#include "solution.h"


class Render {
public:
  Render(float zoom);
  ~Render();
  void OpenGLInit();
  void set(u32 windowW, u32 windowH, u32 gridW, u32 gridH, float zoom,
                    const Pos& boardDragOffset);

  void draw(
    Circuit &circuit, const Solution &solution,
    u32 windowW, u32 windowH, u32 gridW, u32 gridH, float zoom,
    const Pos& boardDragOffset, bool showInputBool);
  Pos boardToScreenCoord(const Pos &boardCoord) const;
  Pos screenToBoardCoord(const Pos &screenCoord) const;
  Pos centerOffsetScreenPixels() const;
  float viaSpaceScreenPixels() const;

private:
  void drawCircuit(Circuit &);
  void drawSolution(const Solution &);
  void drawInputConnections(Circuit &);
  void drawFilledRectangle(const Pos& start, const Pos& end, float r, float g, float b, float alpha=1.0f);
  void drawFilledCircle(const Pos& center, float radius, float r, float g, float b, float alpha=1.0f);
  void drawThickLine(const Pos& start, const Pos& end, float radius, float r, float g, float b, float alpha=1.0f);
  void setColor(float r, float g, float b, float alpha);

  OglText oglText_;

  u32 windowW_;
  u32 windowH_;
  u32 gridW_;
  u32 gridH_;
  Pos boardDragOffset_;
  float zoom_;

  GLuint fillProgramId_;
  GLuint vertexBufId_;
};
