#pragma once

#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>

#include "int_types.h"


using namespace std;


struct CharMeta {
  u32 tex_x;
  u32 tex_y;
  u32 tex_w;
  u32 tex_h;
  s32 top_offset;
  s32 left_offset;
  s32 advance;
};

class OglText {
public:
  OglText(const string& fontPath, u32 fontH, u32 x=0, u32 y=0);
  ~OglText();
  void OpenGLInit();
  void reset(u32 windowW, u32 windowH, u32 x, u32 y, u32 fontH);
  void print(u32 nLine, const std::string &str);
  u32 calcStringWidth(const std::string &str);
  u32 getStringHeight();
private:
  void createFreeType(const std::string& fontPath);
  void createFontTexture();
  bool renderFont(std::vector<u8> &fontVec);
  void drawText(u32 nLine, const std::string &str);
  void drawTextBackground(u32 nLine, const std::string &str);

  bool oglInitialized_;

  u32 texWH_;
  u32 fontH_;

  u32 x_;
  u32 y_;

  GLuint textProgramId_;
  GLuint textBackgroundProgramId;

  u32 windowW_;
  u32 windowH_;

  FT_Library freetypeLibraryHandle_;
  FT_Face face_;

  GLuint textureId_;
  GLuint vertexBufId_;
  GLuint texBufId_;

  u32 lineH_;
  u32 maxAscender_;

  std::vector<CharMeta> charMeta_;
};
