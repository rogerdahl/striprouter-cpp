#define _USE_MATH_DEFINES

#include <cassert>
#include <iostream>
#include <cmath>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <fmt/format.h>

#include "pcb.h"
#include "shader.h"

using namespace std;
using namespace fmt;


const float PI_F = static_cast<float>(M_PI);
const float ZOOM = 2.5;
const int NUM_HOLE_TRIANGLES = 8 * ZOOM;
const float HOLE_RADIUS_PIXELS = 2.5f * ZOOM;
const int STRIP_WIDTH_PIXELS = 10 * ZOOM;
const int STRIP_SPACE_PIXELS = 2 * ZOOM;
const int HOLE_SPACE_PIXELS = STRIP_WIDTH_PIXELS + STRIP_SPACE_PIXELS;
const int W_HOLES = 80;
const int H_HOLES = 80;

const char *FONT_PATH2 = "./fonts/LiberationMono-Regular.ttf";
const u32 FONT_SIZE2 = 10 * ZOOM;


PcbDraw::PcbDraw(u32 window_w, u32 window_h)
    : window_w_(window_w), window_h_(window_h),
      oglText_(OglText(window_w, window_h, FONT_PATH2, FONT_SIZE2))
{
  fillProgramId = createProgram("fill.vert", "fill.frag");
  glGenBuffers(1, &vertexBufId_);
}

PcbDraw::~PcbDraw()
{
  glDeleteBuffers(1, &vertexBufId_);
}


void PcbDraw::draw(Parser& parser)
{
  auto projection = glm::ortho(0.0f, static_cast<float>(window_w_), static_cast<float>(window_h_),
                               0.0f, 0.0f, 100.0f);

  glUseProgram(fillProgramId);
  GLint projectionId = glGetUniformLocation(fillProgramId, "projection");
  assert(projectionId >= 0);
  glUniformMatrix4fv(projectionId, 1, GL_FALSE, glm::value_ptr(projection));

  // Draw stripboard

  for (int x = 0; x < W_HOLES; ++x) {
    drawFilledRectangle(x * HOLE_SPACE_PIXELS - (STRIP_WIDTH_PIXELS / 2), 0,
                        x * HOLE_SPACE_PIXELS + (STRIP_WIDTH_PIXELS / 2), window_h_,
                        217.0f, 144.0f, 88.0f);
  }

  for (int y = 0; y < H_HOLES; ++y) {
    for (int x = 0; x < W_HOLES; ++x) {
      drawFilledCircle(x * HOLE_SPACE_PIXELS, y * HOLE_SPACE_PIXELS, HOLE_RADIUS_PIXELS, 0.0f, 0.0f, 0.0f);
    }
  }

  // Draw components

//  s32 REC_WIDTH_PIXELS = HOLE_SPACE_PIXELS / 2;
//  s32 EXT_PIXELS = 3;
  u32 PIX1 = HOLE_SPACE_PIXELS / 2;
  u32 W = 2;

  for (auto componentName : parser.getComponentNameVec()) {
    auto ci = parser.getComponentInfo(componentName);

    // Component outline
    drawFilledRectangle(
        ci.extent.x1 * HOLE_SPACE_PIXELS - PIX1 - W, ci.extent.y1 * HOLE_SPACE_PIXELS - PIX1 - W,
        ci.extent.x2 * HOLE_SPACE_PIXELS + PIX1 + W, ci.extent.y2 * HOLE_SPACE_PIXELS + PIX1 + W,
        0, 0, 0, 0.5f
    );

    // Component pins
    for (auto pinAbsCoord :  ci.pinAbsCoordVec) {
      s32 x = pinAbsCoord.first;
      s32 y = pinAbsCoord.second;
      drawFilledCircle(x * HOLE_SPACE_PIXELS, y * HOLE_SPACE_PIXELS, HOLE_RADIUS_PIXELS, 200.0f, 0.0f, 0.0f);
    }

    // Component name
    u32 stringWidth = oglText_.calcStringWidth(ci.componentName);
    u32 stringHeight = oglText_.getStringHeight();
    u32 x1 = ci.extent.x1 * HOLE_SPACE_PIXELS;
    u32 y1 = ci.extent.y1 * HOLE_SPACE_PIXELS;
    u32 x2 = ci.extent.x2 * HOLE_SPACE_PIXELS;
    u32 y2 = ci.extent.y2 * HOLE_SPACE_PIXELS;
    u32 centerX = x1 + (x2 - x1) / 2;
    u32 centerY = y1 + (y2 - y1) / 2;

    oglText_.print(
        centerX - stringWidth / 2,
        centerY - stringHeight / 2,
        0,
        ci.componentName
    );
  }

  for (auto fromToCoord : parser.getConnectionCoordVec()) {
    drawThickLine(
        fromToCoord.x1 * HOLE_SPACE_PIXELS,
        fromToCoord.y1 * HOLE_SPACE_PIXELS,
        fromToCoord.x2 * HOLE_SPACE_PIXELS,
        fromToCoord.y2 * HOLE_SPACE_PIXELS,
        3.0f, 0, 100, 200
    );
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
  glVertexAttribPointer(
      0,         // attribute
      3,         // size
      GL_FLOAT,  // type
      GL_FALSE,  // normalized?
      0,         // stride
      (void *) 0   // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void PcbDraw::drawFilledCircle(float x, float y, float radius, float r, float g, float b, float alpha)
{
  glUseProgram(fillProgramId);

  float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, alpha};
  GLint colorLoc = glGetUniformLocation(fillProgramId, "color");
  glProgramUniform4fv(fillProgramId, colorLoc, 1, color);

  vector<GLfloat> triVec;

  for (int i = 0; i <= NUM_HOLE_TRIANGLES; ++i) {
    float x1 = x + (radius * cosf(i * 2.0f * PI_F / NUM_HOLE_TRIANGLES));
    float y1 = y + (radius * sinf(i * 2.0f * PI_F / NUM_HOLE_TRIANGLES));
    float x2 = x + (radius * cosf((i + 1) * 2.0f * PI_F / NUM_HOLE_TRIANGLES));
    float y2 = y + (radius * sinf((i + 1) * 2.0f * PI_F / NUM_HOLE_TRIANGLES));

    triVec.insert(triVec.end(), {x, y, 0.0f});
    triVec.insert(triVec.end(), {x1, y1, 0.0f});
    triVec.insert(triVec.end(), {x2, y2, 0.0f});
  }

  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, vertexBufId_);
  glBufferData(GL_ARRAY_BUFFER, triVec.size() * sizeof(GLfloat), &triVec[0], GL_STATIC_DRAW);
  glVertexAttribPointer(
      0,         // attribute
      3,         // size
      GL_FLOAT,  // type
      GL_FALSE,  // normalized?
      0,         // stride
      (void *) 0   // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}

void PcbDraw::drawThickLine(float x1, float y1, float x2, float y2, float radius, float r, float g, float b, float alpha)
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
  glVertexAttribPointer(
      0,         // attribute
      3,         // size
      GL_FLOAT,  // type
      GL_FALSE,  // normalized?
      0,         // stride
      (void *) 0   // array buffer offset
  );

  glDrawArrays(GL_TRIANGLES, 0, triVec.size());
}
