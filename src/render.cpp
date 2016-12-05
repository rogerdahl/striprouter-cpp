#define _USE_MATH_DEFINES
#include <cassert>
#include <iostream>
#include <cmath>
#include <mutex>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <fmt/format.h>

#include "render.h"
#include "shader.h"


const float PI_F = static_cast<float>(M_PI);
const int NUM_VIA_TRIANGLES = 6;
const int CIRCUIT_FONT_SIZE = 12;
const char *CIRCUIT_FONT_PATH = "./fonts/Roboto-Black.ttf";
const int NOTATION_FONT_SIZE = 10;
const float STRIP_WIDTH_PIXELS = 10.0f;
const float STRIP_GAP_PIXELS = 2.0f;
const float VIA_RADIUS_PIXELS = 2.5f;
const float WIRE_WIDTH_PIXELS = 1.5f;
const float SET_DIM = 0.3f;


Render::Render(float zoom)
  : oglText_(OglText(CIRCUIT_FONT_PATH, static_cast<int>(CIRCUIT_FONT_SIZE * zoom))),
    notations_(OglText(CIRCUIT_FONT_PATH, NOTATION_FONT_SIZE)),
    zoom_(zoom),
    fillProgramId_(0),
    vertexBufId_(0)
{
}

void Render::openGLInit()
{
  fillProgramId_ = createProgram("fill.vert", "fill.frag");
  glGenBuffers(1, &vertexBufId_);
  oglText_.openGLInit();
  notations_.openGLInit();
}

Render::~Render()
{
  glDeleteBuffers(1, &vertexBufId_);
}

