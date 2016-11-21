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

using namespace std;
using namespace fmt;

const float PI_F = static_cast<float>(M_PI);
const int NUM_VIA_TRIANGLES = 6;
const u32 FONT_SIZE2 = 10;
const float STRIP_WIDTH_PIXELS = 10;
const float STRIP_SPACE_PIXELS = 2;
const float VIA_RADIUS_PIXELS = 2.5f;
const float ZOOM_DEFAULT = 1.5f;

const char *FONT_PATH2 = "./fonts/LiberationMono-Regular.ttf";


PcbDraw::PcbDraw(u32 windowW, u32 windowH, u32 gridW, u32 gridH)
  : windowW_(windowW), windowH_(windowH), gridW_(gridW), gridH_(gridH),
  oglText_(OglText(windowW, windowH, FONT_PATH2, FONT_SIZE2 * ZOOM_DEFAULT)),
    zoom_(ZOOM_DEFAULT)
{
  fillProgramId = createProgram("fill.vert", "fill.frag");
  glGenBuffers(1, &vertexBufId_);
}

PcbDraw::~PcbDraw()
{
  glDeleteBuffers(1, &vertexBufId_);
}

void PcbDraw::setZoom(float zoom)
{
  zoom_ = zoom;
}

void PcbDraw::draw(Circuit& circuit, Solution& solution, bool showInputBool)
{
  drawCircuit(circuit, showInputBool);
  drawSolution(solution);
}

