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
const u32 CIRCUIT_FONT_SIZE = 14;
const char *CIRCUIT_FONT_PATH = "./fonts/LiberationMono-Regular.ttf";
const float STRIP_WIDTH_PIXELS = 10.0f;
const float STRIP_GAP_PIXELS = 2.0f;
const float VIA_RADIUS_PIXELS = 2.5f;
const float WIRE_WIDTH_PIXELS = 1.5f;


using namespace std;
using namespace fmt;
using namespace Eigen;


Render::Render(float zoom)
  : oglText_(OglText(CIRCUIT_FONT_PATH, static_cast<u32>(CIRCUIT_FONT_SIZE * zoom))),
    zoom_(zoom), fillProgramId_(0), vertexBufId_(0)
{
}

void Render::OpenGLInit()
{
  fillProgramId_ = createProgram("fill.vert", "fill.frag");
  glGenBuffers(1, &vertexBufId_);
  oglText_.OpenGLInit();
}

Render::~Render()
{
  glDeleteBuffers(1, &vertexBufId_);
}

void Render::set(u32 windowW, u32 windowH, u32 gridW, u32 gridH, float zoom,
                   const Pos& boardDragOffset)
{
  windowW_ = windowW;
  windowH_ = windowH;
  gridW_ = gridW;
  gridH_ = gridH;
  zoom_ = zoom;
}