void Render::draw(Solution &solution,
                  int windowW,
                  int windowH,
                  float zoom,
                  const Pos &boardDragOffset,
                  const Pos &mouseScreenPos,
                  bool showInputBool)
{
  solution_ = &solution;
  windowW_ = windowW;
  windowH_ = windowH;
  boardDragOffset_ = boardDragOffset;
  mouseScreenPos_ = mouseScreenPos;

  auto projection = glm::ortho(0.0f, static_cast<float>(windowW), static_cast<float>(windowH), 0.0f, 0.0f, 100.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(fillProgramId_);
  GLint projectionId = glGetUniformLocation(fillProgramId_, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  if (zoom != zoom_) {
    zoom_ = zoom;
    oglText_.reset(windowW, windowH, 0, 0, static_cast<int>(CIRCUIT_FONT_SIZE * zoom));
  }

  drawSolutionUsedStripboard();
  drawSolutionUsedWires();
  drawCircuit();
  drawInputConnections(!showInputBool);
  if (solution_->hasError) {
    drawDiag();
  }
}

Pos Render::boardToScreenCoord(const Pos &boardCoord) const
{
  return boardCoord * viaSpaceScreenPixels() + centerOffsetScreenPixels();
}

Pos Render::screenToBoardCoord(const Pos &screenCoord) const
{
  return (screenCoord - centerOffsetScreenPixels()) / viaSpaceScreenPixels();
}

Pos Render::centerOffsetScreenPixels() const
{
  auto p = Pos(windowW_ / 2.0f - solution_->gridW * viaSpaceScreenPixels() / 2.0f + boardDragOffset_.x(),
             windowH_ / 2.0f - solution_->gridH * viaSpaceScreenPixels() / 2.0f + boardDragOffset_.y());
//  fmt::print("centerOffsetScreenPixels() {},{}\n", p.x(), p.y());
  return p;
}

float Render::viaSpaceScreenPixels() const
{
  float stripWidthScreenPixels = STRIP_WIDTH_PIXELS * zoom_;
  float stripGapScreenPixels = STRIP_GAP_PIXELS * zoom_;
  return stripWidthScreenPixels + stripGapScreenPixels;
}

//
// Private
//

ViaValid Render::getMouseVia()
{
  Pos mouseBoardPos = screenToBoardCoord(mouseScreenPos_);
  Via v = Via(static_cast<int>(mouseBoardPos.x() + 0.5f), static_cast<int>(mouseBoardPos.y() + 0.5f));
  if (v.x() >= 0 && v.y() >= 0 && v.x() < solution_->gridW && v.y() < solution_->gridH) {
    return ViaValid(v, true);
  }
  else {
    return ViaValid(v, false);
  }
}

ViaSet &Render::getMouseNet()
{
  static auto emptyViaSet = ViaSet();
  auto v = getMouseVia();
  if (!v.isValid) {
    return emptyViaSet;
  }
  auto idx = solution_->idx(v.via);
  assert(idx < static_cast<int>(solution_->setIdxVec.size()));
  auto setIdx = solution_->setIdxVec[idx];
  if (setIdx == -1) {
    return emptyViaSet;
  }
  else {
    return solution_->viaSetVec[setIdx];
  }
}

float Render::setAlpha(const Via &v)
{
  auto mouseNet = getMouseNet();
  if (!mouseNet.size()) {
    return 1.0f;
  }
  return getMouseNet().count(v) ? 1.0f : SET_DIM;
}

void Render::drawStripboardSection(const ViaStartEnd &viaStartEnd)
{
  // Copper strip
  float gapFactor = STRIP_GAP_PIXELS / STRIP_WIDTH_PIXELS;
  float gapB = (1.0f - gapFactor) / 2.0f;
  int y1 = viaStartEnd.start.y();
  int y2 = viaStartEnd.end.y();
  if (y1 > y2) {
    std::swap(y1, y2);
  }
  Pos start(viaStartEnd.start.x() - gapB, y1 - 0.40f);
  Pos end(viaStartEnd.start.x() + gapB, y2 + 0.40f);
  auto f = setAlpha(viaStartEnd.start);
  drawFilledRectangle(start, end, RGBA(.85f * f, .565f * f, .345f * f, 1.0f));
  // Vias
  for (int y = y1; y <= y2; ++y) {
    addFilledCircle(Pos(viaStartEnd.start.x(), y), VIA_RADIUS_PIXELS);
//      drawFilledCircle(center, VIA_RADIUS_PIXELS, RGBA(0, 0, 0, 1));
  }
  drawAllFilledCircle(RGBA(0, 0, 0, 1));
}

void Render::drawCircuit()
{
  // Components
  for (auto ci : solution_->circuit.componentNameToInfoMap) {
    auto &componentName = ci.first;
//    auto& componentInfo = ci.second;
    auto footprint = const_cast<Solution &>(*solution_).circuit.calcComponentFootprint(componentName);
    // Component outline
    auto start = footprint.start.cast<float>() - 0.5f;
    auto end = footprint.end.cast<float>() + 0.5f;
    drawFilledRectangle(start, end, RGBA(0, 0, 0, 0.5f));
    // Component pins
    bool isPin0 = true;
    for (auto pinVia : const_cast<Solution &>(*solution_).circuit.calcComponentPins(componentName)) {
      if (isPin0) {
        isPin0 = false;
        auto start = pinVia.cast<float>() - .25f;
        auto end = pinVia.cast<float>() + .25f;
        drawFilledRectangle(start, end, RGBA(.784f, 0.0f, 0.0f, 1.0f));
      }
      else {
        addFilledCircle(pinVia.cast<float>(), VIA_RADIUS_PIXELS);
      }
    }
    drawAllFilledCircle(RGBA(.784f, 0.0f, 0.0f, 1.0f));
    // Component name
    int stringWidth = oglText_.calcStringWidth(componentName);
    int stringHeight = oglText_.getStringHeight();
    Pos txtCenterB(start.x() + (end.x() - start.x()) / 2.0f, start.y() + (end.y() - start.y()) / 2.0f);
    auto txtCenterS = boardToScreenCoord(txtCenterB);
    oglText_.reset(windowW_,
                   windowH_,
                   txtCenterS.x() - stringWidth / 2.0f,
                   txtCenterS.y() - stringHeight / 2.0f,
                   static_cast<int>(CIRCUIT_FONT_SIZE * zoom_));
    oglText_.print(0, componentName);
  }
}

void Render::drawInputConnections(bool onlyFailed)
{
  auto &s = solution_->routeStatusVec;
  auto v = const_cast<Solution &>(*solution_).circuit.genConnectionViaVec();
  int i = 0;
  for (auto c : v) {
    RGBA rgba;
    // If there is no routeStatusVec, the input solution is being rendered, and we draw all
    // lines in blue.
    if (!s.size()) {
      rgba = RGBA(0, .392f, .784f, 1.0f); // blue
    }
    else {
      if (s[i++]) {
        if (onlyFailed) {
          continue;
        }
        rgba = RGBA(0, .196f, .392f, 1.0f); // dark blue
      }
      else {
        rgba = RGBA(.784f, .392f, 0, 1.0f); // orange
      }
    }
    drawThickLine(c.start.cast<float>(), c.end.cast<float>(), 1.0f, rgba);
  }
}

void Render::drawSolutionUsedWires()
{
  for (auto routeSectionVec : solution_->routeVec) {
    for (auto section : routeSectionVec) {
      const auto &start = section.start.via;
      const auto &end = section.end.via;
      if (start.x() != end.x() && start.y() == end.y()) {
        int x1 = section.start.via.x();
        int x2 = section.end.via.x();
        if (x1 > x2) {
          std::swap(x1, x2);
        }
        RGBA rgba(0, 0, 0, 0.7f);
        auto mouseNet = getMouseNet();
        if (mouseNet.size() && mouseNet.count(section.start.via)) {
          rgba = RGBA(0.5f, 0.5f, 0.5f, 0.7f);
        }
        drawThickLine(section.start.via.cast<float>(), section.end.via.cast<float>(), WIRE_WIDTH_PIXELS, rgba);
      }
    }
  }
}

void Render::drawSolutionUsedStripboard()
{
// Routes
// Draw strips and wires separately so that wires are always on top.
// Strips
  for (auto routeSectionVec : solution_->routeVec) {
    for (auto section : routeSectionVec) {
      const auto &start = section.start.via;
      const auto &end = section.end.via;
      assert(section.start.isWireLayer == section.end.isWireLayer);
      if (!section.start.isWireLayer) {
//        int y1 = start.y();
//        int y2 = end.y();
//        if (y1 > y2) {
//          std::swap(y1, y2);
//        }
//        for (int y = y1; y <= y2; ++y) {
//          addFilledCircle(Pos(start.x(), y), VIA_RADIUS_PIXELS);
//        }
        drawStripboardSection(ViaStartEnd(start, end));
      }
    }
  }
}

// Draw diagnostics for debugging
void Render::drawDiag()
{
  // Draw diag route if specified
  for (auto v : solution_->diagRouteStepVec) {
    RGBA rgba;
    if (v.isWireLayer) {
      rgba = RGBA(1, 0, 0, 1);
    }
    else {
      rgba = RGBA(0, 1, 0, 1);
    }
    drawFilledCircle(v.via.cast<float>(), 1, rgba);
  }
  // Draw dots where costs have been set.
  for (int y = 0; y < solution_->gridH; ++y) {
    for (int x = 0; x < solution_->gridW; ++x) {
      int idx = x + solution_->gridW * y;
      auto v = solution_->diagCostVec[idx];
      if (v.wireCost != INT_MAX) {
        drawFilledCircle(Pos(x - 0.2f, y), 0.6f, RGBA(1, 0, 0, 1));
      }
      if (v.stripCost != INT_MAX) {
        drawFilledCircle(Pos(x + 0.2f, y), 0.6f, RGBA(0, 1, 0, 1));
      }
    }
  }
  // Draw start and end positions if set.
  if (solution_->diagStartVia.isValid) {
    drawFilledCircle(solution_->diagStartVia.via.cast<float>(), 1.5, RGBA(1, 1, 1, 1));
    printNotation(solution_->diagStartVia.via.cast<float>(), 0, "start");
  }
  if (solution_->diagEndVia.isValid) {
    drawFilledCircle(solution_->diagEndVia.via.cast<float>(), 1.5, RGBA(1, 1, 1, 1));
    printNotation(solution_->diagEndVia.via.cast<float>(), 0, "end");
  }
  // Draw wire jump labels
  for (int y = 0; y < solution_->gridH; ++y) {
    for (int x = 0; x < solution_->gridW; ++x) {
      auto &wireToVia = solution_->diagTraceVec[solution_->idx(Via(x, y))].wireToVia;
      if (wireToVia.isValid) {
        printNotation(Pos(x, y), 0, fmt::format("->{}", str(wireToVia.via)));
      }
    }
  }
  // Print error notice and any info
  auto nLine = 20;
  oglText_.reset(windowW_, windowH_, 0, 0, CIRCUIT_FONT_SIZE);
  oglText_.print(nLine++, "Diag");
  oglText_.print(nLine++, "wire = red");
  oglText_.print(nLine++, "strip = green");
  for (const auto &str : solution_->errorStringVec) {
    oglText_.print(nLine++, str);
  }
  // Mouse pointer coordinate info
  auto v = getMouseVia();
  if (v.isValid) {
    nLine = 2;
    auto idx = solution_->idx(v.via);
    printNotation(mouseScreenPos_, nLine++, fmt::format("{}", str(v.via)));
    const auto &viaCost = solution_->diagCostVec[idx];
    printNotation(mouseScreenPos_, nLine++, fmt::format("wireCost: {}", viaCost.wireCost));
    printNotation(mouseScreenPos_, nLine++, fmt::format("stripCost: {}", viaCost.stripCost));
    const auto &viaTrace = solution_->diagTraceVec[idx];
    printNotation(mouseScreenPos_, nLine++, fmt::format("wireBlocked: {}", viaTrace.isWireSideBlocked));
    // Nets
    printNotation(mouseScreenPos_, nLine++, fmt::format(""));
    auto setIdxSize = solution_->setIdxVec.size();
    printNotation(mouseScreenPos_, nLine++, fmt::format("setIdxSize: {}", setIdxSize));
    if (setIdxSize) {
      auto setIdx = solution_->setIdxVec[idx];
      printNotation(mouseScreenPos_, nLine++, fmt::format("setIdx: {}", setIdx));
      printNotation(mouseScreenPos_, nLine++, fmt::format("setSize: {}", solution_->viaSetVec[setIdx].size()));
    }
  }
}

// Print small notations using board coordinates. Used for debugging.
void Render::printNotation(Pos boardPos, int nLine, std::string msg)
{
  auto screenPos = boardToScreenCoord(boardPos);
  notations_.reset(windowW_, windowH_, screenPos.x() + 5, screenPos.y() + 5, NOTATION_FONT_SIZE);
  notations_.print(nLine, msg);
}

void Render::drawFilledRectangle(const Pos &start, const Pos &end, const RGBA &rgba)
{
  setColor(rgba);
  std::vector<GLfloat> triVec;
  auto startS = boardToScreenCoord(start);
  auto endS = boardToScreenCoord(end);
  triVec.insert(triVec.end(), {startS.x(), startS.y(), 0.0f});
  triVec.insert(triVec.end(), {endS.x(), endS.y(), 0.0f});
  triVec.insert(triVec.end(), {startS.x(), endS.y(), 0.0f});
  triVec.insert(triVec.end(), {startS.x(), startS.y(), 0.0f});
  triVec.insert(triVec.end(), {endS.x(), startS.y(), 0.0f});
  triVec.insert(triVec.end(), {endS.x(), endS.y(), 0.0f});
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::drawFilledCircle(const Pos &center, float radius, const RGBA &rgba)
{
  setColor(rgba);
  radius *= zoom_;
  int numViaTriangles = static_cast<int>(NUM_VIA_TRIANGLES * zoom_);
  auto centerS = boardToScreenCoord(center);
  std::vector<GLfloat> triVec;
  for (int i = 0; i <= numViaTriangles; ++i) {
    float x1 = centerS.x() + (radius * cosf(i * 2.0f * PI_F / numViaTriangles));
    float y1 = centerS.y() + (radius * sinf(i * 2.0f * PI_F / numViaTriangles));
    float x2 = centerS.x() + (radius * cosf((i + 1) * 2.0f * PI_F / numViaTriangles));
    float y2 = centerS.y() + (radius * sinf((i + 1) * 2.0f * PI_F / numViaTriangles));
    triVec.insert(triVec.end(), {centerS.x(), centerS.y(), 0.0f});
    triVec.insert(triVec.end(), {x1, y1, 0.0f});
    triVec.insert(triVec.end(), {x2, y2, 0.0f});
  }
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::addFilledCircle(const Pos &center, float radius)
{
  radius *= zoom_;
  int numViaTriangles = static_cast<int>(NUM_VIA_TRIANGLES * zoom_);
  auto centerS = boardToScreenCoord(center);
  for (int i = 0; i <= numViaTriangles; ++i) {
    float x1 = centerS.x() + (radius * cosf(i * 2.0f * PI_F / numViaTriangles));
    float y1 = centerS.y() + (radius * sinf(i * 2.0f * PI_F / numViaTriangles));
    float x2 = centerS.x() + (radius * cosf((i + 1) * 2.0f * PI_F / numViaTriangles));
    float y2 = centerS.y() + (radius * sinf((i + 1) * 2.0f * PI_F / numViaTriangles));
    triVec.insert(triVec.end(), {centerS.x(), centerS.y(), 0.0f});
    triVec.insert(triVec.end(), {x1, y1, 0.0f});
    triVec.insert(triVec.end(), {x2, y2, 0.0f});
  }
}

void Render::drawAllFilledCircle(const RGBA &rgba)
{
  setColor(rgba);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
  triVec.clear();
}

void Render::drawThickLine(const Pos &start, const Pos &end, float radius, const RGBA &rgba)
{
  drawFilledCircle(start, radius, rgba);
  drawFilledCircle(end, radius, rgba);
  radius *= zoom_;
  float t1 = radius * 2;
  float t2 = radius * 2;
  auto startS = boardToScreenCoord(start);
  auto endS = boardToScreenCoord(end);
  float angle = atan2(endS.y() - startS.y(), endS.x() - startS.x());
  float t2sina1 = t1 / 2 * sin(angle);
  float t2cosa1 = t1 / 2 * cos(angle);
  float t2sina2 = t2 / 2 * sin(angle);
  float t2cosa2 = t2 / 2 * cos(angle);
  std::vector<GLfloat> triVec;
  triVec.insert(triVec.end(), {startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {endS.x() + t2sina2, endS.y() - t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {startS.x() - t2sina1, startS.y() + t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f});
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::setColor(const RGBA &rgba)
{
  glUseProgram(fillProgramId_);
  GLint colorLoc = glGetUniformLocation(fillProgramId_, "color");
  glProgramUniform4fv(fillProgramId_, colorLoc, 1, rgba.data());
}

