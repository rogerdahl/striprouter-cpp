#pragma once

#include <vector>

#include <Eigen/Core>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "circuit.h"
#include "layout.h"
#include "ogl_text.h"
#include "file_parser.h"
#include "router.h"


typedef Eigen::Array4f RGBA;

class Render
{
public:
  Render(); // float zoom
  ~Render();
  void openGLInit();
  void draw(
    Layout &layout,
    glm::tmat4x4<float>& projMat,
    const Pos& boardScreenOffset,
    const Pos& mouseBoardPos,
    float zoom,
    int windowW,
    int windowH,
    bool showRatsNestBool,
    bool showOnlyFailedBool
  );

private:
  void drawUsedStrips();
  void drawWireSections();
  void drawComponents();
  void drawStripboardSection(const StartEndVia &viaStartEnd);
  void drawRatsNest(bool showOnlyFailedBool);
  void drawBorder();
  void drawDiag();
  void drawFilledRectangle(const Pos &start, const Pos &end, const RGBA &);
  void drawFilledCircle(const Pos &center, float radius, const RGBA &);
  void addFilledCircle(const Pos &center, float radius);
  void drawFilledCircleBuffer(const RGBA &rgba);
  void
  drawThickLine(const Pos &start, const Pos &end, float radius, const RGBA &);
  void printNotation(Pos p, int nLine, std::string msg);
  void setColor(const RGBA &);
  bool isPointOutsideScreen(const Pos &p);
  bool isLineOutsideScreen(const Pos& start, const Pos& end);
  float setAlpha(const Via &);
  ValidVia getMouseVia();
  ViaSet &getMouseNet();

  OglText componentText_;
  OglText notationText_;

  Layout *layout_;
  glm::tmat4x4<float> projMat_;
  Pos boardScreenOffset_;
  Pos mouseBoardPos_;
  float zoom_;
  float windowW_;
  float windowH_;

  GLuint fillProgramId_;
  GLuint vertexBufId_;

  std::vector<GLfloat> triVec;
};