void Render::draw(Circuit &circuit, const Solution &solution,
                   u32 windowW, u32 windowH, u32 gridW, u32 gridH, float zoom,
                   const Pos& boardDragOffset, bool showInputBool)
{
  windowW_ = windowW;
  windowH_ = windowH;
  gridW_ = gridW;
  gridH_ = gridH;
  zoom_ = zoom;
  boardDragOffset_ = boardDragOffset; 

  auto projection = glm::ortho(0.0f, static_cast<float>(windowW),
                               static_cast<float>(windowH), 0.0f, 0.0f,
                               100.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(fillProgramId_);
  GLint projectionId = glGetUniformLocation(fillProgramId_, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  if (zoom != zoom_) {
    zoom_ = zoom;
    oglText_.reset(windowW, windowH, 0, 0, static_cast<u32>(CIRCUIT_FONT_SIZE* zoom));
  }

  drawCircuit(circuit);
  drawSolution(solution);
  if (showInputBool) {
    drawInputConnections(circuit);
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
  return Pos(
    windowW_ / 2.0f - gridW_ * viaSpaceScreenPixels() / 2.0f + boardDragOffset_.x(),
    windowH_ / 2.0f - gridH_ * viaSpaceScreenPixels() / 2.0f + boardDragOffset_.y()
  );
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

void Render::drawCircuit(Circuit &circuit)
{
  lock_guard<mutex> lockCircuit(circuitMutex);

  // Copper strips
  float gapFactor = STRIP_GAP_PIXELS / STRIP_WIDTH_PIXELS;
  float gapB = (1.0f - gapFactor) / 2.0f;
  for (u32 x = 0; x < gridW_; ++x) {
    Pos start(x - gapB, -1.0f);
    Pos end(x + gapB, gridH_);
    drawFilledRectangle(start, end, 217.0f, 144.0f, 88.0f);
  }
  // Vias
  for (u32 y = 0; y < gridH_; ++y) {
    for (u32 x = 0; x < gridW_; ++x) {
      Pos center(x, y);
      drawFilledCircle(center, VIA_RADIUS_PIXELS, 0.0f, 0.0f, 0.0f);
    }
  }
  // Components
  for (auto ci : circuit.componentNameToInfoMap) {
    auto& componentName = ci.first;
    auto& componentInfo = ci.second;
    auto footprint = circuit.calcComponentFootprint(componentName);
    // Component outline
    auto start = footprint.start.cast<float>() - 0.5f;
    auto end = footprint.end.cast<float>() + 0.5f;
    drawFilledRectangle(start, end, 0, 0, 0, 0.5f);
    // Component pins
    bool isPin0 = true;
    for (auto pinVia : circuit.calcComponentPins(componentName)) {
      if (isPin0) {
        isPin0 = false;
        auto start = pinVia.cast<float>() - 0.5f;
        auto end = pinVia.cast<float>() + 0.5f;
        drawFilledRectangle(start, end, 0, 0, 0, 1.0f);
      }
      drawFilledCircle(pinVia.cast<float>(), VIA_RADIUS_PIXELS, 200.0f, 0.0f, 0.0f);
    }
    // Component name
    u32 stringWidth = oglText_.calcStringWidth(componentName);
    u32 stringHeight = oglText_.getStringHeight();
    Pos txtCenterB(start.x() + (end.x() - start.x()) / 2.0f,
                        start.y() + (end.y() - start.y()) / 2.0f);
    auto txtCenterS = boardToScreenCoord(txtCenterB);
    oglText_.reset(windowW_, windowH_,
                   txtCenterS.x() - stringWidth / 2.0f,
                   txtCenterS.y() - stringHeight / 2.0f,
                   static_cast<u32>(CIRCUIT_FONT_SIZE * zoom_));
    oglText_.print(0, componentName);
  }
}

void Render::drawInputConnections(Circuit &circuit)
{
  for (auto c : circuit.genConnectionViaVec()) {
    drawThickLine(c.start.cast<float>(), c.end.cast<float>(), 1.5f, 0, 100, 200);
  }
}

void Render::drawSolution(const Solution &solution)
{
  // Routes
  // Draw circles and lines separately so that lines are always on top.
  for (auto RouteStepVec : solution.getRouteVec()) {
    ViaValid prev;
    for (auto c : RouteStepVec) {
      if (prev.isValid && prev.via.x() == c.via.x()) {
        drawFilledCircle(c.via.cast<float>(), VIA_RADIUS_PIXELS, 200.0f, 200.0f, 0.0f);
      }
      prev = c.via;
    }
  }
  for (auto RouteStepVec : solution.getRouteVec()) {
    ViaValid prev;
    for (auto c : RouteStepVec) {
      if (prev.isValid && prev.via.y() == c.via.y()) {
        drawThickLine(prev.via.cast<float>(), c.via.cast<float>(), WIRE_WIDTH_PIXELS, 0, 0, 0, 0.7f);
      }
      prev = c.via;
    }
  }
}


void Render::drawFilledRectangle(const Pos& start, const Pos& end, float r, float g, float b, float alpha)
{
  setColor(r, g, b, alpha);
  vector<GLfloat> triVec;
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
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::drawFilledCircle(const Pos& center, float radius,
                               float r, float g, float b, float alpha)
{
  setColor(r, g, b, alpha);
  radius *= zoom_;
  u32 numViaTriangles = static_cast<u32>(NUM_VIA_TRIANGLES * zoom_);
  auto centerS = boardToScreenCoord(center);
  vector<GLfloat> triVec;
  for (u32 i = 0; i <= numViaTriangles; ++i) {
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
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void
Render::drawThickLine(const Pos& start, const Pos& end, float radius,
                       float r, float g, float b, float alpha)
{
  drawFilledCircle(start, radius, r, g, b, alpha);
  drawFilledCircle(end, radius, r, g, b, alpha);
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
  vector<GLfloat> triVec;
  triVec.insert(triVec.end(), {startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {endS.x() + t2sina2, endS.y() - t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {endS.x() - t2sina2, endS.y() + t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {startS.x() - t2sina1, startS.y() + t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {startS.x() + t2sina1, startS.y() - t2cosa1, 0.0f});
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void Render::setColor(float r, float g, float b, float alpha)
{
  glUseProgram(fillProgramId_);
  float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, alpha};
  GLint colorLoc = glGetUniformLocation(fillProgramId_, "color");
  glProgramUniform4fv(fillProgramId_, colorLoc, 1, color);
}

