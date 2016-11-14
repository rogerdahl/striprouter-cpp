#pragma once

#include <vector>

#include "int_types.h"
#include "parser.h"
#include "ogl_text.h"

class PcbDraw {
public:
  PcbDraw(u32 window_w, u32 window_h);
  ~PcbDraw();
  void draw(Parser& parser);
private:
  void drawFilledRectangle(float x1, float y1, float x2, float y2, float r, float g, float b, float alpha=1.0f);
  void drawFilledCircle(float x, float y, float radius, float r, float g, float b, float alpha=1.0f);
  void drawThickLine(float x1, float y1, float x2, float y2, float radius, float r, float g, float b, float alpha=1.0f);
  u32 window_w_;
  u32 window_h_;
  OglText oglText_;
  GLuint fillProgramId;
//  GLuint textBackgroundProgramId;
  GLuint vertexBufId_;
};
