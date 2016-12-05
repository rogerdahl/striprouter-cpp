#pragma once

#include <vector>

#include <Eigen/Core>

#include "circuit.h"
#include "router.h"
#include "ogl_text.h"
#include "parser.h"
#include "solution.h"


typedef Eigen::Array4f RGBA;

class Render
{
public:
  Render(float zoom);
  ~Render();
  void openGLInit();
  void draw(Solution &solution,
            int windowW,
            int windowH,
            float zoom,
            const Pos &boardDragOffset,
            const Pos &mouseScreenPos,
            bool showInputBool);
  Pos boardToScreenCoord(const Pos &boardCoord) const;
  Pos screenToBoardCoord(const Pos &screenCoord) const;
  Pos centerOffsetScreenPixels() const;
  float viaSpaceScreenPixels() const;

private:
  ViaValid getMouseVia();
  ViaSet &getMouseNet();
  float setAlpha(const Via &);
  void drawSolutionUsedStripboard();
  void drawSolutionUsedWires();
  void drawStripboardSection(const ViaStartEnd &viaStartEnd);
  void drawCircuit();
  void drawInputConnections(bool onlyFailed);
  void drawDiag();
  void drawFilledRectangle(const Pos &start, const Pos &end, const RGBA &);
  void drawFilledCircle(const Pos &center, float radius, const RGBA &);

  void addFilledCircle(const Pos &center, float radius);
  void drawAllFilledCircle(const RGBA &rgba);

  void drawThickLine(const Pos &start, const Pos &end, float radius, const RGBA &);
  void setColor(const RGBA &);

  void printNotation(Pos p, int nLine, std::string msg);

  OglText oglText_;
  OglText notations_;

  Solution *solution_;

  int windowW_;
  int windowH_;
  Pos boardDragOffset_;
  Pos mouseScreenPos_;
  float zoom_;

  GLuint fillProgramId_;
  GLuint vertexBufId_;

  std::vector<GLfloat> triVec;
};
