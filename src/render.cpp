#define _USE_MATH_DEFINES
#include <cassert>
#include <cmath>
#include <iostream>
#include <mutex>

#include <GL/glew.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "gui.h"
#include "render.h"
#include "shader.h"

const float PI_F = static_cast<float>(M_PI);
const float CIRCUIT_FONT_SIZE = 1.0f;
const char* CIRCUIT_FONT_PATH = "./fonts/Roboto-Regular.ttf";
const int NOTATION_FONT_SIZE = 10;
const float SET_DIM = 0.3f;
const int NUM_VIA_TRIANGLES = 16;
const float CUT_W = 0.83f;
const float VIA_RADIUS = 0.2f;
const float WIRE_WIDTH = 0.125f;
const float RATS_NEST_WIRE_WIDTH = 0.1f;
const float CONNECTION_WIDTH = 0.1f;

Render::Render()
  : componentText_(OglText(CIRCUIT_FONT_PATH, CIRCUIT_FONT_SIZE)),
    notationText_(OglText(CIRCUIT_FONT_PATH, NOTATION_FONT_SIZE)),
    fillProgramId_(0),
    vertexBufId_(0)
{
}

Render::~Render()
{
}

void Render::openGLInit()
{
  fillProgramId_ = createProgram("fill.vert", "fill.frag");
  glGenBuffers(1, &vertexBufId_);
  componentText_.openGLInit();
  notationText_.openGLInit();
}

void Render::openGLFree()
{
  glDeleteBuffers(1, &vertexBufId_);
}

void Render::draw(
    Layout& layout, glm::mat4x4& projMat, const Pos& boardScreenOffset,
    const Pos& mouseBoardPos, float zoom, int windowW, int windowH,
    bool showRatsNestBool, bool showOnlyFailedBool)
{
  layout_ = &layout;
  projMat_ = projMat;
  boardScreenOffset_ = boardScreenOffset;
  mouseBoardPos_ = mouseBoardPos;
  zoom_ = zoom;
  windowW_ = static_cast<float>(windowW);
  windowH_ = static_cast<float>(windowH);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(fillProgramId_);
  GLint projectionId = glGetUniformLocation(fillProgramId_, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projMat));

  drawUsedStrips();
  drawWireSections();
  drawComponents();
  drawStripCuts();
  if (showRatsNestBool) {
    drawRatsNest(showOnlyFailedBool);
  }
  drawBorder();
  if (layout_->hasError) {
    drawDiag();
  }
}

//
// Private
//

void Render::drawUsedStrips()
{
  // Routes
  // Draw strips and wires separately so that wires are always on top.
  // Strips
  for (auto routeSectionVec : layout_->routeVec) {
    for (auto section : routeSectionVec) {
      const auto& start = section.start.via;
      const auto& end = section.end.via;
      assert(section.start.isWireLayer == section.end.isWireLayer);
      if (!section.start.isWireLayer) {
        drawStripboardSection(StartEndVia(start, end));
      }
    }
  }
}

void Render::drawWireSections()
{
  for (auto routeSectionVec : layout_->routeVec) {
    for (auto section : routeSectionVec) {
      const auto& start = section.start.via;
      const auto& end = section.end.via;
      if (start.x() != end.x() && start.y() == end.y()) {
        int x1 = section.start.via.x();
        int x2 = section.end.via.x();
        if (x1 > x2) {
          std::swap(x1, x2);
        }
        RGBA rgba(0, 0, 0, 0.7f);
        auto mouseNet = getMouseNet();
        if (mouseNet.size() && mouseNet.count(section.start.via)) {
          rgba = RGBA(0.7f, 0.7f, 0.7f, 0.7f);
        }
        drawThickLine(
            section.start.via.cast<float>(), section.end.via.cast<float>(),
            CONNECTION_WIDTH, rgba);
      }
    }
  }
}