void PcbDraw::drawCircuit(Circuit& circuit, bool showInputBool)
{
  lock_guard<mutex> lockCircuit(circuitMutex);

  auto projection = glm::ortho(0.0f, static_cast<float>(windowW_),
                               static_cast<float>(windowH_), 0.0f, 0.0f,
                               100.0f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(fillProgramId);
  GLint projectionId = glGetUniformLocation(fillProgramId, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  float via_radius_pixels = VIA_RADIUS_PIXELS * zoom_;
  float strip_width_pixels = STRIP_WIDTH_PIXELS * zoom_;
  float strip_space_pixels = STRIP_SPACE_PIXELS * zoom_;
  float via_space_pixels = strip_width_pixels + strip_space_pixels;
  float half_via_space_pixels = via_space_pixels / 2.0f;

  // Copper strips
  for (u32 x = 0; x < gridW_; ++x) {
    drawFilledRectangle(x * via_space_pixels - (strip_width_pixels / 2), 0,
                        x * via_space_pixels + (strip_width_pixels / 2),
                        gridH_ * via_space_pixels, 217.0f, 144.0f, 88.0f);
  }
  // Vias
  for (u32 y = 0; y < gridH_; ++y) {
    for (u32 x = 0; x < gridW_; ++x) {
      drawFilledCircle(x * via_space_pixels, y * via_space_pixels,
                       via_radius_pixels, 0.0f, 0.0f, 0.0f);
    }
  }
  // Draw components
  for (auto componentName : circuit.getComponentNameVec()) {
    auto ci = circuit.getComponentInfoMap().find(componentName)->second;
    // Component outline
    drawFilledRectangle(
      ci.footprint.start.x * via_space_pixels - half_via_space_pixels,
      ci.footprint.start.y * via_space_pixels - half_via_space_pixels,
      ci.footprint.end.x * via_space_pixels + half_via_space_pixels,
      ci.footprint.end.y * via_space_pixels + half_via_space_pixels, 0, 0, 0,
      0.5f);
    // Component pins
    for (auto pinAbsCoord :  ci.pinAbsCoordVec) {
      s32 x = pinAbsCoord.x;
      s32 y = pinAbsCoord.y;
      drawFilledCircle(x * via_space_pixels, y * via_space_pixels,
                       via_radius_pixels, 200.0f, 0.0f, 0.0f);
    }
    // Component name
    u32 stringWidth = oglText_.calcStringWidth(ci.componentName);
    u32 stringHeight = oglText_.getStringHeight();
    u32 x1 = ci.footprint.start.x * via_space_pixels;
    u32 y1 = ci.footprint.start.y * via_space_pixels;
    u32 x2 = ci.footprint.end.x * via_space_pixels;
    u32 y2 = ci.footprint.end.y * via_space_pixels;
    u32 centerX = x1 + (x2 - x1) / 2;
    u32 centerY = y1 + (y2 - y1) / 2;
    oglText_.print(centerX - stringWidth / 2, centerY - stringHeight / 2, 0,
                   ci.componentName);
  }
  // Draw input connections
  if (showInputBool) {
    for (auto viaStartEnd : circuit.getConnectionCoordVec()) {
      drawThickLine(
        viaStartEnd.start.x * via_space_pixels,
        viaStartEnd.start.y * via_space_pixels,
        viaStartEnd.end.x * via_space_pixels,
        viaStartEnd.end.y * via_space_pixels,
        1.0f * zoom_, 0, 100, 200
      );
    }
  }
}

void PcbDraw::drawSolution(Solution& solution)
{
  float via_radius_pixels = VIA_RADIUS_PIXELS * zoom_;
  float strip_width_pixels = STRIP_WIDTH_PIXELS * zoom_;
  float strip_space_pixels = STRIP_SPACE_PIXELS * zoom_;
  float via_space_pixels = strip_width_pixels + strip_space_pixels;

  // Routes
  // Draw circles and lines separately so that lines are always on top.
  for (auto RouteStepVec : solution.getRouteVec()) {
    Via prev;
    for (auto c : RouteStepVec) {
      if (prev.isValid && prev.x == c.x) {
        drawFilledCircle(c.x * via_space_pixels, c.y * via_space_pixels,
                         via_radius_pixels, 200.0f, 200.0f, 0.0f);
      }
      prev = Via(c.x, c.y);
    }
  }
  for (auto RouteStepVec : solution.getRouteVec()) {
    Via prev;
    for (auto c : RouteStepVec) {
      if (prev.isValid && prev.y == c.y) {
        drawThickLine(prev.x * via_space_pixels, prev.y * via_space_pixels,
                      c.x * via_space_pixels, c.y * via_space_pixels,
                      1.5f * zoom_, 0, 0, 0);
      }
      prev = Via(c.x, c.y);
    }
  }
}


void PcbDraw::drawFilledRectangle(float x1, float y1, float x2, float y2, float r, float g, float b, float alpha)
{
  glUseProgram(fillProgramId);

  float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, alpha};
  GLint colorLoc = glGetUniformLocation(fillProgramId, "color");
  glProgramUniform4fv(fillProgramId, colorLoc, 1, color);

  vector<GLfloat> triVec;

  triVec.insert(triVec.end(), {x1, y1, 0.0f});
  triVec.insert(triVec.end(), {x2, y2, 0.0f});
  triVec.insert(triVec.end(), {x1, y2, 0.0f});

  triVec.insert(triVec.end(), {x1, y1, 0.0f});
  triVec.insert(triVec.end(), {x2, y1, 0.0f});
  triVec.insert(triVec.end(), {x2, y2, 0.0f});

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void PcbDraw::drawFilledCircle(float x, float y, float radius,
                               float r, float g, float b, float alpha)
{
  glUseProgram(fillProgramId);

  float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, alpha};
  GLint colorLoc = glGetUniformLocation(fillProgramId, "color");
  glProgramUniform4fv(fillProgramId, colorLoc, 1, color);

  vector<GLfloat> triVec;

  int numViaTriangles = NUM_VIA_TRIANGLES * zoom_;
  for (int i = 0; i <= numViaTriangles; ++i) {
    float x1 = x + (radius * cosf(i * 2.0f * PI_F / numViaTriangles));
    float y1 = y + (radius * sinf(i * 2.0f * PI_F / numViaTriangles));
    float x2 = x + (radius * cosf((i + 1) * 2.0f * PI_F / numViaTriangles));
    float y2 = y + (radius * sinf((i + 1) * 2.0f * PI_F / numViaTriangles));

    triVec.insert(triVec.end(), {x, y, 0.0f});
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
PcbDraw::drawThickLine(float x1, float y1, float x2, float y2, float radius,
                       float r, float g, float b, float alpha)
{
  drawFilledCircle(x1, y1, radius, r, g, b, alpha);
  drawFilledCircle(x2, y2, radius, r, g, b, alpha);

  glUseProgram(fillProgramId);

  float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, alpha};
  GLint colorLoc = glGetUniformLocation(fillProgramId, "color");
  glProgramUniform4fv(fillProgramId, colorLoc, 1, color);

  float t1 = radius * 2;
  float t2 = radius * 2;

  float angle = atan2(y2 - y1, x2 - x1);
  float t2sina1 = t1 / 2 * sin(angle);
  float t2cosa1 = t1 / 2 * cos(angle);
  float t2sina2 = t2 / 2 * sin(angle);
  float t2cosa2 = t2 / 2 * cos(angle);

  vector<GLfloat> triVec;

  triVec.insert(triVec.end(), {x1 + t2sina1, y1 - t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {x2 + t2sina2, y2 - t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {x2 - t2sina2, y2 + t2cosa2, 0.0f});

  triVec.insert(triVec.end(), {x2 - t2sina2, y2 + t2cosa2, 0.0f});
  triVec.insert(triVec.end(), {x1 - t2sina1, y1 + t2cosa1, 0.0f});
  triVec.insert(triVec.end(), {x1 + t2sina1, y1 - t2cosa1, 0.0f});

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}