void Render::drawComponents()
{
  componentText_.setFontH(static_cast<int>(CIRCUIT_FONT_SIZE * zoom_));
  for (auto ci : layout_->circuit.componentNameToComponentMap) {
    auto& componentName = ci.first;
    auto& component = ci.second;
    // Footprint
    auto footprint = layout_->circuit.calcComponentFootprint(componentName);
    auto start = footprint.start.cast<float>() - 0.5f;
    auto end = footprint.end.cast<float>() + 0.5f;
    drawFilledRectangle(start, end, RGBA(0, 0, 0, 0.4f));
    // Pins
    bool isPin0 = true;
    int pinIdx = 0;
    for (auto pinVia : const_cast<Layout&>(*layout_).circuit.calcComponentPins(
             componentName)) {
      auto isDontCarePin = component.dontCarePinIdxSet.count(pinIdx) > 0;
      RGBA rgba = isDontCarePin ? RGBA(0.0f, .784f, 0.0f, 1.0f)
                                : RGBA(.784f, 0.0f, 0.0f, 1.0f);
      if (isPin0) {
        isPin0 = false;
        auto start = pinVia.cast<float>() - VIA_RADIUS;
        auto end = pinVia.cast<float>() + VIA_RADIUS;
        drawFilledRectangle(start, end, rgba);
      }
      else {
        drawFilledCircle(pinVia.cast<float>(), VIA_RADIUS * zoom_, rgba);
      }
      ++pinIdx;
    }
    // Name label
    int stringWidth = componentText_.calcStringWidth(componentName);
    int stringHeight = componentText_.getLineHeight();
    Pos txtCenterB(
        start.x() + (end.x() - start.x()) / 2.0f,
        start.y() + (end.y() - start.y()) / 2.0f);
    auto txtCenterS = boardToScrPos(txtCenterB, zoom_, boardScreenOffset_);
    componentText_.print(
        projMat_, txtCenterS.x() - stringWidth / 2.0f,
        txtCenterS.y() - stringHeight / 2.0f, 0, componentName, true);
  }
}

void Render::drawStripboardSection(const StartEndVia& viaStartEnd)
{
  // Copper strip
  int y1 = viaStartEnd.start.y();
  int y2 = viaStartEnd.end.y();
  if (y1 > y2) {
    std::swap(y1, y2);
  }
  Pos start(viaStartEnd.start.x() - CUT_W / 2.0f, y1 - 0.40f);
  Pos end(viaStartEnd.start.x() + CUT_W / 2.0f, y2 + 0.40f);
  auto f = setAlpha(viaStartEnd.start);
  drawFilledRectangle(start, end, RGBA(.85f * f, .565f * f, .345f * f, 1.0f));
  // Vias
  for (int y = y1; y <= y2; ++y) {
    addFilledCircle(Pos(viaStartEnd.start.x(), y), VIA_RADIUS);
  }
  drawFilledCircleBuffer(RGBA(0, 0, 0, 1));
}

void Render::drawStripCuts()
{
  for (auto& v : layout_->stripCutVec) {
    auto halfStripW = CUT_W / 2.0f;
    auto halfCutH = 0.08f / 2.0f;
    Pos start(v.x() - halfStripW, v.y() - halfCutH);
    Pos end(v.x() + halfStripW, v.y() + halfCutH);
    drawFilledRectangle(
        start - Pos(0, 0.5f), end - Pos(0, 0.5f), RGBA(0, 0.8f, 0.8f, 1));
  }
}

void Render::drawRatsNest(bool showOnlyFailedBool)
{
  auto& routedConVec = layout_->routeStatusVec;
  auto allConVec = layout_->circuit.genConnectionViaVec();
  int i = 0;
  for (auto c : allConVec) {
    auto blueRgba = RGBA(0, .392f, .784f, 0.5f); // not yet routed
    auto greenRgba = RGBA(0, .584f, .192f, 0.5f); // successfully routed
    auto orangeRgba = RGBA(.784f, .3f, 0, 0.5f); // failed routing
    Pos start = c.start.cast<float>();
    Pos end = c.end.cast<float>();
    if (i < static_cast<int>(routedConVec.size())) {
      // Within the routed set
      if (routedConVec[i]) {
        if (!showOnlyFailedBool) {
          drawThickLine(start, end, RATS_NEST_WIRE_WIDTH, greenRgba);
        }
      }
      else {
        drawThickLine(start, end, RATS_NEST_WIRE_WIDTH, orangeRgba);
      }
    }
    else {
      // Outside the routed set
      if (!showOnlyFailedBool) {
        drawThickLine(start, end, RATS_NEST_WIRE_WIDTH, blueRgba);
      }
    }
    ++i;
  }
}

void Render::drawBorder()
{
  Pos start = Pos(0, 0) - 0.5f;
  Pos end = Pos(layout_->gridW - 1, layout_->gridH - 1) + 0.5f;
  RGBA rgba(0, 0, 0, 1);
  float radius = 0.2f;
  drawThickLine(
      Pos(start.x(), start.y()), Pos(end.x(), start.y()), radius, rgba);
  drawThickLine(
      Pos(start.x(), start.y()), Pos(start.x(), end.y()), radius, rgba);
  drawThickLine(Pos(end.x(), start.y()), Pos(end.x(), end.y()), radius, rgba);
  drawThickLine(Pos(start.x(), end.y()), Pos(end.x(), end.y()), radius, rgba);
}

// Draw diagnostics for debugging
void Render::drawDiag()
{
  // Draw diag route if specified
  for (auto v : layout_->diagRouteStepVec) {
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
  for (int y = 0; y < layout_->gridH; ++y) {
    for (int x = 0; x < layout_->gridW; ++x) {
      int idx = x + layout_->gridW * y;
      auto v = layout_->diagCostVec[idx];
      if (v.wireCost != INT_MAX) {
        drawFilledCircle(Pos(x - 0.2f, y), 0.75f, RGBA(1, 0, 0, 1));
      }
      if (v.stripCost != INT_MAX) {
        drawFilledCircle(Pos(x + 0.2f, y), 0.75f, RGBA(0, 1, 0, 1));
      }
    }
  }
  // Draw start and end positions if set.
  if (layout_->diagStartVia.isValid) {
    drawFilledCircle(
        layout_->diagStartVia.via.cast<float>(), 1.5, RGBA(1, 1, 1, 1));
    printNotation(layout_->diagStartVia.via.cast<float>(), 0, "start");
  }
  if (layout_->diagEndVia.isValid) {
    drawFilledCircle(
        layout_->diagEndVia.via.cast<float>(), 1.5, RGBA(1, 1, 1, 1));
    printNotation(layout_->diagEndVia.via.cast<float>(), 0, "end");
  }
  // Draw wire jump labels
  for (int y = 0; y < layout_->gridH; ++y) {
    for (int x = 0; x < layout_->gridW; ++x) {
      auto& wireToVia =
          layout_->diagTraceVec[layout_->idx(Via(x, y))].wireToVia;
      if (wireToVia.isValid) {
        printNotation(Pos(x, y), 0, fmt::format("->{}", str(wireToVia.via)));
      }
    }
  }
  // Print error notice and any info
  auto nLine = 0;
  auto sideBoardPos = screenToBoardPos(Pos(0, 300), zoom_, boardScreenOffset_);
  printNotation(sideBoardPos, nLine++, "Diag");
  printNotation(sideBoardPos, nLine++, "wire = red");
  printNotation(sideBoardPos, nLine++, "strip = green");
  for (const auto& str : layout_->errorStringVec) {
    printNotation(sideBoardPos, nLine++, str);
  }
  // Mouse pointer coordinate info
  auto v = getMouseVia();
  if (v.isValid) {
    nLine = 2;
    auto idx = layout_->idx(v.via);
    printNotation(mouseBoardPos_, nLine++, fmt::format("{}", str(v.via)));
    const auto& viaCost = layout_->diagCostVec[idx];
    printNotation(
        mouseBoardPos_, nLine++, fmt::format("wireCost: {}", viaCost.wireCost));
    printNotation(
        mouseBoardPos_, nLine++,
        fmt::format("stripCost: {}", viaCost.stripCost));
    const auto& viaTrace = layout_->diagTraceVec[idx];
    printNotation(
        mouseBoardPos_, nLine++,
        fmt::format("wireBlocked: {}", viaTrace.isWireSideBlocked));
    // Nets
    printNotation(mouseBoardPos_, nLine++, fmt::format(""));
    auto setIdxSize = layout_->setIdxVec.size();
    //    printNotation(mouseBoardPos_,
    //                  nLine++,
    //                  fmt::format("setIdxSize: {}", setIdxSize));
    if (setIdxSize) {
      auto setIdx = layout_->setIdxVec[idx];
      printNotation(mouseBoardPos_, nLine++, fmt::format("setIdx: {}", setIdx));
      printNotation(
          mouseBoardPos_, nLine++,
          fmt::format("setSize: {}", layout_->viaSetVec[setIdx].size()));
    }
  }
}

void Render::drawFilledRectangle(
    const Pos& start, const Pos& end, const RGBA& rgba)
{
  if (isLineOutsideScreen(start, end)) {
    return;
  }
  setColor(rgba);
  std::vector<GLfloat> triVec;
  auto startS = boardToScrPos(start, zoom_, boardScreenOffset_);
  auto endS = boardToScrPos(end, zoom_, boardScreenOffset_);
  triVec.insert(triVec.end(), { startS.x(), startS.y(), 0.0f });
  triVec.insert(triVec.end(), { endS.x(), endS.y(), 0.0f });
  triVec.insert(triVec.end(), { startS.x(), endS.y(), 0.0f });
  triVec.insert(triVec.end(), { startS.x(), startS.y(), 0.0f });
  triVec.insert(triVec.end(), { endS.x(), startS.y(), 0.0f });
  triVec.insert(triVec.end(), { endS.x(), endS.y(), 0.0f });
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(
      GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0],
      GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::drawFilledCircle(const Pos& center, float radius, const RGBA& rgba)
{
  if (isPointOutsideScreen(center)) {
    return;
  }
  setColor(rgba);
  int numViaTriangles = NUM_VIA_TRIANGLES;
  auto centerS = boardToScrPos(center, zoom_, boardScreenOffset_);
  std::vector<GLfloat> triVec;
  for (int i = 0; i <= numViaTriangles; ++i) {
    float x1 = centerS.x() + (radius * cosf(i * 2.0f * PI_F / numViaTriangles));
    float y1 = centerS.y() + (radius * sinf(i * 2.0f * PI_F / numViaTriangles));
    float x2 =
        centerS.x() + (radius * cosf((i + 1) * 2.0f * PI_F / numViaTriangles));
    float y2 =
        centerS.y() + (radius * sinf((i + 1) * 2.0f * PI_F / numViaTriangles));
    triVec.insert(triVec.end(), { centerS.x(), centerS.y(), 0.0f });
    triVec.insert(triVec.end(), { x1, y1, 0.0f });
    triVec.insert(triVec.end(), { x2, y2, 0.0f });
  }
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(
      GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0],
      GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::addFilledCircle(const Pos& center, float radius)
{
  if (isPointOutsideScreen(center)) {
    return;
  }
  radius *= zoom_;
  int numViaTriangles = NUM_VIA_TRIANGLES;
  auto centerS = boardToScrPos(center, zoom_, boardScreenOffset_);

  if (centerS.x() + radius < 0.0f || centerS.x() - radius > windowW_
      || centerS.y() + radius < 0.0f || centerS.y() - radius > windowH_) {
    return;
  }

  for (int i = 0; i <= numViaTriangles; ++i) {
    float x1 = centerS.x() + (radius * cosf(i * 2.0f * PI_F / numViaTriangles));
    float y1 = centerS.y() + (radius * sinf(i * 2.0f * PI_F / numViaTriangles));
    float x2 =
        centerS.x() + (radius * cosf((i + 1) * 2.0f * PI_F / numViaTriangles));
    float y2 =
        centerS.y() + (radius * sinf((i + 1) * 2.0f * PI_F / numViaTriangles));
    triVec.insert(triVec.end(), { centerS.x(), centerS.y(), 0.0f });
    triVec.insert(triVec.end(), { x1, y1, 0.0f });
    triVec.insert(triVec.end(), { x2, y2, 0.0f });
  }
}

void Render::drawFilledCircleBuffer(const RGBA& rgba)
{
  if (!triVec.size()) {
    return;
  }
  setColor(rgba);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(
      GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0],
      GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
  triVec.clear();
}

void Render::drawThickLine(
    const Pos& start, const Pos& end, float radius, const RGBA& rgba)
{
  if (isLineOutsideScreen(start, end)) {
    return;
  }
  radius *= zoom_;
  drawFilledCircle(start, radius, rgba);
  drawFilledCircle(end, radius, rgba);
  float t1 = radius * 2;
  float t2 = radius * 2;
  auto startS = boardToScrPos(start, zoom_, boardScreenOffset_);
  auto endS = boardToScrPos(end, zoom_, boardScreenOffset_);
  float angle = atan2(endS.y() - startS.y(), endS.x() - startS.x());
  float t2sina1 = t1 / 2 * sin(angle);
  float t2cosa1 = t1 / 2 * cos(angle);
  float t2sina2 = t2 / 2 * sin(angle);
  float t2cosa2 = t2 / 2 * cos(angle);
  std::vector<GLfloat> triVec;
  triVec.insert(
      triVec.end(), { startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f });
  triVec.insert(triVec.end(), { endS.x() + t2sina2, endS.y() - t2cosa2, 0.0f });
  triVec.insert(triVec.end(), { endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f });
  triVec.insert(triVec.end(), { endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f });
  triVec.insert(
      triVec.end(), { startS.x() - t2sina1, startS.y() + t2cosa1, 0.0f });
  triVec.insert(
      triVec.end(), { startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f });
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(
      GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0],
      GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

// Print small notations using board coordinates. Used for debugging.
void Render::printNotation(Pos boardPos, int nLine, std::string msg)
{
  auto scrPos = boardToScrPos(boardPos, zoom_, boardScreenOffset_);
  notationText_.print(projMat_, scrPos.x() + 5, scrPos.y() + 5, nLine, msg);
}

void Render::setColor(const RGBA& rgba)
{
  glUseProgram(fillProgramId_);
  GLint colorLoc = glGetUniformLocation(fillProgramId_, "color");
  glProgramUniform4fv(fillProgramId_, colorLoc, 1, rgba.data());
}

bool Render::isPointOutsideScreen(const Pos& p)
{
  float pad = 50.0f;
  return p.x() < -pad || p.x() >= windowW_ + pad || p.y() < -pad
         || p.y() >= windowH_ + pad;
}

bool Render::isLineOutsideScreen(const Pos& start, const Pos& end)
{
  float pad = 50.0f;
  return (start.x() < -pad && end.x() < -pad)
         || (start.x() >= windowW_ + pad && end.x() >= windowW_ + pad)
         || (start.y() < -pad && end.y() < -pad)
         || (start.y() >= windowW_ + pad && end.y() >= windowH_ + pad);
}

float Render::setAlpha(const Via& v)
{
  auto mouseNet = getMouseNet();
  if (!mouseNet.size()) {
    return 1.0f;
  }
  return getMouseNet().count(v) ? 1.0f : SET_DIM;
}

ValidVia Render::getMouseVia()
{
  Via v =
      Via(static_cast<int>(mouseBoardPos_.x() + 0.5f),
          static_cast<int>(mouseBoardPos_.y() + 0.5f));
  if (v.x() >= 0 && v.y() >= 0 && v.x() < layout_->gridW
      && v.y() < layout_->gridH) {
    return ValidVia(v, true);
  }
  else {
    return ValidVia(v, false);
  }
}

ViaSet& Render::getMouseNet()
{
  static auto emptyViaSet = ViaSet();
  auto v = getMouseVia();
  if (!v.isValid) {
    return emptyViaSet;
  }
  auto idx = layout_->idx(v.via);
  assert(idx < static_cast<int>(layout_->setIdxVec.size()));
  auto setIdx = layout_->setIdxVec[idx];
  if (setIdx == -1) {
    return emptyViaSet;
  }
  else {
    return layout_->viaSetVec[setIdx];
  }
}
